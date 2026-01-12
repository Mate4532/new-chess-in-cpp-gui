#include "ResultManager.h"
#include "filemanager.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

static std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r";
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) return "";
    const auto strEnd = str.find_last_not_of(whitespace);
    return str.substr(strBegin, strEnd - strBegin + 1);
}


void ResultManager::saveGameResult(GameOutcome outcome) {

    fs::path root = FileManager::getProjectRoot();
    fs::path dirPath = root / DIR_PATH;
    fs::path filePath = dirPath / FILE_PATH;

    if (!fs::exists(dirPath)) {
        try {
            fs::create_directories(dirPath);
            std::cout << "Mappa letrehozva: " << dirPath.string() << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "HIBA: Nem sikerult letrehozni a konyvtarat: " << e.what() << std::endl;
            return;
        }
    }

    std::map<std::string, int> stats = readCurrentStats();

    switch (outcome) {
    case OLD_WIN:
        stats["old_searcher_win"]++;
        break;
    case IMPROVED_WIN:
        stats["improved_searcher_win"]++;
        break;
    case DRAW:
        stats["draw"]++;
        break;
    }

    writeStats(stats);
}

std::map<std::string, int> ResultManager::readCurrentStats() {
    std::map<std::string, int> stats;
    stats["old_searcher_win"] = 0;
    stats["improved_searcher_win"] = 0;
    stats["draw"] = 0;

    fs::path root = FileManager::getProjectRoot();
    fs::path filePath = root / DIR_PATH / FILE_PATH;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return stats;
    }

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
    fs::path root = FileManager::getProjectRoot();
    fs::path filePath = root / DIR_PATH / FILE_PATH;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "HIBA: Nem sikerult irni a fajlba: " << filePath.string() << std::endl;
        return;
    }

    file << "old_searcher_win: " << stats.at("old_searcher_win") << "\n";
    file << "improved_searcher_win: " << stats.at("improved_searcher_win") << "\n";
    file << "draw: " << stats.at("draw") << "\n";
}

void ResultManager::resetStats() {
    std::map<std::string, int> zeroStats;
    zeroStats["old_searcher_win"] = 0;
    zeroStats["improved_searcher_win"] = 0;
    zeroStats["draw"] = 0;

    writeStats(zeroStats);

    std::cout << "Statisztikak sikeresen nullazva." << std::endl;
}
