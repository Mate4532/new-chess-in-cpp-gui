#ifndef OLDSEARCHER_H
#define OLDSEARCHER_H

#include "ISearcher.h"
#include "OldTranspositionTable.h"
#include "OldMoveOrdering.h"
#include "OldLMR.h"
#include "nnue.h"
#include "SearcherSettings.h"

#include <malloc.h>

namespace OldSearcher {

class Searcher : public ISearcher{
private:

    static constexpr int MAXIMUM_DEPTH = 128;
    static constexpr int TT_SIZE_MB = 128;

    Difficulty currentDiff = Difficulty::IMPOSSIBLE;
    RobotTimeUsageMode rtum = RobotTimeUsageMode::FIXED_TIME;

    Board board;
    OldTT::TranspositionTable tt;
    bool isSearching;

    int negamax(int depth, int alpha, int beta, int ply, Move prev_move = Move(), bool prev_was_capture = false, bool allowNull = false);
    int quiescence(int alpha, int beta, int ply);

    std::atomic<long long> fixedTimePerMoveMs{1000};
    std::atomic<long long> timeLeftMs{300000};
    std::atomic<long long> incrementMs{0};

    long long softTimeLimit = 0;
    long long hardTimeLimit = 0;

    long long startTime = 0;
    std::atomic<bool> stop{false};
    std::atomic<bool> isStoppedManually{false};
    std::atomic<uint64_t> nodes;

    int historyMoves[2][SQUARE_COUNT][SQUARE_COUNT];
    Move killerMoves[MAX_KILLER_HISTORY][2];

    RepetitionTable repetitionTable;

    NNUEdata nnue_state[MAXIMUM_DEPTH + 10];

    void PrepareSearcher();
    void ClearHistory();
    void AgeHistory();
    void ClearKillers();

    SearcherSettings currentSettings = SearcherSettings::getSettings(Difficulty::IMPOSSIBLE);
    int movesWithoutBlunderOnPropuse = 0;

public:

    static const int MATE_SCORE = 30000;
    static const int MATE_SCORE_BOUND = 20000;

    Searcher() : board(), tt(TT_SIZE_MB) {
        ClearHistory();
        OldLMR::LMR::Init();
        nnue_init("nn-62ef826d1a6d.nnue");
    }

    int see(Move m);
    Move IterativeDeepening();
    Move GetBestAmongTopMoves(const SearcherSettings& settings);
    void PrintPvLine(int depth);
    std::vector<Move> GetPVLine(int depth);
    void PrintWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth);
    std::vector<Move> GetWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth);

    inline bool IsMateScore(int score) { return std::abs(score) >= MATE_SCORE_BOUND;}

    void setState(const Board& board) override { this->board = board;  };
    void ClearSearcher() override;
    Move GetRobotMove() override;
    void setDifficulty(const Difficulty& diff) override;
    inline void setTimeUsageMode(const RobotTimeUsageMode& rtum) override { this->rtum = rtum; }
    inline void setFixedTimePerMove(long long timeMs) override { fixedTimePerMoveMs = timeMs; }
    inline void updateTournementTime(long long timeLeftMs) override { this->timeLeftMs = timeLeftMs; }
    void setTournamentTime(long long timeLeftMs, long long incrementMs = 0) override;
    inline bool isUnderSearch() override { return isSearching; }
    void stopSearch() override;
    std::string getName() const override;
    std::string getNameToSaveInFile() const override;
    Difficulty getDifficulty() const override;
    std::string getDifficultyString() const override;
    std::string getBotDirectoryPath() const override;

};

}

#endif // OLDSEARCHER_H
