#ifndef SETTINGS_H
#define SETTINGS_H

#include "Utils.h"

struct RobotSettings {
    bool isWhiteRobot = false;
    bool isBlackRobot = false;
    bool isBotVsBot = false;
    int botSearchTimeMs = 1000;
    int numThreads = 7;
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
    bool showLegalMoves = true;

    bool operator!=(const BoardSettings& other) const {
        return isBoardFlipped != other.isBoardFlipped ||
               beginnerPosFEN != other.beginnerPosFEN ||
               showLegalMoves != other.showLegalMoves;
    }
};

struct TimeSettings {
    GameMode gm = GameMode::UNLIMITED_THINKING_TIME;
    RobotTimeUsageMode rtum = RobotTimeUsageMode::FIXED_TIME;

    int whiteTimeSec = 5 * 60;
    int blackTimeSec = 5 * 60;
    double incrementSec = 5;

    bool operator!=(const TimeSettings& other) const {
        return gm != other.gm ||
               rtum != other.rtum ||
               whiteTimeSec != other.whiteTimeSec ||
               blackTimeSec != other.blackTimeSec ||
               incrementSec != other.incrementSec;
    }

    long long getWhiteTimeMs() const { return whiteTimeSec * 1000LL; }
    long long getBlackTimeMs() const { return blackTimeSec * 1000LL; }
    long long getIncrementMs() const { return (long long)(incrementSec * 1000.0); }
};

struct AllSettings {
    RobotSettings robotSettings;
    BoardSettings boardSettings;
    TimeSettings timeSettings;
};

#endif // SETTINGS_H
