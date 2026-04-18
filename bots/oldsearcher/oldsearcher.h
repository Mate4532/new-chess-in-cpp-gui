#ifndef OLDSEARCHER_H
#define OLDSEARCHER_H

#include "ISearcher.h"
#include "OldTranspositionTable.h"
#include "OldMoveOrdering.h"
#include "OldPrecomputedEvaluationData.h"
#include "OldLMR.h"

namespace OldSearcher {

class Searcher : public ISearcher{
private:

    static constexpr int MAXIMUM_DEPTH = 128;
    static constexpr int TT_SIZE_MB = 128;

    Difficulty currentDiff = Difficulty::IMPOSSIBLE;
    RobotTimeUsageMode rtum = RobotTimeUsageMode::FIXED_TIME;

    Board& board;
    OldTT::TranspositionTable tt;
    bool isSearching;

    int negamax(int depth, int alpha, int beta, int ply, Move prev_move = Move(), bool prev_was_capture = false, bool allowNull = false);
    int quiescence(int alpha, int beta);

    std::atomic<long long> fixedTimePerMoveMs{1000};
    std::atomic<long long> timeLeftMs{300000};
    std::atomic<long long> incrementMs{0};

    long long softTimeLimit = 0;
    long long hardTimeLimit = 0;

    int max_depth = 128;

    long long startTime = 0;
    std::atomic<bool> stop{false};
    std::atomic<bool> isStoppedManually{false};
    std::atomic<uint64_t> nodes;

    int historyMoves[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY];
    Move killerMoves[MAX_KILLER_HISTORY][2];

    int staticEvalStack[MAXIMUM_DEPTH];

    RepetitionTable repetitionTable;

    void ClearHistory();
    void AgeHistory();
    void ClearKillers();

public:

    static const int MATE_SCORE = 30000;
    static const int MATE_SCORE_BOUND = 20000;

    Searcher(Board& board) : board(board), tt(TT_SIZE_MB) {
        ClearHistory();
        OldPED::PrecomputedEvaluationData::Init();
        OldLMR::LMR::Init();
    }

    void setFixedTimePerMove(long long timeMs);
    void setTournamentTime(long long timeLeft, long long increment = 0);
    void stopSearch();

    int see(Move m);
    Move IterativeDeepening();
    Move GetRobotMove();
    void PrintPvLine(int depth);
    std::vector<Move> GetPVLine(int depth);
    void PrintWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth);
    std::vector<Move> GetWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth);

    inline bool IsMateScore(int score) { return std::abs(score) >= MATE_SCORE_BOUND;}

    inline void setTimeUsageMode(const RobotTimeUsageMode& rtum) {this->rtum = rtum; }
    void updateTournementTime(long long timeLeftMs) { this->timeLeftMs = timeLeftMs; }

    void ClearSearcher();

    void setDifficulty(const Difficulty& diff);
    inline bool isUnderSearch() { return isSearching; }

    std::string getName() const;
    std::string getNameToSaveInFile() const;
    Difficulty getDifficulty() const;
    std::string getDifficultyString() const;
    std::string getBotDirectoryPath() const;
};

}

#endif // OLDSEARCHER_H
