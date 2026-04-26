#ifndef OPENINGLOADER_H
#define OPENINGLOADER_H

#include "filemanager.h"

#include <vector>
#include <string>
#include <mutex>

class OpeningLoader {
public:
    static std::vector<std::string> openings;
    static std::mutex openingMutex;

    static void loadOpenings(const std::string& filePath);

    static std::string getRandomFen();
    static std::string popRandomFen();
    static bool hasOpenings();
};

#endif // OPENINGLOADER_H
