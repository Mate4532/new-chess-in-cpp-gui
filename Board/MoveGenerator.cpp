#include <vector>
#include "Board.h"
#include "Move.h"

PieceType MoveGenerator::GetPromotionPiece(MoveFlag promotion_piece) {
    switch (promotion_piece) {
    case PROMOTION_TYPE_KNIGHT:
        return KNIGHT;
        break;

    case PROMOTION_TYPE_BISHOP:
        return BISHOP;
        break;

    case PROMOTION_TYPE_ROOK:
        return ROOK;
        break;

    case PROMOTION_TYPE_QUEEN:
        return QUEEN;

    default:
        return PIECE_NONE;
        break;
    }
}

void MoveGenerator::GenerateMoves(const Board& board, MoveList& moveList, bool generate_only_captures) {

    Color player = board.getSideToMove();
    Color enemy = (player == WHITE) ? BLACK : WHITE;

    uint64_t all_occ = board.getAllOccupancy();
    uint64_t enemy_occ = board.getSideOccupancy(enemy);
    uint64_t friendly_occ = board.getSideOccupancy(player);

    for (int p = PAWN; p <= KING; p++) {
        PieceType piece_type = (PieceType)p;
        uint64_t piece_bb = board.getPieceBitboard(player, piece_type);

        while (piece_bb) {
            Square from_sq = PopBit(piece_bb);

            switch (piece_type) {
            case PAWN: {
                int direction = (player == WHITE) ? WHITE_ENPASSANT_PIECE_OFFSET : BLACK_ENPASSANT_PIECE_OFFSET;
                Square to_sq = (Square)(from_sq + direction);

                if (!generate_only_captures) {
                    if (!((1ULL << to_sq) & all_occ)) {
                        AddPawnMove(moveList, from_sq, to_sq, player, NORMAL_MOVE);

                        if ((player == WHITE && from_sq / 8 == 1) || (player == BLACK && from_sq / 8 == 6)) {
                            Square double_to = (Square)(to_sq + direction);
                            if (!((1ULL << double_to) & all_occ)) {
                                moveList.push_back(Move(from_sq, double_to, PAWN, DOUBLE_PAWN_PUSH));
                            }
                        }
                    }
                }

                uint64_t attack_mask = board.getInvertedPawnAttacks(from_sq, enemy);
                uint64_t captures = attack_mask & enemy_occ;
                while (captures) {
                    Square cap_to = PopBit(captures);
                    AddPawnMove(moveList, from_sq, cap_to, player, CAPTURE);
                }

                Square ep_sq = board.getEnPassantSquare();
                if (ep_sq != SQUARE_NONE && (attack_mask & (1ULL << ep_sq))) {
                    moveList.push_back(Move(from_sq, ep_sq, PAWN, EN_PASSANT));
                }
                break;
            }

            case KNIGHT:
            case BISHOP:
            case ROOK:
            case QUEEN:
            case KING: {
                uint64_t moves;
                if (piece_type == KNIGHT) moves = board.getKnightAttacks(from_sq);
                else if (piece_type == KING) moves = board.getKingAttacks(from_sq);
                else if (piece_type == BISHOP) moves = board.getBishopAttacks(from_sq, all_occ);
                else if (piece_type == ROOK) moves = board.getRookAttacks(from_sq, all_occ);
                else moves = board.getRookAttacks(from_sq, all_occ) | board.getBishopAttacks(from_sq, all_occ);

                if (generate_only_captures) {
                    moves &= enemy_occ;
                    while (moves) {
                        Square to_sq = PopBit(moves);
                        moveList.push_back(Move(from_sq, to_sq, piece_type, CAPTURE));
                    }
                }
                else {
                    moves &= ~friendly_occ;
                    while (moves) {
                        Square to_sq = PopBit(moves);
                        MoveFlag flag = ((1ULL << to_sq) & enemy_occ) ? CAPTURE : NORMAL_MOVE;
                        moveList.push_back(Move(from_sq, to_sq, piece_type, flag));
                    }
                }

                if (!generate_only_captures && piece_type == KING) {
                    CastlingRight rights = board.getCastlingRights();
                    if (!board.isSquareAttacked(from_sq, enemy)) {
                        if (player == WHITE) {
                            if ((rights & WHITE_KINGSIDE_CASTLE) && !(all_occ & 0x60ULL)) {
                                if (!board.isSquareAttacked(F1, BLACK) && !board.isSquareAttacked(G1, BLACK))
                                    moveList.push_back(Move(E1, G1, KING, KINGSIDE_CASTLE));
                            }
                            if ((rights & WHITE_QUEENSIDE_CASTLE) && !(all_occ & 0x0EULL)) {
                                if (!board.isSquareAttacked(D1, BLACK) && !board.isSquareAttacked(C1, BLACK))
                                    moveList.push_back(Move(E1, C1, KING, QUEENSIDE_CASTLE));
                            }
                        }
                        else {
                            if ((rights & BLACK_KINGSIDE_CASTLE) && !(all_occ & 0x6000000000000000ULL)) {
                                if (!board.isSquareAttacked(F8, WHITE) && !board.isSquareAttacked(G8, WHITE))
                                    moveList.push_back(Move(E8, G8, KING, KINGSIDE_CASTLE));
                            }
                            if ((rights & BLACK_QUEENSIDE_CASTLE) && !(all_occ & 0x0E00000000000000ULL)) {
                                if (!board.isSquareAttacked(D8, WHITE) && !board.isSquareAttacked(C8, WHITE))
                                    moveList.push_back(Move(E8, C8, KING, QUEENSIDE_CASTLE));
                            }
                        }
                    }
                }
                break;
            }
            }
        }
    }
}
