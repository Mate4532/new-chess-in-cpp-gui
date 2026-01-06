#include "UCIParsing.h"

Move UCIParsing::Parse(const std::string& uci, const Board& board) {
    if (uci.length() < 4) return Move();

    int from_file = uci[0] - 'a';
    int from_rank = uci[1] - '1';
    int to_file = uci[2] - 'a';
    int to_rank = uci[3] - '1';

    Square from_sq = (Square)(from_rank * 8 + from_file);
    Square to_sq = (Square)(to_rank * 8 + to_file);

    Color us = board.getSideToMove();
    PieceType moving_piece = board.getPieceAt(from_sq, us);

    if (moving_piece == PIECE_NONE) return Move();

    MoveFlag flag = NORMAL_MOVE;
    Color enemy = (us == WHITE) ? BLACK : WHITE;

    if (board.getPieceAt(to_sq, enemy) != PIECE_NONE) {
        flag = CAPTURE;
    }

    if (moving_piece == PAWN && to_sq == board.getEnPassantSquare()) {
        flag = EN_PASSANT;
    }

    if (moving_piece == KING) {
        if (from_sq == E1) {
            if (to_sq == G1) flag = KINGSIDE_CASTLE;
            else if (to_sq == C1) flag = QUEENSIDE_CASTLE;
        }
        else if (from_sq == E8) {
            if (to_sq == G8) flag = KINGSIDE_CASTLE;
            else if (to_sq == C8) flag = QUEENSIDE_CASTLE;
        }
    }

    if (moving_piece == PAWN && std::abs(to_rank - from_rank) == 2) {
        flag = DOUBLE_PAWN_PUSH;
    }

    if (uci.length() == 5) {
        char prom = uci[4];
        MoveFlag prom_flag = PROMOTION_FLAG;

        if (prom == 'q') flag = (MoveFlag)(prom_flag | 0b0011);
        else if (prom == 'r') flag = (MoveFlag)(prom_flag | 0b0010);
        else if (prom == 'b') flag = (MoveFlag)(prom_flag | 0b0001);
        else if (prom == 'n') flag = (MoveFlag)(prom_flag | 0b0000);
        
        if (board.getPieceAt(to_sq, enemy) != PIECE_NONE) {
            flag = (MoveFlag)(flag | CAPTURE_FLAG);
        }
    }

    return Move(from_sq, to_sq, moving_piece, flag);
}
