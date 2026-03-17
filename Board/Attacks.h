#pragma once
#include <cstdint>

namespace Attacks {
    extern uint64_t KingMasks[64];
    extern uint64_t KnightMasks[64];
    extern uint64_t PawnAttacks[2][64];

    void InitKingMasks();
    void InitAll();
}
