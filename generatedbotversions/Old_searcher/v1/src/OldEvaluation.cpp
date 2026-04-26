#include "OldEvaluation.h"
#include "nnue-probe-master/src/nnue.h"

using namespace OldEvaluation;

int Evaluation::GetPieceValue(PieceType p) {
    switch (p) {
    case PAWN:   return 100;
    case KNIGHT: return 300;
    case BISHOP: return 300;
    case ROOK:   return 500;
    case QUEEN:  return 900;
    case KING:   return 10000;
    default:     return 0;
    }
}

int Evaluation::GetNnuePieceNum(PieceType p, Color c) {

    switch(p){

    case KING:
        return (c == WHITE ? 1 : 7);

    case QUEEN:
        return (c == WHITE ? 2 : 8);

    case ROOK:
        return (c == WHITE ? 3 : 9);

    case BISHOP:
        return (c == WHITE ? 4 : 10);

    case KNIGHT:
        return (c == WHITE ? 5 : 11);

    case PAWN:
        return (c == WHITE ? 6 : 12);

    default:
        return -1;
    }

}
int Evaluation::EvaluatePos(const Board& board, int ply, NNUEdata* nnue_state) {
    int player = (board.getSideToMove() == WHITE) ? 0 : 1;

    NNUEdata* nnue_data_pointers[3];
    nnue_data_pointers[0] = &nnue_state[ply];
    nnue_data_pointers[1] = (ply >= 1) ? &nnue_state[ply - 1] : nullptr;
    nnue_data_pointers[2] = (ply >= 2) ? &nnue_state[ply - 2] : nullptr;

    int pieces[65];
    int squares[65];
    int index = 2;

    squares[0] = board.getKingSquare(WHITE);
    pieces[0] = 1;

    squares[1] = board.getKingSquare(BLACK);
    pieces[1] = 7;

    for (int c = WHITE; c <= BLACK; ++c) {
        Color color = (Color)c;

        for (int p = PAWN; p < KING; ++p) {
            PieceType pt = (PieceType)p;

            uint64_t bb = board.getPieceBitboard(color, pt);

            if (bb) {
                int nnue_piece_code = GetNnuePieceNum(pt, color);

                while (bb) {
                    int sq = PopBit(bb);

                    pieces[index] = nnue_piece_code;
                    squares[index] = sq;
                    index++;
                }
            }
        }
    }

    pieces[index] = 0;
    squares[index] = 0;

    return nnue_evaluate_incremental(player, pieces, squares, nnue_data_pointers);
}
