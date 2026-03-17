#ifndef ISEARCHER_H
#define ISEARCHER_H

#include "Board.h"

enum SearcherType {
    OLD_SEARCHER, IMRPOVED_SEARCHER
};

class ISearcher {
public:
    virtual void ClearSearcher() = 0;
    virtual Move GetBestMove() = 0;
    virtual void setDifficulty(Difficulty d) = 0;
    virtual void setSearchTime(int t) = 0;
    virtual bool isUnderSearch() = 0;
    virtual void stopSearch() = 0;
    virtual SearcherType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual ~ISearcher() = default;
};

#endif // ISEARCHER_H
