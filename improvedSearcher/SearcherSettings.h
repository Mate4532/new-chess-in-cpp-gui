#ifndef SEARCHERSETTINGS_H
#define SEARCHERSETTINGS_H

#include "Utils.h"

struct SearcherSettings {
    int maxDepth;
    bool worseEvaluationEnabled;
    bool areBlundersOnPurposeEnabled;
    int chanceToActivatePossBlunder;
    int topNmove;
    int blunderThreshold;

    static SearcherSettings getSettings(Difficulty diff) {
        switch (diff) {
        case Difficulty::EASY:
            return { 6, true, true, 40, 8, 900 };

        case Difficulty::MEDIUM:
            return { 6, true, true, 30, 6, 500 };

        case Difficulty::HARD:
            return { 9, true, true, 20, 4, 300 };

        case Difficulty::IMPOSSIBLE:
            return { 128, false, false, 0, 0, 0 };

        default:
            return { 3, true, true, 50, 5, 500 };
        }
    }
};

#endif // SEARCHERSETTINGS_H
