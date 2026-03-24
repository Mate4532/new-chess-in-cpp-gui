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
            return { 3, true, true, 60, 10, 900 };

        case Difficulty::MEDIUM:
            return { 6, true, true, 40, 5, 400 };

        case Difficulty::HARD:
            return { 9, true, true, 20, 3, 200 };

        case Difficulty::IMPOSSIBLE:
            return { 128, false, false, 0, 0, 0 };

        default:
            return { 3, true, true, 50, 5, 500 };
        }
    }
};

#endif // SEARCHERSETTINGS_H
