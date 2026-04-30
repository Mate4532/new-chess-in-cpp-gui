#pragma once
#include "Attacks.h"
#include "Board.h"
#include "Searcher.h"
#include "oldsearcher.h"
#include "memory"
#include "Settings.h"
#include "BotFactory.h"
#include "ResultManager.h"
#include <mutex>

class SettingsDialog;

class BoardManager {
private:
    Board board;
    ResultManager resultManager;
    std::unique_ptr<ISearcher> whiteRobot;
    std::unique_ptr<ISearcher> blackRobot;
    bool is_white_player = true;
    bool is_black_player = true;

    long long whiteTimeLeftMs = 0;
    long long blackTimeLeftMs = 0;
    long long incrementMs = 0;
    std::vector<std::pair<long long, long long>> timeLeftAtPly;

    std::chrono::steady_clock::time_point turnStartTime;
    bool isClockRunning = false;

    void setRobotTimeUsageMode(RobotTimeUsageMode rtum);
    void saveRemainingTime();
    void updateRobotsState();

    bool isInBotSimulation = false;
    std::string currentSimFen;

    AllSettings currentSettings;

    static inline std::mutex consoleMutex;

public:
    const std::string OPENING_PATH = "assets/openings.txt";

    BoardManager();

	void goPerft(int perftDepth);
    void resetForNewGame();
    void loadOpenings();
    void loadBeginnerFEN();
    void loadFEN(std::string FEN);
    std::string getRandomOpening();
    std::string popRandomFen();
    Move getBestMoveOnBoard() { return board.getSideToMove() == WHITE ? whiteRobot->GetRobotMove() : blackRobot->GetRobotMove(); }
    Move MakeRobotMove();
    void printBestMove();
    bool isMovePromotion(int fromX, int fromY, int toX, int toY);
    Move getMove(int fromX, int fromY, int toX, int toY, PieceType promotionPiece = PIECE_NONE);
    bool MakeMove(Move m);
    void undoMove(int plyToUndo);
    GameResult getGameResult();
    bool didGameEnd();
    std::string getGameResultString(GameResult gameResult);
    void writeGameResult(GameResult gameResult, std::vector<std::string> moveList, std::string beginnerFen);
    void startGameLoop();
    void runUCIService();

    void startTurnClock();
    void stopTurnClock();
    void restartClock();
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
    void setSettings(const AllSettings& settings);
    void stopRobotCalculation();
    std::vector<std::pair<int, int>> getLegalMovesForPiece(int file, int rank);
    std::pair<int, int> getKingSquare(Color kingColor, int ply = -1);

    void updateClocks(long long elapsedMs);

    void currentPlayerGaveUp() { board.currentPlayerGaveUp(); }

    std::string getRobotNameWithDifficulty(Color searcherColor);

    inline int getPly() { return board.getPly(); }
    inline int getFullMoveNumber() { return board.getFullMoveNumber(); }
    inline Color getSideToMove(int ply = -1) { return board.getCommittedSideToMove(ply); }
    inline MoveFlag getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece) { return board.getMoveFlagBasedOnPromotionPiece(promotionPiece); }
    inline bool wasMoveCapture(Move m) { return m.getFlags() & MoveFlag::CAPTURE_FLAG; }
    inline bool wasMovePromotion(Move m) { return m.getFlags() & MoveFlag::PROMOTION_FLAG; }
    inline PieceType getPromotionPiece(Move m) { return Board::GetPromotionPiece(m); }
    inline PieceType getCapturedPieceTypeAt(int ply) { return board.getCapturePieceType(ply); }
    inline PieceType getLastCapturedPieceType() { return board.getLastCapturePieceType(); }
    inline Move getMove(int ply = -1) { return board.getMove(ply); }
    inline bool isRobot(Color c) const { return (c == WHITE && !is_white_player) || (c == BLACK && !is_black_player);}
    inline bool isEnemyRobot() const { return (isRobot((Color)(board.getCommittedSideToMove() ^ 1)));}
    inline bool isRobotToMove() const { return (isRobot(WHITE) && board.getCommittedSideToMove() == WHITE) || (isRobot(BLACK) && board.getSideToMove() == BLACK); }
    inline std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix(int ply = -1) const { return board.getBoardMatrix(ply); }
    inline void getPieceCounts(int piecesOut[2][6], int ply = -1) { board.getPieceCounts(piecesOut, ply); }
    inline MoveInfo getMoveInfo(int ply = -1) { return board.getMoveInfo(ply); }
    inline bool wasMoveCheck(int ply = -1) { return board.wasMoveCheck(ply); }
    inline std::string convertMoveToSAN(int ply = -1, bool addPieceCharToString = true) { return board.convertMoveToSAN(ply, addPieceCharToString); }
    inline long long getTimeLeft(Color player, int ply = -1) const {
        if (ply == -1)
            return player == WHITE ? whiteTimeLeftMs : blackTimeLeftMs;
        return player == WHITE ? timeLeftAtPly[ply].first : timeLeftAtPly[ply].second;
    }

    void startBotSimulation(int numGames, int threadId = -1);
    void startMultiThreadedSimulation(int totalGames, int numThreads);
    void stopBotSimulation();

};
