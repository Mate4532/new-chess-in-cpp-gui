#include "BotFactory.h"
#include <oldsearcher.h>
#include <searcher.h>

static auto globalTT = std::make_shared<ImprovedTT::TranspositionTable>(128);

std::unique_ptr<ISearcher> BotFactory::createBot(SearcherType st, RobotSettings robotSettings) {
    switch (st) {
    case SearcherType::OLD_SEARCHER:
        return std::make_unique<OldSearcher::Searcher>();

    case SearcherType::IMPROVED_SEARCHER:

        return std::make_unique<ImprovedSearcher::Searcher>(globalTT.get(), robotSettings.numThreads);
    default:
        return std::make_unique<ImprovedSearcher::Searcher>(globalTT.get(), robotSettings.numThreads);
    }
}