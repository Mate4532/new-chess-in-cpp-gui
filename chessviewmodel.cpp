#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
}

void ChessViewModel::startGame() {
    if (isGameRunning || isUnderSearch)
        return;

    isGameRunning = true;

    if (bm.isRobotToMove())
        makeRobotMove();
}

 void ChessViewModel::loadNewGame() {
     if (isUnderSearch)
         return;

     isBeginnerPos = true;
     bm.loadNewGame();
     emit boardChanged();
     startGame();
 }

void ChessViewModel::endGame() {
    isGameRunning = false;
}

void ChessViewModel::currentPlayerGaveUp() {
    if (!isGameRunning || isUnderSearch)
        return;
    endGame();
}

void ChessViewModel::movePiece(int fromX, int fromY, int toX, int toY, MoveFlag mf) {

    if (!isGameRunning && isBeginnerPos)
        startGame();

    if (isUnderSearch || !isGameRunning)
        return;

    if (isBeginnerPos)
        isBeginnerPos = false;

    bool validMove = bm.MakeMove(fromX, fromY, toX, toY, mf);

    if (validMove) {
        emit boardChanged();

        if (isGameRunning && bm.isRobotToMove())
            makeRobotMove();

        if (bm.didGameEnd())
            endGame();
    }
}

void ChessViewModel::refreshView() {
    if (!isUnderSearch)
        emit boardChanged();
}

void ChessViewModel::makeRobotMove() {

    if (isUnderSearch || !isGameRunning)
        return;

    cachedMatrix = bm.getBoardMatrix();
    isUnderSearch = true;

    QThread* thread = QThread::create([this]() {
        bm.MakeRobotMove();

    });

    connect(thread, &QThread::finished, this, [this, thread]() {

        isUnderSearch = false;

        cachedMatrix.clear();

        emit boardChanged();

        if (bm.didGameEnd())
            endGame();

        thread->deleteLater();
    });

    thread->start();
}

void ChessViewModel::undoLastMove(){
    if (!isGameRunning || isUnderSearch)
        return;

    bm.undoLastMove();
    emit boardChanged();
}

std::vector<std::vector<std::pair<PieceType, Color>>> ChessViewModel::getBoardMatrix() const {
    if (isUnderSearch) {
        return cachedMatrix;
    }
    return bm.getBoardMatrix();
}
