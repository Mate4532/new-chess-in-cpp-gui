#pragma once
#include "Board.h"
#include "nnue.h"

namespace OldEvaluation {

    class Evaluation {
    public:
        static int GetPieceValue(PieceType p);
        static int GetNnuePieceNum(PieceType p, Color c);
        static int EvaluatePos(const Board& board, int ply, NNUEdata* nnue_state);
    };
}