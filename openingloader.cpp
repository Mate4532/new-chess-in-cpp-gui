#include "openingloader.h"

#include <fstream>
#include <iostream>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <filesystem>

std::vector<std::string> OpeningLoader::openings;

static std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r";
    const auto strBegin = str.find_first_not_of(whitespace);

    if (strBegin == std::string::npos)
        return "";

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

void OpeningLoader::loadOpenings(const std::string& relativePath)
{
    openings.clear();
    std::unordered_set<std::string> seen;

    std::string fullPath = FileManager::findAssetPath(relativePath);
    std::ifstream file(fullPath);

    if (!file.is_open()) {
        std::cout << "HIBA: Nem sikerult megnyitni az opening fajlt: " << fullPath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmedLine = trim(line);

        if (trimmedLine.length() >= 4 && trimmedLine.substr(0, 4) == "pos ") {

            std::string fen = trimmedLine.substr(4);

            if (seen.insert(fen).second) {
                openings.push_back(fen);
            }
        }
    }

    file.close();

    std::cout << "Sikeresen betoltve " << openings.size()
              << " db opening FEN." << std::endl;
}

std::string OpeningLoader::getRandomFen() {
    if (openings.empty()) {
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, openings.size() - 1);

    return openings[dist(rng)];
}

bool OpeningLoader::hasOpenings() {
    return !openings.empty();
}
