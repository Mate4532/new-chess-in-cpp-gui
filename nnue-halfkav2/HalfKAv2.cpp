#include "HalfKAv2.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace HalfKAv2 {

namespace {

    constexpr uint32_t FeatureHash = 0x7f234cb8u;
    constexpr int OutputScale = 16;
    constexpr int WeightScaleBits = 6;
    constexpr int FC0Total = FC0Outputs + 1;

    struct alignas(64) LayerStack {
        int32_t fc0Bias[FC0Total];
        alignas(64) int8_t fc0Weight[FC0Total][L1];
        int32_t fc1Bias[FC1Outputs];
        alignas(64) int8_t fc1Weight[FC1Outputs][FC1Inputs];
        int32_t fc2Bias;
        alignas(64) int8_t fc2Weight[FC1Outputs];
    };

    struct Network {
        alignas(64) int16_t ftBias[L1];
        std::unique_ptr<int16_t[]> ftWeight;
        std::unique_ptr<int32_t[]> psqtWeight;
        std::unique_ptr<LayerStack[]> stacks;
    };

    Network net;
    bool loaded = false;

    constexpr int PS_NB = 11 * 64;

    constexpr int PieceSquareIndex[2][16] = {
        { 0, 0 * 64, 2 * 64, 4 * 64, 6 * 64, 8 * 64, 10 * 64, 0,
          0, 1 * 64, 3 * 64, 5 * 64, 7 * 64, 9 * 64, 10 * 64, 0 },
        { 0, 1 * 64, 3 * 64, 5 * 64, 7 * 64, 9 * 64, 10 * 64, 0,
          0, 0 * 64, 2 * 64, 4 * 64, 6 * 64, 8 * 64, 10 * 64, 0 }
    };

    inline int KingBucket(int sq) {
        const int rank = sq >> 3;
        const int file = sq & 7;
        const int col = file <= 3 ? file : 7 - file;
        return ((7 - rank) * 4 + col) * PS_NB;
    }

    inline int Orient(int ksq) {
        return (ksq & 7) <= 3 ? 7 : 0;
    }

    inline int MakeIndex(Color perspective, int sq, int piece, int ksq) {
        const int flip = 56 * (int)perspective;
        return (sq ^ Orient(ksq) ^ flip) + PieceSquareIndex[perspective][piece] + KingBucket(ksq ^ flip);
    }

    inline void AddFeature(Accumulator& acc, Color perspective, int index) {
        const int16_t* w = &net.ftWeight[(size_t)index * L1];
        int16_t* v = acc.values[perspective];
        for (int j = 0; j < L1; j++) v[j] += w[j];
        const int32_t* p = &net.psqtWeight[(size_t)index * PsqtBuckets];
        for (int b = 0; b < PsqtBuckets; b++) acc.psqt[perspective][b] += p[b];
    }

    inline void RemoveFeature(Accumulator& acc, Color perspective, int index) {
        const int16_t* w = &net.ftWeight[(size_t)index * L1];
        int16_t* v = acc.values[perspective];
        for (int j = 0; j < L1; j++) v[j] -= w[j];
        const int32_t* p = &net.psqtWeight[(size_t)index * PsqtBuckets];
        for (int b = 0; b < PsqtBuckets; b++) acc.psqt[perspective][b] -= p[b];
    }

    void ApplyDirty(const Accumulator& prev, Accumulator& next, const DirtyPieces& dirty, Color perspective, int ksq) {
        std::memcpy(next.values[perspective], prev.values[perspective], sizeof(next.values[perspective]));
        std::memcpy(next.psqt[perspective], prev.psqt[perspective], sizeof(next.psqt[perspective]));
        for (int i = 0; i < dirty.count; i++) {
            if (dirty.from[i] != NoSquare) RemoveFeature(next, perspective, MakeIndex(perspective, dirty.from[i], dirty.piece[i], ksq));
            if (dirty.to[i] != NoSquare) AddFeature(next, perspective, MakeIndex(perspective, dirty.to[i], dirty.piece[i], ksq));
        }
        next.computed[perspective] = true;
    }

    inline bool MovesKing(const DirtyPieces& dirty, Color perspective) {
        const int king = PieceCode(KING, perspective);
        for (int i = 0; i < dirty.count; i++) {
            if (dirty.piece[i] == king) return true;
        }
        return false;
    }

