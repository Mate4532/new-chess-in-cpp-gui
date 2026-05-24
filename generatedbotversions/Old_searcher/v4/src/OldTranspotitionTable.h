#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <xmmintrin.h>
#include "Move.h"

namespace OldTT {

    enum TTFlag : uint8_t { TT_NONE, TT_EXACT, TT_ALPHA, TT_BETA };

    struct TTEntry {
        uint64_t key;
        int16_t  score;
        uint16_t moveData;
        uint8_t  movePieceType;
        int8_t   depth;
        uint8_t  type;
        uint8_t  gen;
    };

    constexpr int CLUSTER_SIZE = 4;

    struct TTCluster {
        TTEntry entry[CLUSTER_SIZE];
    };

    class TranspositionTable {
    public:
        TranspositionTable(size_t mb);

        void NewWrite() { generation++; }

        void Prefetch(uint64_t hash);

        void Store(uint64_t hash, int score, int ply, int depth, TTFlag flag, Move bestMove);

        bool Probe(uint64_t hash, int ply, int depth, int alpha, int beta, int& score, Move& bestMove);

        void Clear();

    private:
        std::vector<TTCluster> table;
        size_t size;
        uint8_t generation = 0;

        int ScoreToTT(int score, int ply);
        int ScoreFromTT(int score, int ply);
    };

}