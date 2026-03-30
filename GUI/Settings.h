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
    std::string beginnerPosFEN = "";

    bool operator!=(const BoardSettings& other) const {
        return isBoardFlipped != other.isBoardFlipped;
    }
};

struct AllSettings {
    RobotSettings robotSettings;
    BoardSettings boardSettings;
};

#endif // SETTINGS_H
