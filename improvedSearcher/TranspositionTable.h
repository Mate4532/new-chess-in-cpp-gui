#pragma once
#include <vector>
#include <cstdint>
#include "Move.h"

namespace ImprovedTT {

static constexpr int CLUSTER_SIZE = 4;

    enum TTFlag : uint8_t {
        TT_NONE = 0,
        TT_EXACT = 1,
        TT_ALPHA = 2,
        TT_BETA = 3
    };

    struct TTEntry {
        uint64_t key;
        uint16_t moveData;
        int16_t score;
        uint8_t pieceType;
        int8_t depth;
        uint8_t type;
        uint8_t gen;
    };

    struct TTCluster {
        TTEntry entry[CLUSTER_SIZE];
    };

    class TranspositionTable {
    public:
        TranspositionTable(size_t mb);

        void Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove);
        bool Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove);

        void Clear();
        void NewWrite() { generation++; }

    private:
        std::vector<TTCluster> table;
        size_t size;
        uint8_t generation = 0;
        size_t NextPowerOf2(size_t n);
    };

}
