#pragma once

#include "Utils.h"
#include <random>
#include "Board.h"

class Zobrist {
public:
    static uint64_t pieceKeys[2][6][64];
    static uint64_t sideKey;
    static uint64_t castlingKeys[16];
    static uint64_t enPassantKeys[8];

    static void Init();
};