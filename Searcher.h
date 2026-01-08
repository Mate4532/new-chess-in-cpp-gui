#pragma once
#include "Board.h"
#include "MoveGenerator.h"
#include "MoveOrdering.h"
#include "TranspositionTable.h"
#include "PrecomputedEvaluationData.h"
#include <iostream>
#include "LMR.h"

#define MAXIMUM_DEPTH 128
#define TT_SIZE_MB 128

class Searcher {
private:
    Board& board;
    TranspositionTable tt;
    bool isSearching;

    int negamax(int depth, int alpha, int beta, int ply, Move prev_move = Move(), bool prev_was_capture = false, bool allowNull = false);
    int quiescence(int alpha, int beta);

    int max_depth = 128;
    const int robot_thinking_time_ms = 1000;

    long long startTime = 0;
    std::atomic<bool> stop;
    bool isStoppedManually;
    std::atomic<uint64_t> nodes;

    int historyMoves[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY];
    Move killerMoves[MAX_KILLER_HISTORY][2];
	RepetitionTable repetitionTable;

    void ClearHistory();
    void AgeHistory();
    void ClearKillers();

public:

    static const int MATE_SCORE = 100000;
    static const int MATE_SCORE_BOUND = 90000;

    Searcher(Board& board) : board(board), tt(TT_SIZE_MB) {
        ClearHistory(); 
        PrecomputedEvaluationData::Init();
		LMR::Init();
    }

    void stopSearch();

    int see(Move m);
    Move IterativeDeepening();
    Move GetBestMove();
    void PrintPvLine(int depth);
    std::vector<Move> GetPVLine(int depth);
    void PrintWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth);
    std::vector<Move> GetWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth);

    inline bool IsMateScore(int score) {
        return std::abs(score) >= MATE_SCORE - 1000;
    }

    inline int ScoreToTT(int score, int ply);
	inline int ScoreFromTT(int score, int ply);
    void ClearSearcher();

    void setDifficulty(Difficulty diff);
    inline bool isUnderSearch() { return isSearching; }
};
