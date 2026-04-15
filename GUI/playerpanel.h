#ifndef PLAYERPANEL_H
#define PLAYERPANEL_H

#include <QHBoxLayout>
#include <QLabel>
#include "Utils.h"
#include "playerinfo.h"

class PlayerPanel : public QHBoxLayout{
private:
    Color playerColor;

    QLabel* playerLabel;
    QLabel* playerIconLabel;
    QLabel* materialScoreLabel;
    QWidget* piecesContainer;
    QLabel* timerLabel;

    std::vector<QLabel*> takenPiecesLabels;

    std::vector<PieceType> orderedTakenPieces;

    void orderPieces(std::vector<PieceType>& pieces);
    QString getIconPathForPiece(PieceType piece);
    void deletePieceLabels();

    int playerPanelPieceSide = 25;

    int diffPiecesDistanePx[5] = {23, 23, 23, 23, 25};
    int samePiecesDistanePx[5] = {10, 12, 8, 10, 15};

    void preloadPixmaps();

public:
    PlayerPanel(PlayerInfo& player, bool isTimerLabelVisible, QWidget* parent = nullptr);
    void setPlayerName(QString playerName);
    void setPlayerIcon(QString iconPath);
    void setLeftMargin(int pixels);
    void updateMaterialScore(int scoreDiff);
    void updatePanel();
    void syncPiecesWithPanel(int pieces[6]);
    void setTimerVisibility(bool isVisible);
    void setTimerActive(bool isActive);
    void setTimerText(const QString& timeStr);
    void clearPanel();

    int getMaterialScore();
    static int getPieceValue(PieceType p);
    static int getPieceOrder(PieceType p);

    std::unordered_map<QString, QPixmap> loadedPixmaps;
    static const std::unordered_map<PieceType, QString> whitePieceMap;
    static const std::unordered_map<PieceType, QString> blackPieceMap;
};

#endif // PLAYERPANEL_H
