#pragma once
#include <string>
#include <map>
#include <vector>

class ResultManager {
public:
    enum GameOutcome {
        OLD_WIN,
        IMPROVED_WIN,
        DRAW
    };

    static void saveGameResult(GameOutcome outcome);
    static void resetStats();

private:

    static constexpr std::string DIR_PATH = "botvsbotresults";
    static constexpr std::string FILE_PATH = "botvsbot.txt";

    static std::map<std::string, int> readCurrentStats();
    static void writeStats(const std::map<std::string, int>& stats);
};
