#pragma once
#include <cstdint>
#include <string>

#define MAX_PLY 2048

const std::string newPosFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum GameResult : uint8_t {
    WHITE_WON_ON_TIME        = 0b00000001,
    WHITE_WON_WITH_CHECKMATE = 0b00000010,
    WHITE_GAVE_UP            = 0b00000100,
    BLACK_WON_ON_TIME        = 0b00001000,
    BLACK_WON_WITH_CHECKMATE = 0b00010000,
    BLACK_GAVE_UP            = 0b00100000,
    DRAW                     = 0b01000000,
    GAME_DID_NOT_END         = 0b10000000,

    WHITE_WON = WHITE_WON_ON_TIME | WHITE_WON_WITH_CHECKMATE | BLACK_GAVE_UP,

    BLACK_WON = BLACK_WON_ON_TIME | BLACK_WON_WITH_CHECKMATE | WHITE_GAVE_UP
};

enum Color : uint8_t {
    WHITE = 0,
    BLACK = 1,
    COLORS = 2,
    COLOR_NONE = 3
};

enum PieceType : uint8_t {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_COUNT, PIECE_NONE
};

enum Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQUARE_COUNT = 64,
    SQUARE_NONE
};

const char piece_chars[2][7] = {
    {'P', 'N', 'B', 'R', 'Q', 'K', '.'},
    {'p', 'n', 'b', 'r', 'q', 'k', '.'}
};

enum CastlingRight : uint8_t {
    WHITE_KINGSIDE_CASTLE = 0b0001,
    WHITE_QUEENSIDE_CASTLE = 0b0010,
    BLACK_KINGSIDE_CASTLE = 0b0100,
    BLACK_QUEENSIDE_CASTLE = 0b1000,
    ALL_CASTLING_RIGHTS = WHITE_KINGSIDE_CASTLE | WHITE_QUEENSIDE_CASTLE | BLACK_KINGSIDE_CASTLE | BLACK_QUEENSIDE_CASTLE,
    CASTLING_RIGHTS_NONE = 0b0000,
    WHITE_ALL_CASTLE_RIGHTS = WHITE_KINGSIDE_CASTLE | WHITE_QUEENSIDE_CASTLE,
    BLACK_ALL_CASTLE_RIGHTS = BLACK_KINGSIDE_CASTLE | BLACK_QUEENSIDE_CASTLE
};

enum MoveFlag : uint8_t {
    NORMAL_MOVE = 0,

    CAPTURE_FLAG = 0b0100,
    PROMOTION_FLAG = 0b1000,

    DOUBLE_PAWN_PUSH = 0b0001,
    KINGSIDE_CASTLE = 0b0010,
    QUEENSIDE_CASTLE = 0b0011,

    CAPTURE = CAPTURE_FLAG,
    EN_PASSANT = CAPTURE_FLAG | 0b0001,

    PROMOTION_TYPE_KNIGHT = 0b0000 | PROMOTION_FLAG,
    PROMOTION_TYPE_BISHOP = 0b0001 | PROMOTION_FLAG,
    PROMOTION_TYPE_ROOK = 0b0010 | PROMOTION_FLAG,
    PROMOTION_TYPE_QUEEN = 0b0011 | PROMOTION_FLAG,
};

inline Square GetLSB(uint64_t bb) {
#ifdef _MSC_VER
    unsigned long index;
    _BitScanForward64(&index, bb);
    return (Square)index;
#else
    return (Square)__builtin_ctzll(bb);
#endif
}

inline Square PopBit(uint64_t& bb) {
    Square sq = GetLSB(bb);
    bb &= bb - 1;
    return sq;
}

const uint64_t RANK_1 = 0x00000000000000FFULL;
const uint64_t RANK_2 = 0x000000000000FF00ULL;
const uint64_t RANK_3 = 0x0000000000FF0000ULL;
const uint64_t RANK_4 = 0x00000000FF000000ULL;
const uint64_t RANK_5 = 0x000000FF00000000ULL;
const uint64_t RANK_6 = 0x0000FF0000000000ULL;
const uint64_t RANK_7 = 0x00FF000000000000ULL;
const uint64_t RANK_8 = 0xFF00000000000000ULL;

const uint64_t RANK_MASKS[8] = {
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8
};

const uint64_t FILE_A = 0x0101010101010101ULL;
const uint64_t FILE_B = 0x0202020202020202ULL;
const uint64_t FILE_C = 0x0404040404040404ULL;
const uint64_t FILE_D = 0x0808080808080808ULL;
const uint64_t FILE_E = 0x1010101010101010ULL;
const uint64_t FILE_F = 0x2020202020202020ULL;
const uint64_t FILE_G = 0x4040404040404040ULL;
const uint64_t FILE_H = 0x8080808080808080ULL;

const uint64_t FILE_MASKS[8] = {
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H
};

const uint64_t LIGHT_SQUARES = 0x55AA55AA55AA55AAULL;
const uint64_t DARK_SQUARES  = 0xAA55AA55AA55AA55ULL;

const int rook_directions[4] = { 8, -8, 1, -1 };
const int bishop_directions[4] = { 7, 9, -7, -9 };

const std::string square_to_coordinates[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

const uint8_t castling_mask[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7,  15, 15, 15,  3, 15, 15, 11
};

enum class Difficulty {
    EASY,
    MEDIUM,
    HARD,
    IMPOSSIBLE
};

enum GameMode {
    UNLIMITED_THINKING_TIME,
    TOURNAMENT_MODE
};

enum RobotTimeUsageMode {
    FIXED_TIME, TOURNEMENT_TIME
};

const int basePieceCounts[PieceType::PIECE_TYPE_COUNT] = { 8, 2, 2, 2, 1, 1 };

struct MoveInfo {
    int fromFile = -1, fromRank = -1;
    int toFile = -1, toRank = -1;
    bool isValid() { return !(fromFile == -1 || fromRank == -1 || toFile == -1 || toRank == -1); }
};
