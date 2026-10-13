#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <filesystem>

class FileManager
{
public:
    FileManager();
    static std::string getProjectRoot();
    static std::string findAssetPath(const std::string& fileName);
    static std::filesystem::path getExecutableDir();
    static std::string findNnueFile(const std::string& relativePath);
};

#endif // FILEMANAGER_H
