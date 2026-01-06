#pragma once
#include "Attacks.h"
#include "Board.h"
#include "Searcher.h"

class BoardManager {
private:
    Board board;
	Searcher searcher;
	bool is_white_robot;
	bool is_black_robot;
	bool is_white_player;
	bool is_black_player;

public:
	BoardManager(bool is_white_robot, bool is_black_robot)
		: board(),
		searcher(board),
		is_white_robot(is_white_robot),
		is_black_robot(is_black_robot) { 
		Attacks::InitAll(); 
		is_white_player = !is_white_robot;
		is_black_player = !is_black_robot;
	}

	void goPerft(int perftDepth);
    void loadNewGame();
	Move getBestMoveOnBoard() { return searcher.GetBestMove(); }
    void MakeRobotMove();
    void printBestMove();
    bool MakeMove(int fromX, int fromY, int toX, int toY, MoveFlag mf);
    void undoMove(int plyToUndo);
    void undoLastMove();
	bool didGameEnd();
    void startGameLoop();

    inline bool isRobot(Color c) const { return (c == WHITE && is_white_robot) || (c == BLACK && is_black_robot);}
    inline bool isRobotToMove() const { return (is_white_robot && board.getSideToMove() == WHITE) || (is_black_robot && board.getSideToMove() == BLACK); }
    inline std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const { return board.getBoardMatrix(); }
};
