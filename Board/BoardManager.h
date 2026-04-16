#pragma once
#include "Attacks.h"
#include "Board.h"
#include "Searcher.h"
#include "oldsearcher.h"
#include "memory"
#include "Settings.h"
#include "ISearcher.h"

class SettingsDialog;

class BoardManager {
private:
    Board board;
    std::unique_ptr<ISearcher> whiteRobot;
    std::unique_ptr<ISearcher> blackRobot;
    bool is_white_robot;
	bool is_black_robot;
	bool is_white_player;
	bool is_black_player;

    long long whiteTimeLeftMs = 0;
    long long blackTimeLeftMs = 0;
    long long incrementMs = 0;
    std::vector<std::pair<long long, long long>> timeLeftAtPly;
    GameMode gameMode = GameMode::UNLIMITED_THINKING_TIME;
    RobotTimeUsageMode rtum = RobotTimeUsageMode::FIXED_TIME;

    std::chrono::steady_clock::time_point turnStartTime;
    bool isClockRunning = false;

    void setRobotTimeUsageMode(RobotTimeUsageMode rtum);
    void saveRemainingTime();

public:
    const std::string OPENING_PATH = "assets/openings.txt";

    BoardManager();

    std::unique_ptr<ISearcher> createBot(SearcherType st);

	void goPerft(int perftDepth);
    void resetForNewGame();
    void loadBeginnerFEN();
    void loadFEN(std::string FEN);
    std::string getRandomOpening();
    Move getBestMoveOnBoard() { return board.getSideToMove() == WHITE ? whiteRobot->GetRobotMove() : blackRobot->GetRobotMove(); }
    Move MakeRobotMove();
    void printBestMove();
    MoveFlag getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece);
    bool isMovePromotion(int fromX, int fromY, int toX, int toY);
    Move getMove(int fromX, int fromY, int toX, int toY, PieceType promotionPiece);
    bool MakeMove(Move m);
    void undoMove(int plyToUndo);
    bool didGameEnd();
    GameResult getGameResult();
    void writeGameResult();
    void startGameLoop();

    void startTurnClock();
    void stopTurnClock();
    long long getTimeRemaining(Color player) const;

    void setPlayer(Color c);
    void setRobot(Color c);
    void ClearSearchers();
    void setupBotsForNormalGame(const RobotSettings& rs);
    void prepareImprovedBotVsOldBot();
    void SwapRobots();
    void setDifficulty(Color c, Difficulty d);
    void setFixedTimePerMove(long long timePerMoveMs);
    void setTournementTime(long long tournementTimeMs, long long incrementMs = 0);
    void updateRobotTournementTime();
    void setGameMode(GameMode gm);
    void stopRobotCalculation();

    void updateClocks(long long elapsedMs);

    void currentPlayerGaveUp() { board.currentPlayerGaveUp(); }

    std::string getRobotNameWithDifficulty(Color searcherColor);

    inline int getPly() { return board.getPly(); }
    inline int getFullMoveNumber() { return board.getFullMoveNumber(); }
    inline Color getSideToMove(int ply = -1) { return board.getSideToMove(ply); }
    inline bool wasMoveCapture(Move m) { return m.getFlags() & MoveFlag::CAPTURE_FLAG; }
    inline bool wasMovePromotion(Move m) { return m.getFlags() & MoveFlag::PROMOTION_FLAG; }
    inline PieceType getPromotionPiece(Move m) { return Board::GetPromotionPiece(m); }
    inline PieceType getCapturedPieceTypeAt(int ply) { return board.getCapturePieceType(ply); }
    inline PieceType getLastCapturedPieceType() { return board.getLastCapturePieceType(); }
    inline Move getMove(int ply = -1) { return board.getMove(ply); }
    inline bool isRobot(Color c) const { return (c == WHITE && is_white_robot) || (c == BLACK && is_black_robot);}
    inline bool isEnemyRobot() const { return (isRobot((Color)(board.getSideToMove() ^ 1)));}
    inline bool isRobotToMove() const { return (is_white_robot && board.getSideToMove() == WHITE) || (is_black_robot && board.getSideToMove() == BLACK); }
    inline std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix(int ply = -1) const { return board.getBoardMatrix(ply); }
    inline void getPieceCounts(int piecesOut[2][6], int ply = -1) { board.getPieceCounts(piecesOut, ply); }
    inline MoveInfo getMoveInfo(int ply = -1) { return board.getMoveInfo(ply); }
    inline bool wasMoveCheck(int ply = -1) { return board.wasMoveCheck(ply); }
    inline long long getTimeLeft(Color player, int ply = -1) const {
        if (ply == -1)
            return player == WHITE ? whiteTimeLeftMs : blackTimeLeftMs;
        return player == WHITE ? timeLeftAtPly[ply].first : timeLeftAtPly[ply].second;
    }

};
