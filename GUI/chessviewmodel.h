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

    void movePiece(int fromX, int fromY, int toX, int toY, PieceType promotionPiece = PieceType::PIECE_NONE);
    void makeRobotMove();
    void stopRobotGameLoop();
    void startRobotGameLoop();
    void onRobotMoveFinished(Move m);

    void undoMove();
    MoveInfo getMoveInfo();
    std::vector<std::vector<std::pair<PieceType, Color>>> getBoardMatrix() const;
    void refreshView();

    void loadBeginnerFEN();
    void loadFEN(std::string fen);

    bool isMovePromotion(int fromX, int fromY, int toX, int toY) const;
    bool isRobotUnderSearch() const;
    inline bool getIsBoardFlipped() const { return isBoardFlipped; }

    void loadSettings(AllSettings& allS);

public slots:
    void updateSettings(AllSettings& oldS, AllSettings& newS);
    void reviewHistory(int targetPly);
    void advanceSimulation();
    void runNextSimGame();

signals:
    void gameEnded(GameResult gr);
    void robotSimulationEnded();
    void boardChanged();
    void moveMade(int movePly, const QString& move, Color c);
    void removeLastButFromInfoDisplayRequest();
    void newGameStarted();
    void flipBoardToRequest(bool isFlipped);
    void playerPanelsUpdateRequest(QString playerName, QString playerIconPath, Color playerColor);
    void syncPiecesWithPanelsRequest(int pieces[2][6]);

private:
    BoardManager& bm;
    bool isGameRunning = false;
    bool isUnderSearch = false;
    bool isBeginnerPos = true;
    bool isBoardFlipped = false;
    bool isInBotSimulation = false;

    void afterMoveBeenMade(Move m);
    void swapRobots();

    void updatePlayerPanelsIconAndLabel();
    void updatePlayerPanelAtNewPos();

    QThread* robotThread = nullptr;
    int simI = 0;
    int simJ = 0;
    QString currentSimFen;
    std::vector<std::vector<std::pair<PieceType, Color>>> cachedMatrix;

    int reviewingPly = -1;
    void reviewEnded();
};

#endif // CHESSVIEWMODEL_H
