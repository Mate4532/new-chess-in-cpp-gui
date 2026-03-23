#pragma once
#include "Move.h"
#include <array>
#include <cstddef>
#include <iostream>

struct MoveList {
    Move moves[256];
    int count = 0;

    void push_back(const Move& move) {
        moves[count++] = move;
    }

    Move& operator[](size_t index) {
        return moves[index];
    }

    const Move& operator[](size_t index) const {
        return moves[index];
    }

    bool contains(const Move& move) const {
        for (int i = 0; i < count; i++) {
            if (moves[i].getFrom() == move.getFrom() && moves[i].getTo() == move.getTo()) {
                return true;
            }
        }
        return false;
	}

    Move* begin() { return moves; }
    Move* end() { return moves + count; }
    size_t size() const { return (size_t)count; }
};
