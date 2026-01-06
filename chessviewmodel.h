#ifndef CHESSVIEWMODEL_H
#define CHESSVIEWMODEL_H

#include <QObject>
#include <QPoint>
#include "BoardManager.h"

class ChessViewModel : public QObject
{
    Q_OBJECT
public:
    explicit ChessViewModel(BoardManager& b, QObject* parent = nullptr);

    void startGame();
    void loadNewGame();
    void endGame();
    void currentPlayerGaveUp();

    void movePiece(int fromX, int fromY, int toX, int toY, MoveFlag mf = MoveFlag::NORMAL_MOVE);
    void makeRobotMove();
    void undoLastMove();
    std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const;
    void refreshView();

signals:
    void boardChanged();

private:
    BoardManager& bm;
    bool isUnderSearch = false;
    bool isGameRunning = false;
    bool isBeginnerPos = true;

    std::vector<std::vector<std::pair<PieceType, Color>>> cachedMatrix;
};

#endif // CHESSVIEWMODEL_H
