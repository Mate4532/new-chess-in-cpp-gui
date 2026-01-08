#include "Board.h"
#include "BoardManager.h"
#include "UCIParsing.h"
#include "settingsDialog.h"

#include <sstream>

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}


void BoardManager::goPerft(int perftDepth) {

    std::cout << "Perft(" << perftDepth << ") inditasa..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t nodes = board.PerftDivide(perftDepth);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();

    double nps = (seconds > 0) ? (nodes / seconds) : nodes;

    std::cout << "\n------------------------------------" << std::endl;
    std::cout << "Perft(" << perftDepth << ") eredmenye: " << nodes << " csomopont" << std::endl;
    std::cout << "Idotartam: " << std::fixed << std::setprecision(3) << seconds << " masodperc" << std::endl;
    std::cout << "Sebesseg: " << std::fixed << std::setprecision(0) << nps << " NPS (Nodes Per Second)" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    board.PrintBoard();
}

void BoardManager::loadNewGame(){
    whiteRobot.ClearSearcher();
    blackRobot.ClearSearcher();
    board.loadNewGame();
}

void BoardManager::printBestMove() {
    Move best_move = getBestMoveOnBoard();

    std::cout << "Best move: " + square_to_coordinates[best_move.getFrom()] + square_to_coordinates[best_move.getTo()] << std::endl;
}

bool BoardManager::MakeMove(int fromX, int fromY, int toX, int toY, MoveFlag mf){

    Square fromSq = (Square)(fromY * 8 + fromX);
    Square toSq = (Square)(toY * 8 + toX);

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    for (int i = 0; i < moves.count; ++i){

        Move& m = moves[i];

        if (m.getFrom() == fromSq && m.getTo() == toSq) {

            MoveFlag withoutCaptureFlag = (MoveFlag)(m.getFlags() & ~MoveFlag::CAPTURE_FLAG);

            if (mf & MoveFlag::PROMOTION_FLAG && withoutCaptureFlag == mf){
                board.MakeMove(moves[i]);
                return true;
            }

            else {
                board.MakeMove(moves[i]);
                return true;
            }
        }
    }

    return false;
}

void BoardManager::undoMove(int plyToUndo) {

    for (int i = 0; i < plyToUndo; ++i) {
        if (board.getPly() > 0) {
            board.UndoMove(board.getLastMove());
        }
    }
}

void BoardManager::undoLastMove() {

    Color us = board.getSideToMove();
    Color enemy = (Color)(us ^ 1);
    int plyToUndo = isRobot(enemy) ? 2 : 1;

    for (int i = 0; i < plyToUndo; ++i) {
        if (board.getPly() > 0) {
            board.UndoMove(board.getLastMove());
        }
    }
}

void BoardManager::MakeRobotMove() {
    Move robot_move;
	std::cout << "Robot gondolkodik..." << std::endl;
    if (board.isDebugMode) {
        uint64_t hash_before = board.getHash();
        std::cout << "Hash kereses elott: " << board.getHash() << std::endl;
        robot_move = board.getSideToMove() == WHITE ? whiteRobot.GetBestMove() : blackRobot.GetBestMove();

        uint64_t hash_after = board.getHash();
        std::cout << "Hash kereses utan: " << board.getHash() << std::endl;
        if (hash_before != hash_after) {
            std::cout << "BAJ VAN\n\n\n\n\n" << std::endl;
        }
    }
    else {
        robot_move = board.getSideToMove() == WHITE ? whiteRobot.GetBestMove() : blackRobot.GetBestMove();
    }
    if (!robot_move.isValid())
        return;

    board.MakeMove(robot_move);
    std::cout << "Robot lepese: " + robot_move.toAlgebraic() << std::endl;
}

bool BoardManager::didGameEnd() {

    if (board.IsDraw() || board.IsCheckMate())
        board.PrintBoard(is_white_player, is_black_player);

    if (board.IsDraw()) {
        std::cout << "\nDontetlen!" << std::endl;
        return true;
    }

    if (board.IsCheckMate()) {
        std::cout << "\nSakkmat, " << (board.getSideToMove() == WHITE ? "Fekete" : "Feher") << " nyert!" << std::endl;
        return true;
    }
    return false;
}

void BoardManager::startGameLoop() {

    std::string userInput;

    while (true) {

        if (didGameEnd())
            break;

        if ((board.getSideToMove() == WHITE && is_white_robot) || (board.getSideToMove() == BLACK && is_black_robot)) {
            MakeRobotMove();
        }

        if (didGameEnd())
            break;

        if (is_white_robot && is_black_robot) {
            continue;
        }

        board.PrintBoard(is_white_player, is_black_player);

		std::cout << (board.getSideToMove() == WHITE ? "Feher" : "Fekete") << " van lepesben!" << std::endl;
        std::cout << "Add meg a lepest (pl. e2e4): ";
        std::getline(std::cin >> std::ws, userInput);

        std::vector<std::string> trimmed = tokenize(userInput);

        if (trimmed.size() == 0) {
            continue;
        }

        if (trimmed[0] == "exit") break;
        else if (trimmed[0] == "undo") {

            int n = 0;

            if (trimmed.size() == 1) {
                n = 1;
            }
            else {
                n = trimmed[1][0] - '0';
            }
            for (int i = 0; i < n; ++i) {
                if (board.getPly() > 0) {
                    board.UndoMove(board.getLastMove());
                }
            }

            board.PrintBoard(is_white_player, is_black_player);
            continue;
        }

        Move move = UCIParsing::Parse(userInput, board);

        MoveList moves;
        MoveGenerator::GenerateMoves(board, moves);

        if (!moves.contains(move)) {
            std::cout << "A lepes nem ervenyes!" << std::endl;
            continue;
        }

        if (move.getPieceType() != PIECE_NONE) {
            if (board.MakeMove(move)) {
                std::cout << "Sikeres lepes!" << std::endl;
            }
            else {
                std::cout << "Szabalytalan lepes (sakkban maradsz)!" << std::endl;
            }
        }
        else {
            std::cout << "Ervenytelen koordinatak vagy ures mezo!" << std::endl;
        }
    }
}

void BoardManager::setPlayer(Color c) {
    if (c == WHITE) {
        is_white_player = true;
        is_white_robot = false;
    }

    else {
        is_black_player = true;
        is_black_robot = false;
    }

}

void BoardManager::setRobot(Color c){
    if (c == WHITE) {
        is_white_player = false;
        is_white_robot = true;
    }

    else {
        is_black_player = false;
        is_black_robot = true;
    }
}

void BoardManager::setDifficulty(Color c, Difficulty d) {
    if (c == WHITE) whiteRobot.setDifficulty(d);
    else blackRobot.setDifficulty(d);
}

void BoardManager::stopRobotCalculation() {
    if (whiteRobot.isUnderSearch()) whiteRobot.stopSearch();
    if (blackRobot.isUnderSearch()) blackRobot.stopSearch();
}


