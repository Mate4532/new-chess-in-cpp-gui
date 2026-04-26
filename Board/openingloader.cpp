#include "openingloader.h"

#include <fstream>
#include <iostream>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <filesystem>
#include <Utils.h>

std::vector<std::string> OpeningLoader::openings;
std::mutex OpeningLoader::openingMutex;

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
    std::lock_guard<std::mutex> lock(openingMutex);
    openings.clear();
    std::unordered_set<std::string> seen;

    std::string fullPath = FileManager::findAssetPath(relativePath);
    std::ifstream file(fullPath);

    if (!file.is_open()) {
        LOG_DEBUG("Nem sikerult megnyitni az opening fajlt: " << fullPath);
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
    LOG_DEBUG("Sikeresen betoltve " << openings.size()
        << " db opening FEN.");
}

std::string OpeningLoader::getRandomFen() {
    std::lock_guard<std::mutex> lock(openingMutex);
    if (openings.empty()) {
        return newPosFen;
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, openings.size() - 1);

    return openings[dist(rng)];
}

bool OpeningLoader::hasOpenings() {
    std::lock_guard<std::mutex> lock(openingMutex);
    return !openings.empty();
}

std::string OpeningLoader::popRandomFen() {
    std::lock_guard<std::mutex> lock(openingMutex);
    if (openings.empty()) {
        return newPosFen;
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, openings.size() - 1);

    size_t index = dist(rng);
    std::string selectedFen = openings[index];

    if (index != openings.size() - 1) {
        openings[index] = std::move(openings.back());
    }
    openings.pop_back();

    return selectedFen;
}
