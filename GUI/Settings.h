#ifndef SETTINGS_H
#define SETTINGS_H

#include "Utils.h"

struct RobotSettings {
    bool isWhiteRobot = false;
    bool isBlackRobot = false;
    bool isBotVsBot = false;
    int botSearchTimeMs = 1000;
    Difficulty whiteRobotDifficulty = Difficulty::EASY;
    Difficulty blackRobotDifficulty = Difficulty::EASY;

    bool operator!=(const RobotSettings& other) const {
        return isWhiteRobot != other.isWhiteRobot ||
               isBlackRobot != other.isBlackRobot ||
               whiteRobotDifficulty != other.whiteRobotDifficulty ||
               blackRobotDifficulty != other.blackRobotDifficulty ||
               isBotVsBot != other.isBotVsBot ||
               botSearchTimeMs != other.botSearchTimeMs;
    }
};

struct BoardSettings {
    bool isBoardFlipped = false;
    std::string beginnerPosFEN = newPosFen;

    bool operator!=(const BoardSettings& other) const {
        return isBoardFlipped != other.isBoardFlipped ||
                beginnerPosFEN != other.beginnerPosFEN;
    }
};

struct TimeSettings {
    GameMode gm = GameMode::UNLIMITED_THINKING_TIME;
    RobotTimeUsageMode rtum = RobotTimeUsageMode::FIXED_TIME;
    int tournamentTimeMin = 5;
    int incrementSec = 0;

    bool operator!=(const TimeSettings& other) const {
        return gm != other.gm ||
               rtum != other.rtum ||
               tournamentTimeMin != other.tournamentTimeMin ||
               incrementSec != other.incrementSec;
    }

    long long getTournementTimeMs() { return tournamentTimeMin * 60000LL; }
    long long getIncrementMs() { return incrementSec * 1000LL; }
};

struct AllSettings {
    RobotSettings robotSettings;
    BoardSettings boardSettings;
    TimeSettings timeSettings;
};

#endif // SETTINGS_H
