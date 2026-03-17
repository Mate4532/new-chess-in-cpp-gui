#include "Attacks.h"
#include "Utils.h"

namespace Attacks {
    uint64_t KingMasks[64];
    uint64_t KnightMasks[64];
    uint64_t PawnAttacks[2][64];

    void InitKingMasks() {
        for (int s = 0; s < 64; s++) {
            uint64_t b = 1ULL << s;
            uint64_t v = 0;
            v |= (b << 8);
            v |= (b >> 8);
            v |= (b << 1) & ~FILE_A;
            v |= (b >> 1) & ~FILE_H;
            v |= (b << 9) & ~FILE_A;
            v |= (b << 7) & ~FILE_H;
            v |= (b >> 7) & ~FILE_A;
            v |= (b >> 9) & ~FILE_H;
            KingMasks[s] = v;
        }
    }

    void InitAll() {
        InitKingMasks();
    }
}