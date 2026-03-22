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
    void addPieceToPlayerPanel(Color playerColor, PieceType piece);
    void removePiecesFromPanel(Color playerColor, PieceType piece);
    void updateMaterialScore(std::vector<std::pair<PieceType, Color>> pieces);
    void clearPanels();

public:
    BoardAndPlayerPanel(PlayerPanel* whitePlayer, PlayerPanel* blackPlayer, ChessView* cv, QWidget* parent = nullptr);
    void setPlayer(const QString& newName, Color playerColor);
    int updateSize(int availableWidth, int availableHeight) ;
    QSize getMinimumOptimalSize();
};

#endif // BOARDANDPLAYERPANEL_H
