#ifndef PLAYERINFO_H
#define PLAYERINFO_H

#include "Utils.h"
#include <QLabel>

class PlayerInfo
{
public:
    PlayerInfo(Color playerColor, QString playerName, QString playerIcon);
    inline Color getPlayerColor() { return playerColor; }
    inline QString getPlayerName() { return playerName; }
    inline QString getPlayerIcon() { return playerIcon; }

private:
    Color playerColor;
    QString playerName;
    QString playerIcon;
};

#endif // PLAYERINFO_H
