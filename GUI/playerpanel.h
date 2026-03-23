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

    std::vector<PieceType> orderedTakenPieces;
    std::vector<std::vector<PieceType>> orderedTakenPiecesHistory;

    int currentScoreDiff = 0;
    std::vector<int> materialScoreHistory;


    void orderPieces(std::vector<PieceType>& pieces);
    std::string getIconPathForPiece(PieceType piece);
    void deletePieceLabels();

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
    void addPiecesToPanelAtNewPos(int pieces[6]);
    void removePieceFromPanel(PieceType piece);
    void moveBeenMade(int currentPly);
    void reviewHistory(int ply);
    void undoToLastMovePiecesState();
    void clearPanel();

    int getMaterialScore();
    static int getPieceValue(PieceType p);
    static int getPieceOrder(PieceType p);
};

#endif // PLAYERPANEL_H
