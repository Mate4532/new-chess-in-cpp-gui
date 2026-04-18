#pragma once
#include "Board.h"
#include "MoveList.h"

namespace OldMoveOrdering {

#define MAX_KILLER_HISTORY 128

    class MoveOrdering {
    public:
        static int SortMoves(
            const Board& board,
            MoveList& moves,
            Move ttMove,
            const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY],
            const Move killers[2]
        );
    };
}
