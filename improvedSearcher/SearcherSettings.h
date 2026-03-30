#ifndef SEARCHERSETTINGS_H
#define SEARCHERSETTINGS_H

#include "Utils.h"

struct SearcherSettings {
    int maxDepth;
    bool worseEvaluationEnabled;
    bool areBlundersOnPurposeEnabled;
    int chanceToActivatePossBlunder;
    int topNmove;
    bool topNMoveOff;
    int blunderThreshold;
    int minNormalMovesAfterBlunder;
    bool preventEmbarrassingBlunders;
    bool takeFreePieces;

    static SearcherSettings getSettings(Difficulty diff) {
        switch (diff) {
        case Difficulty::EASY:
            return { 6, true, true, 40, 15, true, 900, 1, false, false };

        case Difficulty::MEDIUM:
            return { 9, true, true, 30, 10, false, 500, 2, true, true };

        case Difficulty::HARD:
            return { 9, true, true, 20, 8, false, 300, 2, true, true };

        case Difficulty::IMPOSSIBLE:
            return { 128, false, false, 0, 0, false, 0, 0, true, true };

        default:
            return { 128, false, false, 0, 0, false, 0, 0, true, true };
        }
    }
};

#endif // SEARCHERSETTINGS_H
