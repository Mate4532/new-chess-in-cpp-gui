#include "boardAndPlayerPanel.h"

BoardAndPlayerPanel::BoardAndPlayerPanel(PlayerPanel* whitePlayer, PlayerPanel* blackPlayer, ChessView* cv, QWidget* parent)
    : QWidget(parent)
{
    this->whitePlayer = whitePlayer;
    this->blackPlayer = blackPlayer;
    this->chessView = cv;

    mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(10, 10, 10, 10);

    mainLay->addLayout(blackPlayer);
    mainLay->addWidget(chessView);
    mainLay->addLayout(whitePlayer);

    this->setAttribute(Qt::WA_StyledBackground, true);

    this->setObjectName("BoardPanel");

    this->setStyleSheet(R"(
        QWidget#BoardPanel {
            background-color: rgb(49, 46, 43);
            border-radius: 8px;
        }
    )");

    connect(chessView, &ChessView::visualOffsetChanged, this, [=](int offsetPx){
        whitePlayer->setLeftMargin(offsetPx);
        blackPlayer->setLeftMargin(offsetPx);
    });
}

void BoardAndPlayerPanel::setPlayer(const QString& newName, Color playerColor) {
    if (playerColor == BLACK) {
        blackPlayer->setPlayerName(newName);
    }
    else {
        whitePlayer->setPlayerName(newName);
    }
}

void BoardAndPlayerPanel::flipPlayerPanels(bool isFlipped) {
    mainLay->removeItem(blackPlayer);
    mainLay->removeItem(whitePlayer);

    if (isFlipped) {
        mainLay->insertLayout(0, whitePlayer);
        mainLay->insertLayout(2, blackPlayer);
    } else {
        mainLay->insertLayout(0, blackPlayer);
        mainLay->insertLayout(2, whitePlayer);
    }

    mainLay->update();
}

void BoardAndPlayerPanel::playerPanelChanged(QString playerName, QString playerIconPath, Color playerColor) {
    if (playerColor == WHITE){
        whitePlayer->setPlayerName(playerName);
        whitePlayer->setPlayerIcon(playerIconPath);
    }
    else {
        blackPlayer->setPlayerName(playerName);
        blackPlayer->setPlayerIcon(playerIconPath);
    }
}
