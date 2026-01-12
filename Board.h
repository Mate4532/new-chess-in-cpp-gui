#pragma once
#include "Utils.h"
#include <cstring>
#include <cstdint>
#include "BoardState.h"
#include "Move.h"
#include <memory>
#include <vector>
#include <sstream>
#include <atomic>
#include "MoveList.h"
#include <iostream>
#include "MoveGenerator.h"
#include <chrono>
#include <thread>
#include "RepetitionTable.h"

struct Magic {
    uint64_t mask;
    uint64_t magic;
    int shift;
};

class Board {
private:
    const std::string newPosFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    uint64_t m_bitboards[2][PIECE_TYPE_COUNT] = { {0} };
    uint8_t piece_count[2][PIECE_TYPE_COUNT] = { {0} };
    uint64_t m_side_occupancy[2] = { 0 };
    uint64_t m_all_occupancy = 0;

    BoardState boardStateHistory[1024] = {};
    uint16_t m_ply = 0;

    Color m_side_to_move = WHITE;

	RepetitionTable repetition_history;
    std::vector<Move> move_history;

    static uint64_t pawn_attacks_table[2][64];
    static uint64_t knight_attacks_table[64];
    static uint64_t king_attacks_table[64];

    static uint64_t rook_table[64][4096];
    static uint64_t bishop_table[64][512];

    static Magic rook_magics[64];
    static Magic bishop_magics[64];

    static uint64_t raw_rook_magics[];
    static int raw_rook_shifts[];
    static uint64_t raw_bishop_magics[];
    static int raw_bishop_shifts[];

    inline int GetSquare(int rank, int file) const {
        return rank * 8 + file;
    };

public:
	bool isDebugMode = true;

    Board();
    void InitializeBoard();
    void InitializeAttackTables();
    void InitializeMagicTables();
    uint64_t GenerateFullHash() const;
    uint64_t SetOccupancy(int index, int bits_in_mask, uint64_t mask);
    uint64_t maskRook(int sq);
    uint64_t maskBishop(int sq);
    void LoadFEN(std::string fen);
    void loadNewGame();
    PieceType getPieceAt(Square sq, Color color) const;
    uint64_t getBishopAttacksSlow(Square sq, uint64_t occupied) const;
    uint64_t getBishopAttacks(Square sq, uint64_t occupied) const;
    uint64_t getKnightAttacks(Square sq) const;
    uint64_t getRookAttacksSlow(Square sq, uint64_t occupied) const;
    uint64_t getRookAttacks(Square sq, uint64_t occupied) const;
    uint64_t getKingAttacks(Square sq) const;
    uint64_t getInvertedPawnAttacks(Square sq, Color attackerColor) const;
    uint64_t getPawnAttacks(Square sq, Color attackerColor) const;
    inline const uint64_t(&getBitboards() const)[2][PIECE_TYPE_COUNT]{
        return m_bitboards;
    }
    inline uint64_t getPieceBitboard(Color c, PieceType p) const {
        return m_bitboards[c][p];
    }
    inline Color getSideToMove() const {
        return m_side_to_move;
    }
    inline Square getKingSquare(Color c) const {
        return (Square)GetLSB(m_bitboards[c][KING]);
    }
    inline uint64_t getSideOccupancy(Color c) const {
        return m_side_occupancy[c];
    }
    inline uint64_t getAllOccupancy() const {
        return m_all_occupancy;
    }
    inline CastlingRight getCastlingRights() const{
        return (CastlingRight)boardStateHistory[m_ply].castling_rights;
    }
    inline Square getEnPassantSquare() const {
        return (Square)boardStateHistory[m_ply].en_passant_sq;
    }
    inline PieceType getCapturePieceType() const{
        return (PieceType)boardStateHistory[m_ply].captured_piece_type;
    }
    inline uint8_t getHalfMoveClock(int i = -1) const {
        return boardStateHistory[i == -1 ? m_ply : i].half_move_clock;
    }
    inline uint16_t getFullMoveNumber() const {
        return boardStateHistory[m_ply].full_move_number;
    }
    inline uint64_t getHash(int i = -1) const {
        return boardStateHistory[i == -1 ? m_ply : i].zobrist_hash;
    }
    inline std::vector<uint64_t> getRepetitionHash() const {
        return repetition_history.getHashes();
	}
    inline uint16_t getPly() const {
        return m_ply;
	}
    inline const Move& getLastMove() {
		return move_history[move_history.size() - 1];
    }
    bool HasNonPawnMaterial(Color color) const;
    uint64_t getAttacksTo(Square sq, uint64_t occupied) const;
    Square getSmallestAttacker(uint64_t attackers, Color side, PieceType& attackerType) const;
    uint64_t getNewXRayAttacks(Square to, uint64_t occupied) const;
    bool IsInsufficientMaterial() const;
    bool IsStalemate();
	bool IsDraw();
    bool IsCheckMate();
    bool isSquareAttacked(Square sq, Color attackerColor) const;
    bool MakeMove(Move move, bool in_search = false);
    void UndoMove(Move move, bool in_search = false);
    void MakeNullMove();
    void UndoNullMove();
    uint64_t PerftDivide(int depth);
    uint64_t Perft(int depth);
    uint64_t MultiThreadedPerft(int depth);
    void PrintBoard(bool is_white_player = true, bool is_black_player = true) const;
    std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const;

    void ClearBoard();
};
