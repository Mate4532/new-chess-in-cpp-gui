#ifndef CHESSVIEWMODEL_H
#define CHESSVIEWMODEL_H

#include "BoardManager.h"
#include "settingsDialog.h"

#include <QObject>
#include <QPoint>
#include <QtConcurrent>
#include <QFutureWatcher>

class ChessViewModel : public QObject
{
    Q_OBJECT
public:
    explicit ChessViewModel(BoardManager& b, QObject* parent = nullptr);
    static constexpr int ROBOT_GAMES = 200;

    AllSettings currentSettings;

    void startGame();
    void loadNewGame();
    void endGame();
    void currentPlayerGaveUp();

    void stopRobotSearch();
    void switchBots();

    void movePiece(int fromX, int fromY, int toX, int toY, PieceType promotionPiece = PieceType::PIECE_NONE);
    void makeRobotMove();
    void startRobotGameLoop();
    void onRobotMoveFinished();

    void undoLastMove();
    std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const;
    void refreshView();

    bool isMovePromotion(int fromX, int fromY, int toX, int toY) const;
    bool isRobotUnderSearch() const;
    inline bool getIsBoardFlipped() const { return isBoardFlipped; }

    void loadSettings(AllSettings& allS);

public slots:
    void updateSettings(AllSettings& oldS, AllSettings& newS);

signals:
    void gameEnded();
    void boardChanged();

private:
    BoardManager& bm;
    bool isGameRunning = false;
    bool isUnderSearch = false;
    bool isBeginnerPos = true;
    bool isBoardFlipped = false;
    bool stopBotSimulation = false;

    void afterMoveBeenMade();

    QFutureWatcher<void> robotWatcher;

    std::vector<std::vector<std::pair<PieceType, Color>>> cachedMatrix;
};

#endif // CHESSVIEWMODEL_H
