#pragma once

#include <vector>
#include <algorithm>

namespace OldPED{

    class PrecomputedEvaluationData {
    public:
        static std::vector<int> PawnShieldSquaresWhite[64];
        static std::vector<int> PawnShieldSquaresBlack[64];

        static void Init();
    };
}
