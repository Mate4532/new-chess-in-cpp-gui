#pragma once
#include <vector>
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
};