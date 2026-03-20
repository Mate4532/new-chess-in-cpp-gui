#ifndef PLAYERPANEL_H
#define PLAYERPANEL_H

#include <QHBoxLayout>
#include <QLabel>
#include "Utils.h"

class PlayerPanel : public QHBoxLayout{
private:
    QString playerName;
    QString playerIconPath;

    QLabel* playerLabel;
    QLabel* playerIconLabel;

public:
    PlayerPanel(QString playerName, QString playerIcon, QWidget* parent = nullptr);
    void setPlayerName(QString playerName);
    void setPlayerIcon(QString iconPath);
    void setPlayerToRobot(QString robotName);
    void updatePanel();
    void setLeftMargin(int pixels);
    void deleteItems();
};

#endif // PLAYERPANEL_H
