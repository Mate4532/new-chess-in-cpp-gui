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
    void loadNewGame();
    void loadFEN(std::string randomFEN);
    std::string getRandomOpening();
    Move getBestMoveOnBoard() { return board.getSideToMove() == WHITE ? whiteRobot->GetBestMove() : blackRobot->GetBestMove(); }
    void MakeRobotMove();
    void printBestMove();
    MoveFlag getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece);
    bool isMovePromotion(int fromX, int fromY, int toX, int toY);
    bool MakeMove(int fromX, int fromY, int toX, int toY, PieceType promotionPiece);
    void undoMove(int plyToUndo);
    void undoLastMove();
	bool didGameEnd();
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

    void ClearBoard();

    inline int getPly() { return board.getPly(); }
    inline bool isRobot(Color c) const { return (c == WHITE && is_white_robot) || (c == BLACK && is_black_robot);}
    inline bool isRobotToMove() const { return (is_white_robot && board.getSideToMove() == WHITE) || (is_black_robot && board.getSideToMove() == BLACK); }
    inline std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const { return board.getBoardMatrix(); }
};
