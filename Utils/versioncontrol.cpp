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

void VersionControl::manageBotVersion(const std::string& botName, const std::string& botSourcePath) {
    fs::path projectRoot = FileManager::getProjectRoot();
    fs::path botBaseFolder = projectRoot / "generatedbotversions" / botName;

    if (!fs::exists(botBaseFolder)) fs::create_directories(botBaseFolder);

    std::string currentHash = calculateFolderHash(botSourcePath);
    if (currentHash.empty()) return;

    int maxV = 0;
    std::string lastHash = "";

    for (const auto& entry : fs::directory_iterator(botBaseFolder)) {
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();
            std::string prefix = "v";
            if (dirName.find(prefix) == 0) {
                try {
                    int v = std::stoi(dirName.substr(prefix.length()));
                    if (v > maxV) {
                        maxV = v;
                        std::ifstream hFile(entry.path() / "hash.txt");
                        std::getline(hFile, lastHash);
                    }
                } catch (...) {}
            }
        }
    }

    if (currentHash != lastHash) {
        int nextV = maxV + 1;
        fs::path newVersionDir = botBaseFolder / ("v" + std::to_string(nextV));

        try {
            fs::create_directories(newVersionDir);
            std::ofstream hFile(newVersionDir / "hash.txt");
            hFile << currentHash;
            hFile.close();

            fs::path sourceDest = newVersionDir / "src";
            if (fs::exists(sourceDest)) fs::remove_all(sourceDest);
            fs::create_directories(sourceDest);

            for (const auto& entry : fs::directory_iterator(botSourcePath)) {
                fs::path currentPath = entry.path();
                fs::path destPath = sourceDest / currentPath.filename();
                if (fs::is_directory(currentPath)) {
                    fs::copy(currentPath, destPath, fs::copy_options::recursive);
                } else {
                    fs::copy_file(currentPath, destPath, fs::copy_options::overwrite_existing);
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[VersionControl] Backup failed: " << e.what() << std::endl;
        }
        std::cout << "[VersionControl] New version for " << botName << " saved in: " << botBaseFolder.string() << "\\v" << nextV << std::endl;
    }
}

int VersionControl::getLatestVersion(const std::string& botName) {
    fs::path projectRoot = FileManager::getProjectRoot();
    fs::path botBaseFolder = projectRoot / "generatedbotversions" / botName;

    if (!fs::exists(botBaseFolder)) return 1;

    int maxV = 1;
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
