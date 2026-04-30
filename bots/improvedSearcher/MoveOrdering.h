#pragma once
#include "Board.h"
#include "MoveList.h"

namespace ImprovedMoveOrdering {

#define MAX_KILLER_HISTORY 128

    class MoveOrdering {
    public:
        static constexpr int SCORE_SIZE = 256;
        static int See(const Board& board, Move m);
        static void ScoreMoves(
            const Board& board,
            MoveList& moves,
            Move ttMove,
            const int history[2][SQUARE_COUNT][SQUARE_COUNT],
            const Move killers[2],
            int scores[SCORE_SIZE]
        );
    };
}
