#include "OldMoveOrdering.h"
#include "OldEvaluation.h"

using namespace OldMoveOrdering;
using namespace OldEvaluation;

int MoveOrdering::See(const Board& board, Move m) {
    Square from = m.getFrom();
    Square to = m.getTo();
    MoveFlag flags = m.getFlags();
    Color us = board.getSideToMove();

    PieceType victim = board.getPieceAt(to, (Color)(us ^ 1));
    if (flags == EN_PASSANT) victim = PAWN;

    PieceType movingPiece = m.getPieceType();

    PieceType pieceOnTarget = movingPiece;
    if (flags & PROMOTION_FLAG) {
        pieceOnTarget = Board::GetPromotionPiece(flags);
    }

    int gain[32];
    int d = 0;

    uint64_t occupied = board.getAllOccupancy();
    uint64_t attackers = board.getAttacksTo(to, occupied);

    gain[d] = Evaluation::GetPieceValue(victim);
    Color side = us;

    if (flags == EN_PASSANT) {
        Square epSquare = (Square)((int)to + (us == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET));
        occupied ^= (1ULL << epSquare);
    }

    occupied ^= (1ULL << from);

    uint64_t sliderAttacks = (board.getBishopAttacks(to, occupied) & board.getBishopsQueens()) |
        (board.getRookAttacks(to, occupied) & board.getRooksQueens());
    attackers |= sliderAttacks;

    while (true) {
        side = (Color)(side ^ 1);
        attackers &= occupied;

        PieceType nextAttacker;
        Square nextSq = board.getSmallestAttacker(attackers, side, nextAttacker);

        if (nextSq == SQUARE_NONE) break;

        d++;

        gain[d] = Evaluation::GetPieceValue(pieceOnTarget) - gain[d - 1];

        if (std::max(-gain[d - 1], gain[d]) < 0) break;

        pieceOnTarget = nextAttacker;

        occupied ^= (1ULL << nextSq);

        sliderAttacks = (board.getBishopAttacks(to, occupied) & board.getBishopsQueens()) |
            (board.getRookAttacks(to, occupied) & board.getRooksQueens());
        attackers |= sliderAttacks;
    }

    while (d > 0) {
        d--;
        gain[d] = -std::max(-gain[d], gain[d + 1]);
    }

    return gain[0];
}

static inline int ScoreMove(
    const Board& board,
    const Move& m,
    const Move& ttMove,
    const int history[2][SQUARE_COUNT][SQUARE_COUNT],
    const Move killers[2]
) {

    if (m.isValid() && ttMove.isValid() && m == ttMove) {
        return 10'000'000;
    }

    Color us = board.getSideToMove();
    Color enemy = (Color)(us ^ 1);

    MoveFlag moveFlag = m.getFlags();

    int promotionFlag = moveFlag & 0b1011;

    if (moveFlag & CAPTURE_FLAG) {

        if (moveFlag & PROMOTION_FLAG) {
            switch (promotionFlag) {
            case PROMOTION_TYPE_QUEEN:
                return 9'000'000;
            case PROMOTION_TYPE_KNIGHT:
                return 8'000'000;
            default:
                return -2'000'000;
            }
        }

        PieceType victim =
            (moveFlag == EN_PASSANT)
            ? PAWN
            : board.getPieceAt(m.getTo(), enemy);

        int v = Evaluation::GetPieceValue(victim);
        int a = Evaluation::GetPieceValue(m.getPieceType());

        int mvv_lva = v * 16 - a;

        if (v >= a) return 5'000'000 + mvv_lva;
        else if (MoveOrdering::See(board, m) >= 0) return 5'000'000 + mvv_lva;
        else return -1'000'000 + mvv_lva;
    }

    if (moveFlag & PROMOTION_FLAG) {
        switch (promotionFlag) {
        case PROMOTION_TYPE_QUEEN:
            return 4'000'000;
        case PROMOTION_TYPE_KNIGHT:
            return 3'000'000;
        default:
            return -2'000'000;
        }
    }

    if (m.isValid()) {
        if (m == killers[0]) return 2'000'000;
        if (m == killers[1]) return 1'500'000;
    }

    Square mFrom = m.getFrom();
    Square mTo = m.getTo();

    int histScore = history[us][mFrom][mTo];
    return std::min(histScore, 999'999);
}

void MoveOrdering::SortMoves(
    const Board& board,
    MoveList& moves,
    Move ttMove,
    const int history[2][SQUARE_COUNT][SQUARE_COUNT],
    const Move killers[2]
) {
    int scores[256];

    int n = (int)moves.size();
    for (int i = 0; i < n; i++) {
        scores[i] = ScoreMove(
            board,
            moves[i],
            ttMove,
            history,
            killers
        );
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
}
