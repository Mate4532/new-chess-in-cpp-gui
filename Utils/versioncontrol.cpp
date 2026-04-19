#include "versioncontrol.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

std::string VersionControl::calculateFolderHash(const std::string& botPath) {
    if (!fs::exists(botPath)) {
        std::cerr << "[VersionControl] Folder not found: " << botPath << std::endl;
        return "";
    }

    std::string combinedHashes = "";
    std::vector<fs::path> files;

    for (const auto& entry : fs::recursive_directory_iterator(botPath)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const auto& path : files) {
        std::ifstream f(path.string(), std::ios::binary);
        if (f) {
            std::string fileHash;
            picosha2::hash256_hex_string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>(), fileHash);
            combinedHashes += fileHash;
        }
    }

    return picosha2::hash256_hex_string(combinedHashes);
}

void VersionControl::manageBotVersion(const std::string& botNameToSaveInFile, const std::string& botSourcePath) {
    fs::path projectRoot = FileManager::getProjectRoot();
    fs::path botBaseFolder = projectRoot / "generatedbotversions" / botNameToSaveInFile;
    fs::path fullSourcePath = projectRoot / fs::path(botSourcePath);

    if (!fs::exists(botBaseFolder)) fs::create_directories(botBaseFolder);

    std::string currentHash = calculateFolderHash(fullSourcePath.string());
    if (currentHash.empty()) return;

    int existingV = getCurrentVersion(botNameToSaveInFile, botSourcePath);

    if (existingV == 0) {
        int nextV = getLatestVersion(botNameToSaveInFile) + 1;
        fs::path newVersionDir = botBaseFolder / ("v" + std::to_string(nextV));

        try {
            fs::create_directories(newVersionDir);

            std::ofstream hFile(newVersionDir / "hash.txt");
            hFile << currentHash;
            hFile.close();

            fs::path sourceDest = newVersionDir / "src";
            fs::copy(fullSourcePath, sourceDest, fs::copy_options::recursive);

            std::cout << "[VersionControl] Unique code state detected. Saved as v" << nextV << " for " << botNameToSaveInFile << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[VersionControl] Error during version save: " << e.what() << std::endl;
        }
    } else {
        std::cout << "[VersionControl] Bot " << botNameToSaveInFile << " matches existing version: v" << existingV << std::endl;
    }
}

int VersionControl::getLatestVersion(const std::string& botName) {
    fs::path projectRoot = FileManager::getProjectRoot();
    fs::path botBaseFolder = projectRoot / "generatedbotversions" / botName;

    if (!fs::exists(botBaseFolder)) return 0;

    int maxV = 0;
    for (const auto& entry : fs::directory_iterator(botBaseFolder)) {
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();
            if (dirName.find("v") == 0) {
                try {
                    int v = std::stoi(dirName.substr(1));
                    if (v > maxV) maxV = v;
                } catch (...) {}
            }
        }
    }
    return maxV;
}

std::string VersionControl::getLatestVersionName(const std::string& botName) {
    int v = getLatestVersion(botName);
    return botName + "_v" + std::to_string(v);
}


int VersionControl::getCurrentVersion(const std::string& botName, const std::string& botSourcePath) {
    fs::path projectRoot = FileManager::getProjectRoot();
    fs::path botBaseFolder = projectRoot / "generatedbotversions" / botName;

    if (!fs::exists(botBaseFolder)) return 0;

    fs::path fullSourcePath = projectRoot / fs::path(botSourcePath);
    std::string currentSourceHash = calculateFolderHash(fullSourcePath.string());

    if (currentSourceHash.empty()) return 0;

    for (const auto& entry : fs::directory_iterator(botBaseFolder)) {
        if (entry.is_directory()) {
            fs::path hashFile = entry.path() / "hash.txt";
            if (fs::exists(hashFile)) {
                std::ifstream hFile(hashFile);
                std::string savedHash;
                std::getline(hFile, savedHash);

                if (currentSourceHash == savedHash) {
                    std::string dirName = entry.path().filename().string();
                    try {
                        return std::stoi(dirName.substr(1));
                    } catch (...) {}
                }
            }
        }
    }

    return 0;
}

std::string VersionControl::getCurrentVersionName(const std::string& botName, const std::string& botSourcePath) {
    int v = getCurrentVersion(botName, botSourcePath);

    if (v == 0) {
        return botName + "_vUNKNOWN_MODIFIED";
    }

    return botName + "_v" + std::to_string(v);
}