    inline int32_t Dot(const uint8_t* input, const int8_t* weights, int size) {
#ifdef __AVX2__
        __m256i sum = _mm256_setzero_si256();
        const __m256i ones = _mm256_set1_epi16(1);
        for (int i = 0; i < size; i += 32) {
            const __m256i in = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            const __m256i w = _mm256_load_si256(reinterpret_cast<const __m256i*>(weights + i));
            const __m256i product = _mm256_maddubs_epi16(in, w);
            sum = _mm256_add_epi32(sum, _mm256_madd_epi16(product, ones));
        }
        __m128i s = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(1, 0, 3, 2)));
        s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
        return _mm_cvtsi128_si32(s);
#else
        int32_t sum = 0;
        for (int i = 0; i < size; i++) sum += (int32_t)input[i] * weights[i];
        return sum;
#endif
    }

    int Propagate(const Accumulator& acc, Color stm, int bucket) {
        const Color perspectives[2] = { stm, (Color)(stm ^ 1) };

        alignas(64) uint8_t transformed[L1];
        for (int p = 0; p < 2; p++) {
            const int16_t* v = acc.values[perspectives[p]];
            uint8_t* out = transformed + p * (L1 / 2);
            for (int j = 0; j < L1 / 2; j++) {
                const int a = std::clamp<int>(v[j], 0, 127);
                const int b = std::clamp<int>(v[j + L1 / 2], 0, 127);
                out[j] = (uint8_t)((unsigned)(a * b) / 128);
            }
        }

        const LayerStack& s = net.stacks[bucket];

        int32_t fc0[FC0Total];
        for (int o = 0; o < FC0Total; o++) {
            fc0[o] = s.fc0Bias[o] + Dot(transformed, s.fc0Weight[o], L1);
        }

        alignas(64) uint8_t fc1In[FC1Inputs] = {};
        for (int i = 0; i < FC0Outputs; i++) {
            fc1In[i] = (uint8_t)std::min<long long>(127, ((long long)fc0[i] * fc0[i]) >> (2 * WeightScaleBits + 7));
            fc1In[FC0Outputs + i] = (uint8_t)std::clamp(fc0[i] >> WeightScaleBits, 0, 127);
        }

        alignas(64) uint8_t fc2In[FC1Outputs];
        for (int o = 0; o < FC1Outputs; o++) {
            int32_t sum = s.fc1Bias[o];
            for (int i = 0; i < FC1Inputs; i++) sum += (int32_t)fc1In[i] * s.fc1Weight[o][i];
            fc2In[o] = (uint8_t)std::clamp(sum >> WeightScaleBits, 0, 127);
        }

        int32_t fc2 = s.fc2Bias;
        for (int i = 0; i < FC1Outputs; i++) fc2 += (int32_t)fc2In[i] * s.fc2Weight[i];

        const int32_t fwdOut = fc0[FC0Outputs] * (600 * OutputScale) / (127 * (1 << WeightScaleBits));
        const int32_t positional = fc2 + fwdOut;
        const int32_t psqt = (acc.psqt[perspectives[0]][bucket] - acc.psqt[perspectives[1]][bucket]) / 2;

        return (psqt + positional) / OutputScale;
    }

    int Bucket(const Board& board) {
        int pieces = 0;
        for (int c = WHITE; c <= BLACK; c++) {
            for (int pt = PAWN; pt < PIECE_TYPE_COUNT; pt++) {
                pieces += __builtin_popcountll(board.getPieceBitboard((Color)c, (PieceType)pt));
            }
        }
        return (pieces - 1) / 4;
    }

    template <typename T>
    bool Read(std::ifstream& in, T* dst, size_t count) {
        in.read(reinterpret_cast<char*>(dst), (std::streamsize)(sizeof(T) * count));
        return (bool)in;
    }

    uint32_t AffineHash(uint32_t prev, uint32_t outputs) {
        uint32_t h = 0xCC03DAE4u + outputs;
        h ^= prev >> 1;
        h ^= prev << 31;
        return h;
    }

    uint32_t NetworkHash() {
        uint32_t h = 0xEC42E90Du ^ (L1 * 2);
        h = AffineHash(h, FC0Total);
        h = 0x538D24C7u + h;
        h = AffineHash(h, FC1Outputs);
        h = 0x538D24C7u + h;
        h = AffineHash(h, 1);
        return h;
    }

}

int PieceCode(PieceType pt, Color c) {
    if (pt == AMAZON) pt = QUEEN;
    return (int)c * 8 + (int)pt + 1;
}

