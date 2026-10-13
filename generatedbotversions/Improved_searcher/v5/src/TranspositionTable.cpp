#include "TranspositionTable.h"
#include <bit>
#include <algorithm>
#include <climits>
#include "Searcher.h"

using namespace ImprovedTT;
using namespace ImprovedSearcher;

namespace {

// payload layout (64 bits): score 16 | move 16 | piece type 8 | depth 8 | flag 8 | generation 8
struct Unpacked {
    int16_t  score;
    uint16_t moveData;
    uint8_t  movePieceType;
    uint8_t  depth;
    uint8_t  type;
    uint8_t  gen;
};

inline uint64_t Pack(const Unpacked& u) {
    return  (uint64_t)(uint16_t)u.score
         | ((uint64_t)u.moveData      << 16)
         | ((uint64_t)u.movePieceType << 32)
         | ((uint64_t)u.depth         << 40)
         | ((uint64_t)u.type          << 48)
         | ((uint64_t)u.gen           << 56);
}

inline Unpacked Unpack(uint64_t d) {
    Unpacked u;
    u.score         = (int16_t)(uint16_t)(d & 0xFFFF);
    u.moveData      = (uint16_t)((d >> 16) & 0xFFFF);
    u.movePieceType = (uint8_t)((d >> 32) & 0xFF);
    u.depth         = (uint8_t)((d >> 40) & 0xFF);
    u.type          = (uint8_t)((d >> 48) & 0xFF);
    u.gen           = (uint8_t)((d >> 56) & 0xFF);
    return u;
}

// a stored entry always has a non-zero flag, so data == 0 means the slot is empty
inline bool IsEmpty(uint64_t data) { return data == 0; }

inline void Write(TTEntry& e, uint64_t hash, uint64_t data) {
    e.data.store(data, std::memory_order_relaxed);
    e.keyXorData.store(hash ^ data, std::memory_order_relaxed);
}

}

TranspositionTable::TranspositionTable(size_t mb) {
    size_t sizeInBytes = mb * 1024 * 1024;
    size_t clusterCount = sizeInBytes / sizeof(TTCluster);
    size = clusterCount < 2 ? 1 : std::bit_floor(clusterCount);

    table = std::make_unique<TTCluster[]>(size);
}

void TranspositionTable::Prefetch(uint64_t hash) {
    size_t index = hash & (size - 1);
    _mm_prefetch(reinterpret_cast<const char*>(&table[index]), _MM_HINT_T0);
}

int TranspositionTable::ScoreToTT(int score, int ply) {
    if (std::abs(score) >= Searcher::MATE_SCORE_BOUND) {
        return score > 0 ? score + ply : score - ply;
    }
    return score;
}

int TranspositionTable::ScoreFromTT(int score, int ply) {
    if (std::abs(score) >= Searcher::MATE_SCORE_BOUND) {
        return score > 0 ? score - ply : score + ply;
    }
    return score;
}

void TranspositionTable::Store(uint64_t hash, int score, int ply, int depth, TTFlag flag, Move bestMove) {
    const uint8_t storeDepth = (uint8_t)std::clamp(depth, 0, 127);

    TTCluster& cluster = table[hash & (size - 1)];

    int replaceIndex = 0;
    int worstScore = INT_MIN;
    uint64_t replaceData = 0;
    bool sameKey = false;

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        const uint64_t data = cluster.entry[i].data.load(std::memory_order_relaxed);
        const uint64_t key = cluster.entry[i].keyXorData.load(std::memory_order_relaxed) ^ data;

        if (IsEmpty(data)) {
            replaceIndex = i;
            replaceData = 0;
            break;
        }
        if (key == hash) {
            replaceIndex = i;
            replaceData = data;
            sameKey = true;
            break;
        }

        const Unpacked u = Unpack(data);
        const int age = (uint8_t)(generation - u.gen); // wraps correctly after 256 generations
        const int badness = (age * 4) - (int)u.depth;

        if (badness > worstScore) {
            worstScore = badness;
            replaceIndex = i;
            replaceData = data;
        }
    }

    Unpacked u;

    if (sameKey) {
        u = Unpack(replaceData);

        // a much shallower search (e.g. quiescence) must not clobber the move found by a deeper one
        if (bestMove.isValid() && (u.moveData == 0 || storeDepth + 2 >= u.depth)) {
            u.moveData = bestMove.getMoveData();
            u.movePieceType = bestMove.getPieceType();
        }

        if (storeDepth >= u.depth || flag == TT_EXACT) {
            u.score = (int16_t)ScoreToTT(score, ply);
            u.depth = storeDepth;
            u.type = (uint8_t)flag;
        }
        u.gen = generation;
    } else {
        u.score = (int16_t)ScoreToTT(score, ply);
        u.moveData = bestMove.isValid() ? bestMove.getMoveData() : 0;
        u.movePieceType = bestMove.isValid() ? bestMove.getPieceType() : 0;
        u.depth = storeDepth;
        u.type = (uint8_t)flag;
        u.gen = generation;
    }

    Write(cluster.entry[replaceIndex], hash, Pack(u));
}

bool TranspositionTable::Probe(uint64_t hash, int ply, int depth, int alpha, int beta, int& score, Move& bestMove, int& ttDepth, TTFlag& ttFlag) {
    TTCluster& cluster = table[hash & (size - 1)];

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        const uint64_t data = cluster.entry[i].data.load(std::memory_order_relaxed);
        const uint64_t key = cluster.entry[i].keyXorData.load(std::memory_order_relaxed) ^ data;

        if (IsEmpty(data) || key != hash) continue;

        const Unpacked u = Unpack(data);

        bestMove = u.moveData != 0 ? Move(u.moveData, u.movePieceType) : Move();

        const int retrievedScore = ScoreFromTT(u.score, ply);
        score = retrievedScore;
        ttDepth = u.depth;
        ttFlag = static_cast<TTFlag>(u.type);

        if (u.depth >= depth) {
            if (u.type == TT_EXACT) return true;
            if (u.type == TT_ALPHA && retrievedScore <= alpha) return true;
            if (u.type == TT_BETA && retrievedScore >= beta) return true;
        }
        return false;
    }

    return false;
}

Move TranspositionTable::getPvMove(uint64_t hash) {
    TTCluster& cluster = table[hash & (size - 1)];

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        const uint64_t data = cluster.entry[i].data.load(std::memory_order_relaxed);
        const uint64_t key = cluster.entry[i].keyXorData.load(std::memory_order_relaxed) ^ data;

        if (IsEmpty(data) || key != hash) continue;

        const Unpacked u = Unpack(data);
        if (u.type == TT_EXACT && u.moveData != 0) {
            return Move(u.moveData, u.movePieceType);
        }
        break;
    }
    return Move();
}

void TranspositionTable::Clear() {
    for (size_t c = 0; c < size; c++) {
        for (int i = 0; i < CLUSTER_SIZE; i++) {
            table[c].entry[i].data.store(0, std::memory_order_relaxed);
            table[c].entry[i].keyXorData.store(0, std::memory_order_relaxed);
        }
    }
    generation = 0;
}
