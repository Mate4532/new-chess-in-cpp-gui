#pragma once
#include <cstdint>
#include "Utils.h"

struct Move {
private:
    uint16_t m_move_data;
    uint8_t m_piece_type;

public:
    Move() : m_move_data(0), m_piece_type(0) {}
    Move(Square from, Square to, PieceType piece_type, MoveFlag flags = NORMAL_MOVE) {
        m_move_data = (uint16_t)(flags << 12) | (from << 6) | to;
        m_piece_type = piece_type;
    }

	Move(uint16_t move_data, uint8_t piece_type) : m_move_data(move_data), m_piece_type(piece_type) {}

    inline Square getFrom() const {
        return (Square)((m_move_data >> 6) & 0x3F);
    }

    inline Square getTo() const {
        return (Square)(m_move_data & 0x3F);
    }

    inline MoveFlag getFlags() const {
        return (MoveFlag)(m_move_data >> 12);
    }

    inline PieceType getPieceType() const {
        return (PieceType)m_piece_type;
    }

    inline uint16_t getMoveData() const {
        return m_move_data;
	}

    std::string toAlgebraic() const {
        return square_to_coordinates[getFrom()] + square_to_coordinates[getTo()];
    }

    inline bool isValid() const {
        return m_move_data != 0;
	}

    PieceType getPromotionPieceType() const {

        if (!(getFlags() & PROMOTION_FLAG)) return PIECE_NONE;

        return static_cast<PieceType>((getFlags() & 0b0011) + 1);
    }

};