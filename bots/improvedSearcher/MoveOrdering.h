#pragma once
#include "Board.h"
#include "MoveList.h"

namespace ImprovedMoveOrdering {

#define MAX_KILLER_HISTORY 128

    class MoveOrdering {
    public:
        static int See(const Board& board, Move m);
        static void SortMoves(
            const Board& board,
            MoveList& moves,
            Move ttMove,
            const int history[2][SQUARE_COUNT][SQUARE_COUNT],
            const Move killers[2]
        );
    };
}
