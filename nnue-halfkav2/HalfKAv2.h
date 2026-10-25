#pragma once

#include <cstdint>
#include <string>
#include "Board.h"

namespace HalfKAv2 {

    constexpr int InputDimensions = 22528;
    constexpr int L1 = 1024;
    constexpr int PsqtBuckets = 8;
    constexpr int LayerStacks = 8;
    constexpr int FC0Outputs = 15;
    constexpr int FC1Inputs = 32;
    constexpr int FC1Outputs = 32;
    constexpr int NoSquare = 64;

    struct alignas(64) Accumulator {
        int16_t values[2][L1];
        int32_t psqt[2][PsqtBuckets];
        bool computed[2];
    };

    struct DirtyPieces {
        int count = 0;
        int piece[3] = {};
        int from[3] = {};
        int to[3] = {};
    };

    bool Load(const std::string& path);
    bool IsLoaded();

    int PieceCode(PieceType pt, Color c);

    void Refresh(const Board& board, Color perspective, Accumulator& acc);
    int Evaluate(const Board& board, Accumulator* stack, const DirtyPieces* dirty, int ply);
    int EvaluateFromScratch(const Board& board);

}
