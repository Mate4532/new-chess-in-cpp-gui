#ifndef CHESSVIEWMODEL_H
#define CHESSVIEWMODEL_H

#include "BoardManager.h"
#include "settingsDialog.h"

#include <QObject>
#include <QPoint>

class ChessViewModel : public QObject
{
    Q_OBJECT
public:
    explicit ChessViewModel(BoardManager& b, QObject* parent = nullptr);

    void startGame();
    void loadNewGame();
    void endGame();
    void currentPlayerGaveUp();

    void stopRobotCalculation();

    void movePiece(int fromX, int fromY, int toX, int toY, MoveFlag mf = MoveFlag::NORMAL_MOVE);
    void makeRobotMove();
    void undoLastMove();
    std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const;
    void refreshView();

    bool isRobotUnderSearch() const;
    inline bool getIsBoardFlipped() const { return isBoardFlipped; }

    void loadSettings(AllSettings& allS);

public slots:
    void updateSettings(AllSettings& oldS, AllSettings& newS);

signals:
    void boardChanged();

private:
    BoardManager& bm;
    bool isGameRunning = false;
    bool isBeginnerPos = true;
    bool isBoardFlipped = false;

    void afterMoveBeenMade();

    QThread* robotMoveThread = nullptr;

    std::vector<std::vector<std::pair<PieceType, Color>>> cachedMatrix;
};

#endif // CHESSVIEWMODEL_H
