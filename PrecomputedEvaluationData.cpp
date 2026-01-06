#include "PrecomputedEvaluationData.h"

std::vector<int> PrecomputedEvaluationData::PawnShieldSquaresWhite[64];
std::vector<int> PrecomputedEvaluationData::PawnShieldSquaresBlack[64];

void PrecomputedEvaluationData::Init() {
    for (int sq = 0; sq < 64; sq++) {
        int rank = sq / 8;
        int file = sq % 8;
        int shieldFile = std::max(1, std::min(file, 6));

        auto addIfValid = [](int f, int r, std::vector<int>& list) {
            if (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
                list.push_back(r * 8 + f);
            }
            };

        for (int fOff = -1; fOff <= 1; fOff++) addIfValid(shieldFile + fOff, rank + 1, PawnShieldSquaresWhite[sq]);
        for (int fOff = -1; fOff <= 1; fOff++) addIfValid(shieldFile + fOff, rank + 2, PawnShieldSquaresWhite[sq]);

        for (int fOff = -1; fOff <= 1; fOff++) addIfValid(shieldFile + fOff, rank - 1, PawnShieldSquaresBlack[sq]);
        for (int fOff = -1; fOff <= 1; fOff++) addIfValid(shieldFile + fOff, rank - 2, PawnShieldSquaresBlack[sq]);
    }
}