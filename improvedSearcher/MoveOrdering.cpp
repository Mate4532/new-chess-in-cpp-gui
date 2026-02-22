#include "MoveOrdering.h"
#include "Evaluation.h"

using namespace ImprovedMoveOrdering;
using namespace ImprovedEvaluation;

int MoveOrdering::See(const Board& board, Move m) {
    Square from = m.getFrom();
    Square to = m.getTo();
    PieceType attacker = m.getPieceType();
    PieceType victim = board.getPieceAt(to, (Color)(board.getSideToMove() ^ 1));
    if (m.getFlags() == EN_PASSANT) victim = PAWN;
    int gain[32];
    int d = 0;
    uint64_t occupied = board.getAllOccupancy();
    uint64_t attackers = board.getAttacksTo(to, occupied);

    gain[d] = Evaluation::GetPieceValue(victim);
    Color side = board.getSideToMove();

    occupied ^= (1ULL << from);
    attackers |= board.getNewXRayAttacks(from, occupied);

    while (true) {
        side = (Color)(side ^ 1);
        attackers &= occupied;

        PieceType nextAttacker;
        Square nextSq = board.getSmallestAttacker(attackers, side, nextAttacker);

        if (nextSq == SQUARE_NONE) break;

        d++;
        gain[d] = Evaluation::GetPieceValue(nextAttacker) - gain[d - 1];

        if (std::max(-gain[d - 1], gain[d]) < 0) break;

        occupied ^= (1ULL << nextSq);
        attackers |= board.getNewXRayAttacks(nextSq, occupied);
    }

    while (--d > 0) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }

    return gain[0];
}


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

        int mvv_lva = v * 16 - a;

        if (MoveOrdering::See(board, m) >= 0) {
            return 5'000'000 + mvv_lva;
        } else {
            return 1'000'000 + mvv_lva;
        }
    }

    if (m.getFlags() & PROMOTION_FLAG) {
        if ((m.getFlags() & 0b0011) == PROMOTION_TYPE_QUEEN)
            return 4'000'000;
        return 3'000'000;
    }

    if (m.getPieceType() == PAWN) {
        int rank = m.getTo() >> 3;
        bool dangerous = false;

        if (us == WHITE) {
            if (rank >= 6) dangerous = true;
        } else {
            if (rank <= 1) dangerous = true;
        }

        if (dangerous) {
            return 2'500'000;
        }
    }

    if (m.isValid()) {
        if (m == killers[0]) return 2'000'000;
        if (m == killers[1]) return 1'500'000;
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
