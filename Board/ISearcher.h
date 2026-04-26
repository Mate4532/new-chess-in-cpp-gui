#ifndef ISEARCHER_H
#define ISEARCHER_H

#include "Board.h"

enum SearcherType {
    OLD_SEARCHER, IMRPOVED_SEARCHER
};


class ISearcher {
public:
	virtual void setState(const Board& board) = 0;
    virtual void ClearSearcher() = 0;
    virtual Move GetRobotMove() = 0;
    virtual void setDifficulty(const Difficulty& d) = 0;
    virtual void setTimeUsageMode(const RobotTimeUsageMode& rtum) = 0;
    virtual void setFixedTimePerMove(long long timeMs) = 0;
    virtual void setTournamentTime(long long timeLeftMs, long long incrementMs = 0) = 0;
    virtual void updateTournementTime(long long timeLeftMs) = 0;
    virtual bool isUnderSearch() = 0;
    virtual void stopSearch() = 0;
    virtual std::string getName() const = 0;
    virtual std::string getNameToSaveInFile() const = 0;
    virtual Difficulty getDifficulty() const = 0;
    virtual std::string getDifficultyString() const = 0;
    virtual std::string getBotDirectoryPath() const = 0;
    virtual ~ISearcher() = default;
};

#endif // ISEARCHER_H
