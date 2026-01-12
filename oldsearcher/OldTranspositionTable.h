#pragma once
#include <vector>
#include <cstdint>
#include "Move.h"

namespace OldTT {

    enum TTFlag : uint8_t { TT_NONE, TT_EXACT, TT_ALPHA, TT_BETA };

    struct TTEntry {
        uint64_t key;
        int32_t  score;
        Move move;
        int8_t   depth;
        uint8_t  type;
        uint8_t  gen;
    };

    class TranspositionTable {
    public:
        TranspositionTable(size_t mb);

        void NewWrite() { generation++; }

        void Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove);
        bool Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove);
        void Clear();

    private:
        std::vector<TTEntry> table;
        uint8_t generation = 0;
        size_t NextPowerOf2(size_t n);
    };
}
