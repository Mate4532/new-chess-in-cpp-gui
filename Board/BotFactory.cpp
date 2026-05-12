#include "BotFactory.h"
#include <oldsearcher.h>
#include <searcher.h>

std::unique_ptr<ISearcher> BotFactory::createBot(SearcherType st, RobotSettings robotSettings) {
    switch (st) {
    case SearcherType::OLD_SEARCHER:
        return std::make_unique<OldSearcher::Searcher>();

    case SearcherType::IMPROVED_SEARCHER:

        return std::make_unique<ImprovedSearcher::Searcher>(robotSettings.numThreads);
    default:
        return std::make_unique<ImprovedSearcher::Searcher>(robotSettings.numThreads);
    }
}
