#ifndef VERSIONCONTROL_H
#define VERSIONCONTROL_H

#include "picosha2.h"
#include <vector>
#include <algorithm>
#include <filesystem>
#include "filemanager.h"

class VersionControl
{
public:
    static std::string calculateFolderHash(const std::string& botPath);
    static void manageBotVersion(const std::string& botName, const std::string& botSourcePath);
    static int getLatestVersion(const std::string& botName);
    static std::string getLatestVersionName(const std::string& botName);
};

#endif // VERSIONCONTROL_H
