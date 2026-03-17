#include <cstdint>
#include "Utils.h"

struct BoardState {

    uint8_t castling_rights;
    uint8_t en_passant_sq;
    uint8_t captured_piece_type;
    uint8_t moved_piece_type;

    uint8_t half_move_clock;
    uint16_t full_move_number;
	uint64_t zobrist_hash;
};