#pragma once
#include "ISearcher.h"
#include "Board.h"
#include "MoveGenerator.h"
#include "MoveOrdering.h"
#include "TranspositionTable.h"
#include "SearcherSettings.h"
#include "LMR.h"
#include "nnue.h"

#include <atomic>

namespace ImprovedSearcher {

class Searcher : public ISearcher{
private:

    static constexpr int MAXIMUM_DEPTH = 128;
    static constexpr int TT_SIZE_MB = 128;

    Difficulty currentDiff = Difficulty::IMPOSSIBLE;
    RobotTimeUsageMode rtum = RobotTimeUsageMode::FIXED_TIME;

    Board board;
    bool isSearching = false;

    int negamax(int depth, int alpha, int beta, int ply);
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
    uint64_t localNodes;

    std::shared_ptr<ImprovedTT::TranspositionTable> tt;

    const int SCORE_NONE = 32000;
    int evalHistory[MAX_PLY];

    int historyMoves[2][SQUARE_COUNT][SQUARE_COUNT];
    Move killerMoves[MAX_KILLER_HISTORY][2];

    alignas(64) NNUEdata nnue_state[MAX_PLY];

    struct RootMove {
        Move m;
        int score;
    };

    std::vector<RootMove> rootMoves;

    Move pvTable[MAXIMUM_DEPTH][MAXIMUM_DEPTH];
    int pvLength[MAXIMUM_DEPTH];

    void PrepareSearcher();

    void ClearKillers();
    void ClearHistory();
    void AgeHistory();

    SearcherSettings currentSettings = SearcherSettings::getSettings(Difficulty::IMPOSSIBLE);
    int movesWithoutBlunderOnPropuse = 0;

    int threads = 1;
    bool isHelper = false;
    std::atomic<bool>* abortPtr = nullptr;
    bool shouldStop();
    bool isStopped() { return (stop || (abortPtr && abortPtr->load())); }

public:

    static constexpr int MATE_SCORE = 30000;
    static constexpr int MATE_SCORE_BOUND = 20000;

    Searcher(int threads = 1) : board(), threads(threads) {
        tt = std::make_shared<ImprovedTT::TranspositionTable>(TT_SIZE_MB);
        ClearHistory();
        ClearKillers();
        ImprovedLMR::LMR::Init();
        std::memset(nnue_state, 0, sizeof(nnue_state));
    }

    Searcher(std::shared_ptr<ImprovedTT::TranspositionTable> sharedTT) : board(), tt(sharedTT), threads(1) {
        ClearHistory();
        ClearKillers();
        ImprovedLMR::LMR::Init();
        std::memset(nnue_state, 0, sizeof(nnue_state));
    }

    Move IterativeDeepening(bool silent = true);
    inline bool IsMateScore(int score) { return std::abs(score) >= MATE_SCORE_BOUND;}

    void setState(const Board& board) override { this->board = board; };
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
