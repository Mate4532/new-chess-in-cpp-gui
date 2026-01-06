#include "MoveOrdering.h"
#include "Evaluation.h"

static inline int ScoreMove(
    const Board& board,
    const Move& m,
    const Move& ttMove,
    const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY],
    const Move killers[2]
) {
    Color us = board.getSideToMove();
    Color enemy = (Color)(us ^ 1);

    if (m.isValid() && ttMove.isValid() && m.getMoveData() == ttMove.getMoveData())
        return 10'000'000;

    if (m.getFlags() & CAPTURE_FLAG) {
        PieceType victim =
            (m.getFlags() == EN_PASSANT)
            ? PAWN
            : board.getPieceAt(m.getTo(), enemy);

        int v = Evaluation::GetPieceValue(victim);
        int a = Evaluation::GetPieceValue(m.getPieceType());
        return 5'000'000 + v * 16 - a;
    }

    if (m.getFlags() & PROMOTION_FLAG) {
        if ((m.getFlags() & 0b0011) == PROMOTION_TYPE_QUEEN)
            return 4'000'000;
        return 3'000'000;
    }

    if (m.isValid()) {
        if (m.getMoveData() == killers[0].getMoveData()) return 2'000'000;
        if (m.getMoveData() == killers[1].getMoveData()) return 1'500'000;
    }
    return history[us][m.getFrom()][m.getTo()];
}

int MoveOrdering::SortMoves(
    const Board& board,
    MoveList& moves,
    Move ttMove,
    const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY],
    const Move killers[2]
) {
    int scores[256];
    int counter = 0;

    int n = (int)moves.size();
    for (int i = 0; i < n; i++) {
        scores[i] = ScoreMove(
            board,
            moves[i],
            ttMove,
            history,
            killers
        );
		if (scores[i] >= 1'500'000) counter++;
    }

    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[best])
                best = j;
        }
        if (best != i) {
            std::swap(scores[i], scores[best]);
            std::swap(moves[i], moves[best]);
        }
    }
	return counter;
}
