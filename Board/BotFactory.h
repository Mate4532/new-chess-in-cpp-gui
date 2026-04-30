#pragma once
#include <memory>
#include "ISearcher.h"
#include "Settings.h"

enum class SearcherType { OLD_SEARCHER, IMPROVED_SEARCHER };

class BotFactory {
public:
    static std::unique_ptr<ISearcher> createBot(SearcherType st, RobotSettings rt);
};