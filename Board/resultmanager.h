#pragma once
#include <string>
#include <map>
#include <filesystem>
#include "ISearcher.h"

class ResultManager {
public:
    ResultManager();

    void saveGameResult(const GameResult& gameResult, const std::string& whiteName,
                        const std::string whiteSourcePath, const std::string& blackName,
                        const std::string& blackSourcePath, const std::vector<std::string>& moveList,
                        const std::string& startingFen = "");
    void resetStats(const std::string& whiteName, const std::string& blackName);

private:
    std::filesystem::path fullFilePath;

    void updatePath(const std::string& name1, const std::string& name2);
    std::map<std::string, int> readCurrentStats(const std::string& whiteRobotName, const std::string& blackRobotName);
    void writeStats(const std::map<std::string, int>& stats);
    void ensureDirectoryExists();
    void saveMatch(const GameResult& gameResult, const std::string& whiteName, const std::string& blackName, const std::vector<std::string>& moveList, const std::string& startingFen);

    static constexpr const char* DRAW_KEY = "Draws";
    static constexpr const char* BASE_DIR = "botvsbotresults";
    static constexpr const char* FILE_NAME = "results.txt";
};
