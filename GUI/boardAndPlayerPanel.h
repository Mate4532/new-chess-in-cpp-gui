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

public:
    BoardAndPlayerPanel(PlayerPanel* whitePlayer, PlayerPanel* blackPlayer, ChessView* cv, QWidget* parent = nullptr);
    void setPlayer(const QString& newName, Color playerColor);
};

#endif // BOARDANDPLAYERPANEL_H
