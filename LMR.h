#include <cmath>
#include <algorithm>

class LMR {
public:
    static int table[64][64];

    static void Init();

    static int GetReduction(int depth, int moveCount);
};