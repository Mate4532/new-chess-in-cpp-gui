#ifndef SETTINGS_H
#define SETTINGS_H

#include "Utils.h"

struct RobotSettings {
    bool isWhiteRobot = false;
    bool isBlackRobot = false;
    Difficulty whiteRobotDifficulty = Difficulty::EASY;
    Difficulty blackRobotDifficulty = Difficulty::EASY;

    bool operator!=(const RobotSettings& other) const {
        return isWhiteRobot != other.isWhiteRobot ||
               isBlackRobot != other.isBlackRobot ||
               whiteRobotDifficulty != other.whiteRobotDifficulty ||
               blackRobotDifficulty != other.blackRobotDifficulty;
    }

    RobotSettings& operator=(const RobotSettings& other) {

        if (this == &other) {
            return *this;
        }

        isWhiteRobot = other.isWhiteRobot;
        isBlackRobot = other.isBlackRobot;
        whiteRobotDifficulty = other.whiteRobotDifficulty;
        blackRobotDifficulty = other.blackRobotDifficulty;
        return *this;
    }
};

struct BoardSettings {
    bool isBoardFlipped = false;

    bool operator!=(const BoardSettings& other) const {
        return isBoardFlipped != other.isBoardFlipped;
    }

    BoardSettings& operator=(const BoardSettings& other) {

        if (this == &other) {
            return *this;
        }

        isBoardFlipped = other.isBoardFlipped;
        return *this;
    }
};

struct AllSettings {
    RobotSettings robotSettings;
    BoardSettings boardSettings;
};

#endif // SETTINGS_H
