#include "Zobrist.h"

uint64_t Zobrist::pieceKeys[2][6][64];
uint64_t Zobrist::sideKey;
uint64_t Zobrist::castlingKeys[16];
uint64_t Zobrist::enPassantKeys[8];

void Zobrist::Init() {
    std::mt19937_64 rng(123456789);
    std::uniform_int_distribution<uint64_t> dist;

    for (int s = 0; s < 2; s++)
        for (int p = 0; p < 6; p++)
            for (int sq = 0; sq < 64; sq++)
                pieceKeys[s][p][sq] = dist(rng);

    sideKey = dist(rng);

    for (int i = 0; i < 16; i++)
        castlingKeys[i] = dist(rng);

    for (int i = 0; i < 8; i++)
        enPassantKeys[i] = dist(rng);
}