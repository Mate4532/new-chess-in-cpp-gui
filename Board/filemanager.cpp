#include "filemanager.h"
#include "picosha2.h"
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

std::string FileManager::getProjectRoot() {
    fs::path currentDir = fs::current_path();

    for (int i = 0; i < 5; ++i) {
        if (fs::exists(currentDir / "assets")) {
            return currentDir.string();
        }

        if (currentDir.has_parent_path()) {
            currentDir = currentDir.parent_path();
        } else {
            break;
        }
    }

    return fs::current_path().string();
}

std::string FileManager::findAssetPath(const std::string& fileName) {
    fs::path currentDir = fs::current_path();

    for (int i = 0; i < 5; ++i) {
        fs::path p = currentDir / fileName;

        if (fs::exists(p)) {
            return p.string();
        }

        if (currentDir.has_parent_path()) {
            currentDir = currentDir.parent_path();
        } else {
            break;
        }
    }

    return fileName;
}
