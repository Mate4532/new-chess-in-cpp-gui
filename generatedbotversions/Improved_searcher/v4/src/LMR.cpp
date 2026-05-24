#include "LMR.h"

using namespace ImprovedLMR;

int LMR::table[64][64];
int LMR::pvTable[64][64];

void LMR::Init() {
    for (int d = 0; d < 64; d++) {
        for (int m = 0; m < 64; m++) {
            if (d == 0 || m == 0) {
                table[d][m] = 0;
                continue;
            }
            double reduction = 0.75 + std::log(d) * std::log(m) / 2.3;
            table[d][m] = static_cast<int>(reduction);
        }
    }
    for (int d = 0; d < 64; d++) {
        for (int m = 0; m < 64; m++) {
            if (d == 0 || m == 0) {
                pvTable[d][m] = 0;
                continue;
            }
            double reduction = 0 + std::log(d) * std::log(m) / 2.5;
            pvTable[d][m] = static_cast<int>(reduction);
        }
    }
}

int LMR::GetReduction(int depth, int moveCount, bool isPvNode) {
    return isPvNode ? pvTable[std::min(depth, 63)][std::min(moveCount, 63)] : table[std::min(depth, 63)][std::min(moveCount, 63)];
}
