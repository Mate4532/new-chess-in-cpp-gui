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

public:
    const std::string OPENING_PATH = "assets/openings.txt";

    BoardManager();

    std::unique_ptr<ISearcher> createBot(SearcherType st);

	void goPerft(int perftDepth);
    void resetForNewGame();
    void loadBeginnerFEN();
    void loadFEN(std::string randomFEN);
    std::string getRandomOpening();
    Move getBestMoveOnBoard() { return board.getSideToMove() == WHITE ? whiteRobot->GetBestMove() : blackRobot->GetBestMove(); }
    Move MakeRobotMove();
    void printBestMove();
    MoveFlag getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece);
    bool isMovePromotion(int fromX, int fromY, int toX, int toY);
    Move getMove(int fromX, int fromY, int toX, int toY, PieceType promotionPiece);
    bool MakeMove(Move m);
    void undoMove(int plyToUndo);
    void undoLastMove();
    bool didGameEnd();
    GameResult getGameResult();
    void writeGameResult();
    void startGameLoop();

    void setPlayer(Color c);
    void setRobot(Color c);
    void ClearSearchers();
    void setupBotsForNormalGame(const RobotSettings& rs);
    void prepareImprovedBotVsOldBot();
    void SwapRobots();
    void setDifficulty(Color c, Difficulty d);
    void setSearchTime(int t);
    void stopRobotCalculation();

    void currentPlayerGaveUp() { board.currentPlayerGaveUp(); }

    std::string getRobotNameWithDifficulty(Color searcherColor);

    inline int getPly() { return board.getPly(); }
    inline int getFullMoveNumber() { return board.getFullMoveNumber(); }
    inline Color getSideToMove() { return board.getSideToMove(); }
    inline bool wasMoveCapture(Move m) { return m.getFlags() & MoveFlag::CAPTURE_FLAG; }
    inline bool wasMovePromotion(Move m) { return m.getFlags() & MoveFlag::PROMOTION_FLAG; }
    inline PieceType getPromotionPiece(Move m) { return Board::GetPromotionPiece(m); }
    inline PieceType getCapturedPieceTypeAt(int ply) { return board.getCapturePieceType(ply); }
    inline PieceType getLastCapturedPieceType() { return board.getLastCapturePieceType(); }
    inline bool isRobot(Color c) const { return (c == WHITE && is_white_robot) || (c == BLACK && is_black_robot);}
    inline bool isEnemyRobot() const { return (isRobot((Color)(board.getSideToMove() ^ 1)));}
    inline bool isRobotToMove() const { return (is_white_robot && board.getSideToMove() == WHITE) || (is_black_robot && board.getSideToMove() == BLACK); }
    inline std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix(int ply = -1) const { return board.getBoardMatrix(ply); }
    inline void getPieceCounts(int piecesOut[2][6], int ply = -1) { board.getPieceCounts(piecesOut, ply); }

};
