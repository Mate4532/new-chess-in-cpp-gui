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

    uint64_t m_bitboards[2][PIECE_TYPE_COUNT] = { {0} };
    uint8_t piece_count[2][PIECE_TYPE_COUNT] = { {0} };
    uint64_t m_side_occupancy[2] = { 0 };
    uint64_t m_all_occupancy = 0;

    BoardState boardStateHistory[1024] = {};
    uint16_t m_ply = 0;
    uint16_t committedPly = 0;

    Color m_side_to_move = WHITE;

    std::string beginnerFen = newPosFen;
	RepetitionTable repetition_history;
    std::vector<Move> move_history;
    std::vector<MoveList> legalMovesHistory;
    std::vector<std::vector<std::vector<std::pair<PieceType, Color>>>> pieceHistory;
    std::vector<bool> checkHistory;
    std::vector<Color> playerToMoveHistory;

    GameResult gr = GameResult::GAME_DID_NOT_END;

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

    static uint64_t passed_pawn_mask[2][64];

    inline int GetSquare(int rank, int file) const {
        return rank * 8 + file;
    };

public:
	bool isDebugMode = true;

    Board();
    void InitializeBoard();
    void InitializeAttackTables();
    void InitializeMagicTables();
    void InizializePassedPawnTable();
    uint64_t GenerateFullHash() const;
    uint64_t SetOccupancy(int index, int bits_in_mask, uint64_t mask);
    uint64_t maskRook(int sq);
    uint64_t maskBishop(int sq);
    void LoadFEN(std::string fen);
    std::string GetFEN() const;
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
    inline int getFileFromSquare(Square sq) {
        return static_cast<int>(sq) & 7;
    }
    inline int getRankFromSquare(Square sq) {
        return static_cast<int>(sq) >> 3;
    }
    inline const uint64_t(&getBitboards() const)[2][PIECE_TYPE_COUNT]{
        return m_bitboards;
    }
    inline uint64_t getPieceBitboard(Color c, PieceType p) const {
        return m_bitboards[c][p];
    }
    inline Color getCommittedSideToMove(int ply = -1) const {
        return playerToMoveHistory[ply == -1 ? committedPly : ply];
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
    inline PieceType getCapturePieceType(int ply) const{
        return (PieceType)boardStateHistory[ply].captured_piece_type;
    }
    inline PieceType getLastCapturePieceType() const{
        return (PieceType)boardStateHistory[m_ply].captured_piece_type;
    }
    inline uint8_t getHalfMoveClock(int i = -1) const {
        return boardStateHistory[i == -1 ? m_ply : i].half_move_clock;
    }
    inline uint16_t getFullMoveNumber() const {
        return boardStateHistory[m_ply].full_move_number;
    }
    inline uint64_t getHash(int ply = -1) const {
        return boardStateHistory[ply == -1 ? m_ply : ply].zobrist_hash;
    }
    inline std::vector<uint64_t> getRepetitionHash() const {
        return repetition_history.getHashes();
	}
    inline const RepetitionTable& getRepetitionTable() const {
        return repetition_history;
    }
    inline uint16_t getPly() const {
        return committedPly;
    }
    inline MoveList getCurrentLegalMoves() const {
        return legalMovesHistory[committedPly];
    }
    inline Move getLastMove() {
        return move_history.empty() ? Move() : move_history.back();
    }
    inline std::vector<Move> getMoveHistory() {
        return move_history;
    }
    inline std::string getBeginnerFen() {
        return beginnerFen;
    }
    inline const Move getMove(int ply = -1){
        if (move_history.size() <= 0) {
            return Move();
        }
        int index = (ply == -1) ? static_cast<int>(move_history.size()) - 1 : ply;

        if (index < 0 || index >= static_cast<int>(move_history.size())) {
            return Move();
        }
        return move_history[index];
    }
    inline const bool wasMoveCheck(int ply = -1) {
        if (checkHistory.empty()) return false;
        if (ply == -1) return checkHistory.back();
        if (ply < 0) return false;
        return checkHistory[ply];
    }
    inline uint64_t getBishopsQueens() const {
        return m_bitboards[WHITE][BISHOP] | m_bitboards[BLACK][BISHOP] |
            m_bitboards[WHITE][QUEEN] | m_bitboards[BLACK][QUEEN];
    }

    inline uint64_t getRooksQueens() const {
        return m_bitboards[WHITE][ROOK] | m_bitboards[BLACK][ROOK] |
            m_bitboards[WHITE][QUEEN] | m_bitboards[BLACK][QUEEN];
    }

    bool HasNonPawnMaterial(Color color) const;
    bool hasPromotingPawn() const;
    bool hasAdvancedPawn() const;
    bool hasAdvancedPassedPawn(Color color) const;
    uint64_t getAttacksTo(Square sq, uint64_t occupied) const;
    Square getSmallestAttacker(uint64_t attackers, Color side, PieceType& attackerType) const;
    uint64_t getNewXRayAttacks(Square to, uint64_t occupied) const;
    bool IsInsufficientMaterial() const;
    bool IsStalemate();
	bool IsDraw();
    bool IsCheckMate();
    bool isSquareAttacked(Square sq, Color attackerColor) const;
    bool isPassedPawn(Color color, Square sq) const;
    bool isAdvancedPassedPawnPush(Move move) const;
    bool MakeMove(Move move, bool in_search = false);
    void UndoMove(Move move, bool in_search = false);
    void MakeNullMove();
    void UndoNullMove();
    uint64_t PerftDivide(int depth);
    uint64_t Perft(int depth);
    uint64_t MultiThreadedPerft(int depth);
    void PrintBoard(bool is_white_player = true, bool is_black_player = true) const;
    std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix(int ply = -1) const;
    void getPieceCounts(int piecesOut[2][6], int ply = -1);
    MoveFlag getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece);
    std::string convertMoveToSAN(int ply = -1, bool addPieceCharToString = true);
    std::vector<std::string>  getMoveHistroyInSAN();
    MoveList generateCurrentLegalMoves();
    void currentPlayerGaveUp();
    GameResult getGameResult();

    void ClearBoard();

    MoveInfo getMoveInfo(int ply = -1);
    static PieceType GetPromotionPiece(Move m);
    static PieceType GetPromotionPiece(MoveFlag promotion_piece);
};
