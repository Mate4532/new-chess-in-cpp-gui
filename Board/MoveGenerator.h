#pragma once
#include "Move.h"
#include "Board.h"
#include "MoveList.h"

class Board;

class MoveGenerator {
private:

    static inline void AddPawnMove(MoveList& moves, Square from_sq, Square to_sq, Color pawn_color, MoveFlag flag) {
        if (pawn_color == WHITE ? to_sq >= A8 : to_sq <= H1) {
            moves.push_back(Move(from_sq, to_sq, PAWN, (MoveFlag)(flag | PROMOTION_TYPE_KNIGHT)));
            moves.push_back(Move(from_sq, to_sq, PAWN, (MoveFlag)(flag | PROMOTION_TYPE_BISHOP)));
            moves.push_back(Move(from_sq, to_sq, PAWN, (MoveFlag)(flag | PROMOTION_TYPE_ROOK)));
            moves.push_back(Move(from_sq, to_sq, PAWN, (MoveFlag)(flag | PROMOTION_TYPE_QUEEN)));
        }
        else {
            moves.push_back(Move(from_sq, to_sq, PAWN, flag));
        }
    }


public:
    static void GenerateMoves(const Board& board, MoveList& moveList, bool genereate_only_captures = false);

    static constexpr int WHITE_ENPASSANT_PIECE_OFFSET = 8;
    static constexpr int BLACK_ENPASSANT_PIECE_OFFSET = -8;

    static constexpr Square WHITE_KINGSIDE_CASTLE_KING_POS = G1;
    static constexpr Square WHITE_QUEENSIDE_CASTLE_KING_POS = C1;
    static constexpr Square WHITE_KINGSIDE_CASTLE_ROOK_POS_FROM = H1;
    static constexpr Square WHITE_QUEENSIDE_CASTLE_ROOK_POS_FROM = A1;
    static constexpr Square WHITE_KINGSIDE_CASTLE_ROOK_POS_TO = F1;
    static constexpr Square WHITE_QUEENSIDE_CASTLE_ROOK_POS_TO = D1;

    static constexpr Square BLACK_KINGSIDE_CASTLE_KING_POS = G8;
    static constexpr Square BLACK_QUEENSIDE_CASTLE_KING_POS = C8;
    static constexpr Square BLACK_KINGSIDE_CASTLE_ROOK_POS_FROM = H8;
    static constexpr Square BLACK_QUEENSIDE_CASTLE_ROOK_POS_FROM = A8;
    static constexpr Square BLACK_KINGSIDE_CASTLE_ROOK_POS_TO = F8;
    static constexpr Square BLACK_QUEENSIDE_CASTLE_ROOK_POS_TO = D8;

};