bool IsLoaded() {
    return loaded;
}

bool Load(const std::string& path) {
    loaded = false;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "HalfKAv2: cannot open " << path << std::endl;
        return false;
    }

    uint32_t version = 0, hash = 0, descSize = 0;
    if (!Read(in, &version, 1) || !Read(in, &hash, 1) || !Read(in, &descSize, 1)) return false;
    in.seekg(descSize, std::ios::cur);

    const uint32_t ftHash = FeatureHash ^ (L1 * 2);
    const uint32_t stackHash = NetworkHash();

    uint32_t readFtHash = 0;
    if (!Read(in, &readFtHash, 1) || readFtHash != ftHash || hash != (ftHash ^ stackHash)) {
        std::cerr << "HalfKAv2: " << path << " is not a HalfKAv2_hm 1024x2-16-32-1 network" << std::endl;
        return false;
    }

    net.ftWeight = std::make_unique<int16_t[]>((size_t)InputDimensions * L1);
    net.psqtWeight = std::make_unique<int32_t[]>((size_t)InputDimensions * PsqtBuckets);
    net.stacks = std::make_unique<LayerStack[]>(LayerStacks);

    bool ok = Read(in, net.ftBias, L1)
           && Read(in, net.ftWeight.get(), (size_t)InputDimensions * L1)
           && Read(in, net.psqtWeight.get(), (size_t)InputDimensions * PsqtBuckets);

    for (int i = 0; ok && i < LayerStacks; i++) {
        LayerStack& s = net.stacks[i];
        uint32_t h = 0;
        ok = Read(in, &h, 1) && h == stackHash
          && Read(in, s.fc0Bias, FC0Total) && Read(in, &s.fc0Weight[0][0], (size_t)FC0Total * L1)
          && Read(in, s.fc1Bias, FC1Outputs) && Read(in, &s.fc1Weight[0][0], (size_t)FC1Outputs * FC1Inputs)
          && Read(in, &s.fc2Bias, 1) && Read(in, s.fc2Weight, FC1Outputs);
    }

    if (!ok || in.peek() != std::ifstream::traits_type::eof()) {
        std::cerr << "HalfKAv2: " << path << " has an unexpected layout" << std::endl;
        net.ftWeight.reset();
        net.psqtWeight.reset();
        net.stacks.reset();
        return false;
    }

    std::cout << "HalfKAv2 NNUE loaded: " << path << std::endl;
    loaded = true;
    return true;
}

void Refresh(const Board& board, Color perspective, Accumulator& acc) {
    std::memcpy(acc.values[perspective], net.ftBias, sizeof(net.ftBias));
    std::memset(acc.psqt[perspective], 0, sizeof(acc.psqt[perspective]));

    const int ksq = board.getKingSquare(perspective);
    for (int c = WHITE; c <= BLACK; c++) {
        for (int pt = PAWN; pt < PIECE_TYPE_COUNT; pt++) {
            const int piece = PieceCode((PieceType)pt, (Color)c);
            uint64_t bb = board.getPieceBitboard((Color)c, (PieceType)pt);
            while (bb) {
                const int sq = PopBit(bb);
                AddFeature(acc, perspective, MakeIndex(perspective, sq, piece, ksq));
            }
        }
    }
    acc.computed[perspective] = true;
}

int Evaluate(const Board& board, Accumulator* stack, const DirtyPieces* dirty, int ply) {
    for (int p = WHITE; p <= BLACK; p++) {
        const Color perspective = (Color)p;
        if (stack[ply].computed[perspective]) continue;

        int k = ply;
        bool incremental = true;
        while (!stack[k].computed[perspective]) {
            if (k == 0 || MovesKing(dirty[k], perspective)) {
                incremental = false;
                break;
            }
            k--;
        }

        if (!incremental) {
            Refresh(board, perspective, stack[ply]);
            continue;
        }

        const int ksq = board.getKingSquare(perspective);
        for (int i = k + 1; i <= ply; i++) {
            ApplyDirty(stack[i - 1], stack[i], dirty[i], perspective, ksq);
        }
    }

    return Propagate(stack[ply], board.getSideToMove(), Bucket(board));
}

int EvaluateFromScratch(const Board& board) {
    auto acc = std::make_unique<Accumulator>();
    Refresh(board, WHITE, *acc);
    Refresh(board, BLACK, *acc);
    return Propagate(*acc, board.getSideToMove(), Bucket(board));
}

}
