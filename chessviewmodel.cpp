#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
    robotMoveThread = nullptr;
}

void ChessViewModel::startGame() {

    if (isGameRunning || isRobotUnderSearch())
        return;

    if (!isBeginnerPos)
        loadNewGame();

    isGameRunning = true;

    if (bm.isRobotToMove())
        makeRobotMove();
}

 void ChessViewModel::loadNewGame() {

     endGame();

     isBeginnerPos = true;
     bm.loadNewGame();
     emit boardChanged();
 }

void ChessViewModel::endGame() {

    if (isRobotUnderSearch()) {
        stopRobotCalculation();
    }

    isGameRunning = false;
}

bool ChessViewModel::isRobotUnderSearch() const {
    if (!robotMoveThread)
        return false;
    return robotMoveThread->isRunning();
}

void ChessViewModel::stopRobotCalculation() {

    if (!isRobotUnderSearch())
        return;

    bm.stopRobotCalculation();
}

void ChessViewModel::currentPlayerGaveUp() {
    if (!isGameRunning)
        return;
    endGame();
}

void ChessViewModel::movePiece(int fromX, int fromY, int toX, int toY, MoveFlag mf) {

    if (!isGameRunning && isBeginnerPos)
        startGame();

    if (isRobotUnderSearch() || !isGameRunning)
        return;

    if (isBeginnerPos)
        isBeginnerPos = false;

    bool validMove = bm.MakeMove(fromX, fromY, toX, toY, mf);

    if (validMove) {
        afterMoveBeenMade();
    }
}

void ChessViewModel::afterMoveBeenMade() {
    emit boardChanged();

    if (bm.didGameEnd())
        endGame();

    if (isGameRunning && bm.isRobotToMove())
        makeRobotMove();
}

void ChessViewModel::loadSettings(AllSettings& allS) {

    RobotSettings& rs = allS.robotSettings;
    BoardSettings& bs = allS.boardSettings;

    if (rs.isWhiteRobot) bm.setRobot(WHITE);
    else bm.setPlayer(WHITE);
    bm.setDifficulty(WHITE, rs.whiteRobotDifficulty);

    if (rs.isBlackRobot) bm.setRobot(BLACK);
    else bm.setPlayer(BLACK);
    bm.setDifficulty(BLACK, rs.blackRobotDifficulty);

    bool oldFlip = isBoardFlipped;
    isBoardFlipped = bs.isBoardFlipped;

    if (oldFlip != isBoardFlipped) {
        emit boardChanged();
    }
}

void ChessViewModel::updateSettings(AllSettings& oldS, AllSettings& newS) {

    RobotSettings& oldRs = oldS.robotSettings;
    RobotSettings& newRs = newS.robotSettings;
    BoardSettings& oldBs = oldS.boardSettings;
    BoardSettings& newBs = newS.boardSettings;

    bool robotConfigChanged = (newRs != oldRs);
    bool boardCongifChanged = (newBs != oldBs);

    if (robotConfigChanged) {
        endGame();
    }

    if (robotConfigChanged) {

        newRs.isWhiteRobot ? bm.setRobot(WHITE) : bm.setPlayer(WHITE);
        bm.setDifficulty(WHITE, newRs.whiteRobotDifficulty);

        newRs.isBlackRobot ? bm.setRobot(BLACK) : bm.setPlayer(BLACK);
        bm.setDifficulty(BLACK, newRs.blackRobotDifficulty);
    }

    if (boardCongifChanged) {
        isBoardFlipped = newBs.isBoardFlipped;
        emit boardChanged();
    }

    if (robotConfigChanged) {
        loadNewGame();
    }
}

void ChessViewModel::refreshView() {
    if (!isRobotUnderSearch())
        emit boardChanged();
}

void ChessViewModel::makeRobotMove() {

    if (isRobotUnderSearch() || !isGameRunning)
        return;

    if (isBeginnerPos)
        isBeginnerPos = false;

    cachedMatrix = bm.getBoardMatrix();

    QThread* currentThread = QThread::create([this]() {
        bm.MakeRobotMove();
    });

    robotMoveThread = currentThread;

    connect(currentThread, &QThread::finished, this, [this, currentThread]() {

        cachedMatrix.clear();

        bool wasInterrupted = currentThread->isInterruptionRequested();

        if (robotMoveThread != currentThread) {
            currentThread->deleteLater();
            return;
        }

        robotMoveThread = nullptr;
        currentThread->deleteLater();

        if (wasInterrupted) {
            return;
        }

        afterMoveBeenMade();
    });

    currentThread->start();
}

void ChessViewModel::undoLastMove(){
    if (!isGameRunning || isRobotUnderSearch())
        return;

    bm.undoLastMove();
    emit boardChanged();

    if (bm.isRobotToMove())
        makeRobotMove();
}

std::vector<std::vector<std::pair<PieceType, Color>>> ChessViewModel::getBoardMatrix() const {
    if (isRobotUnderSearch()) {
        return cachedMatrix;
    }
    return bm.getBoardMatrix();
}
