#include "ResultManager.h"
#include "filemanager.h"
#include "versioncontrol.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

static std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r";
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) return "";
    const auto strEnd = str.find_last_not_of(whitespace);
    return str.substr(strBegin, strEnd - strBegin + 1);
}

ResultManager::ResultManager() {}

void ResultManager::ensureDirectoryExists() {
    fs::path dirPath = fullFilePath.parent_path();
    if (!fs::exists(dirPath)) {
        try {
            fs::create_directories(dirPath);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "ERROR: Failed to create directory: " << e.what() << std::endl;
        }
    }
}

void ResultManager::updatePath(const std::string& name1, const std::string& name2) {
    std::string n1 = name1;
    std::string n2 = name2;
    if (n1 > n2) std::swap(n1, n2);
    std::string folderName = n1 + "-" + n2;
    fs::path root = FileManager::getProjectRoot();
    fullFilePath = root / BASE_DIR / folderName / FILE_NAME;
}

void ResultManager::saveGameResult(const GameResult& gameResult,
                                   const std::string& whiteName, const std::string& whiteSourcePath,
                                   const std::string& blackName, const std::string& blackSourcePath,
                                   const std::vector<Move>& moveList, const std::string& startingFen) {

    if (gameResult == GAME_DID_NOT_END) return;

    updatePath(whiteName, blackName);
    fs::path pairRoot = fullFilePath.parent_path();

    fs::path fullWhiteSourcePath = FileManager::getProjectRoot() / fs::path(whiteSourcePath);
    fs::path fullBlackSourcePath = FileManager::getProjectRoot() / fs::path(blackSourcePath);

    VersionControl::manageBotVersion(whiteName, fullWhiteSourcePath.string());
    VersionControl::manageBotVersion(blackName, fullBlackSourcePath.string());

    std::string vNameWhite = VersionControl::getLatestVersionName(whiteName);
    std::string vNameBlack = VersionControl::getLatestVersionName(blackName);

    std::string v1 = vNameWhite;
    std::string v2 = vNameBlack;
    if (v1 > v2) std::swap(v1, v2);

    std::string versionPairDirName = v1 + "-" + v2;
    fs::path versionDir = pairRoot / versionPairDirName;

    if (!fs::exists(versionDir)) {
        fs::create_directories(versionDir);
    }

    fullFilePath = versionDir / FILE_NAME;

    std::map<std::string, int> stats = readCurrentStats(whiteName, blackName);

    if (gameResult == DRAW) {
        stats[DRAW_KEY]++;
    } else {
        bool whiteWon = (gameResult & WHITE_WON) != 0;
        std::string winnerName = whiteWon ? whiteName : blackName;
        stats[winnerName + " win"]++;
    }

    writeStats(stats);
    saveMatchMoves(gameResult, whiteName, blackName, moveList, startingFen);
}

void ResultManager::saveMatchMoves(const GameResult& gameResult, const std::string& whiteName, const std::string& blackName, const std::vector<Move>& moveList, const std::string& startingFen) {
    fs::path matchesBaseDir = fullFilePath.parent_path() / "Matches";

    fs::create_directories(matchesBaseDir / "Draws");
    fs::create_directories(matchesBaseDir / (whiteName + "_Wins"));
    fs::create_directories(matchesBaseDir / (blackName + "_Wins"));

    std::string subDir;
    if (gameResult == DRAW) {
        subDir = "Draws";
    } else if ((gameResult & WHITE_WON) != 0) {
        subDir = whiteName + "_Wins";
    } else {
        subDir = blackName + "_Wins";
    }

    fs::path targetDir = matchesBaseDir / subDir;

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".txt";
    fs::path matchFile = targetDir / oss.str();

    std::ofstream file(matchFile.string());
    if (!file.is_open()) return;

    file << "White: " << whiteName << "\n";
    file << "Black: " << blackName << "\n";
    file << "Result: " << (gameResult == DRAW ? "Draw" : ((gameResult & WHITE_WON) ? "White won" : "Black won")) << "\n";

    if (!startingFen.empty()) {
        file << "Starting FEN: " << startingFen << "\n";
    }

    file << "\nMoves:\n";
    for (size_t i = 0; i < moveList.size(); ++i) {
        if (i % 2 == 0) file << (i / 2 + 1) << ". ";
        file << moveList[i].toHumanReadable() << " ";
        if (i % 2 == 1) file << "\n";
    }

    file.close();
}

std::map<std::string, int> ResultManager::readCurrentStats(const std::string& name1, const std::string& name2) {
    std::map<std::string, int> stats;
    stats[name1 + " win"] = 0;
    stats[name2 + " win"] = 0;
    stats[DRAW_KEY] = 0;

    std::ifstream file(fullFilePath.string());
    if (!file.is_open()) return stats;

    std::string line;
    while (std::getline(file, line)) {
        size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos) {
            std::string key = trim(line.substr(0, delimiterPos));
            std::string valueStr = trim(line.substr(delimiterPos + 1));
            try {
                if (!valueStr.empty()) stats[key] = std::stoi(valueStr);
            } catch (...) {}
        }
    }
    return stats;
}

void ResultManager::writeStats(const std::map<std::string, int>& stats) {
    std::ofstream file(fullFilePath.string());
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to write file: " << fullFilePath.string() << std::endl;
        return;
    }

    for (const auto& [name, wins] : stats) {
        file << name << ": " << wins << "\n";
    }
    file.close();
}

void ResultManager::resetStats(const std::string& whiteName, const std::string& blackName) {
    updatePath(whiteName, blackName);

    std::map<std::string, int> zeroStats;
    zeroStats[whiteName + " win"] = 0;
    zeroStats[blackName + " win"] = 0;
    zeroStats[DRAW_KEY] = 0;

    writeStats(zeroStats);
    std::cout << "Statistics reset: " << whiteName << " vs " << blackName << std::endl;
}
