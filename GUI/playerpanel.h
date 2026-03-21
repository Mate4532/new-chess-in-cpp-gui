#ifndef PLAYERPANEL_H
#define PLAYERPANEL_H

#include <QHBoxLayout>
#include <QLabel>
#include "Utils.h"

class PlayerPanel : public QHBoxLayout{
private:
    Color playerColor;
    QString playerName;
    QString playerIconPath;

    QLabel* playerLabel;
    QLabel* playerIconLabel;
    QLabel* materialScoreLabel;
    QWidget* piecesContainer;

    std::vector<QLabel*> takenPiecesLabels;
    std::vector<PieceType> orderedPieces;

    void orderPieces(std::vector<PieceType>& pieces);
    std::string getIconPathForPiece(PieceType piece);
    void deletePieceLabels();

    int getPieceValue(PieceType p);
    int getPieceOrder(PieceType p);

    int playerPanelPieceSide = 25;

    int diffPiecesDistanePx[5] = {23, 23, 23, 23, 25};
    int samePiecesDistanePx[5] = {10, 12, 8, 10, 15};

public:
    PlayerPanel(Color playerColor, QString playerName, QString playerIcon, QWidget* parent = nullptr);
    void setPlayerName(QString playerName);
    void setPlayerIcon(QString iconPath);
    void setLeftMargin(int pixels);
    void updateMaterialScore(int scoreDiff);
    void updatePanel();
    void addPieceToPanel(PieceType piece);
    void removePieceFromPanel(PieceType);
    void clearPanel();

    int getMaterialScore();
};

#endif // PLAYERPANEL_H
