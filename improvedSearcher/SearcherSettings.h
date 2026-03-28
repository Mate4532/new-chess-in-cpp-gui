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

    static SearcherSettings getSettings(Difficulty diff) {
        switch (diff) {
        case Difficulty::EASY:
            return { 6, true, true, 40, 10, true, 900, 0, false };

        case Difficulty::MEDIUM:
            return { 6, true, true, 40, 8, true, 500, 1, true };

        case Difficulty::HARD:
            return { 9, true, true, 30, 6, true, 400, 1, true };

        case Difficulty::IMPOSSIBLE:
            return { 128, false, false, 0, 0, true, 0, 0, true };

        default:
            return { 128, false, false, 0, 0, true, 0, 0, true };
        }
    }
};

#endif // SEARCHERSETTINGS_H
