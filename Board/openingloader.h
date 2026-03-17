#ifndef OPENINGLOADER_H
#define OPENINGLOADER_H

#include "filemanager.h"

#include <vector>
#include <string>

class OpeningLoader {
public:
    static std::vector<std::string> openings;

    static void loadOpenings(const std::string& filePath);

    static std::string getRandomFen();
    static bool hasOpenings();
};

#endif // OPENINGLOADER_H
