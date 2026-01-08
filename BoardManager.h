#pragma once
#include "Attacks.h"
#include "Board.h"
#include "Searcher.h"
#include "Settings.h"

class SettingsDialog;

class BoardManager {
private:
    Board board;
    Searcher whiteRobot;
    Searcher blackRobot;
    bool is_white_robot;
	bool is_black_robot;
	bool is_white_player;
	bool is_black_player;

public:
    BoardManager()
		: board(),
        whiteRobot(board),
        blackRobot(board) {
		Attacks::InitAll(); 
		is_white_player = !is_white_robot;
        is_black_player = !is_black_robot;
	}

	void goPerft(int perftDepth);
    void loadNewGame();
    Move getBestMoveOnBoard() { return board.getSideToMove() == WHITE ? whiteRobot.GetBestMove() : blackRobot.GetBestMove(); }
    void MakeRobotMove();
    void printBestMove();
    MoveFlag getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece);
    bool isMovePromotion(int fromX, int fromY, int toX, int toY);
    bool MakeMove(int fromX, int fromY, int toX, int toY, PieceType promotionPiece);
    void undoMove(int plyToUndo);
    void undoLastMove();
	bool didGameEnd();
    void startGameLoop();

    void setPlayer(Color c);
    void setRobot(Color c);
    void setDifficulty(Color c, Difficulty d);
    void stopRobotCalculation();

    inline int getPly() { return board.getPly(); }
    inline bool isRobot(Color c) const { return (c == WHITE && is_white_robot) || (c == BLACK && is_black_robot);}
    inline bool isRobotToMove() const { return (is_white_robot && board.getSideToMove() == WHITE) || (is_black_robot && board.getSideToMove() == BLACK); }
    inline std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const { return board.getBoardMatrix(); }
};
