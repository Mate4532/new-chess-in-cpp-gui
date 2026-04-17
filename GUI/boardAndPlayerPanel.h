#ifndef BOARDANDPLAYERPANEL_H
#define BOARDANDPLAYERPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include "playerpanel.h"
#include "chessview.h"
#include "Utils.h"

class BoardAndPlayerPanel : public QWidget {
    Q_OBJECT
private:
    QVBoxLayout* mainLay;
    PlayerPanel* whitePlayer;
    PlayerPanel* blackPlayer;
    ChessView* chessView;

public slots:
    void flipPlayerPanels(bool isFlipped);
    void playerPanelChanged(QString playerName, QString playerIconPath, Color playerColor);
    void syncPiecesWithPanels(int pieces[2][6]);
    void onTimerChanged(Color c, QString timerStr);
    void onActivateTimerColorAndDisableOther(Color timerToActivate);
    void onSetTimersVisible();
    void onDisableTimers();
    void clearPanels();

public:
    BoardAndPlayerPanel(PlayerPanel* whitePlayer, PlayerPanel* blackPlayer, ChessView* cv, QWidget* parent = nullptr);
    void setPlayer(const QString& newName, Color playerColor);
    int updateSize(int availableWidth, int availableHeight) ;
    QSize getMinimumOptimalSize();

    void updateMaterialScoreBasedOnPieces(int pieces[2][6]);
    int getMaterialScore(std::vector<PieceType> pieces);
};

#endif // BOARDANDPLAYERPANEL_H
