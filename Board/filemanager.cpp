#include "filemanager.h"
#include "picosha2.h"
#include <vector>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;

fs::path FileManager::getExecutableDir() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) return fs::current_path();
    return fs::path(buffer).parent_path();
#else
    std::error_code ec;
    const fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::current_path() : exe.parent_path();
#endif
}

std::string FileManager::findNnueFile(const std::string& relativePath) {
    const fs::path inNnueDir = fs::path("nnues") / relativePath;

    const fs::path besideExe = getExecutableDir() / inNnueDir;
    if (fs::exists(besideExe)) return besideExe.string();

    const std::string found = findAssetPath(inNnueDir.string());
    if (fs::exists(found)) return found;

    return "";
}

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
