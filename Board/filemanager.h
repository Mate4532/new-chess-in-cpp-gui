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
};

#endif // FILEMANAGER_H
