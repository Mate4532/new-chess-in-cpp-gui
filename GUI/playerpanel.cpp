#include "playerpanel.h"
#include <QLabel>

PlayerPanel::PlayerPanel(QString playerName, QString iconPath, QWidget* parent) : QHBoxLayout(parent) {
    this->setSpacing(0);
    this->setContentsMargins(0, 0, 0, 0);

    playerIconLabel = new QLabel();
    playerLabel = new QLabel();

    playerLabel->setStyleSheet(R"(
        QLabel {
            color: #d5d5d5;
            font-family: 'Segoe UI', sans-serif;
            font-size: 17px;
            font-weight: 600;
            padding-left: 8px;
        }
    )");

    this->addWidget(playerIconLabel, 0, Qt::AlignCenter);
    this->addWidget(playerLabel, 0, Qt::AlignCenter);
    this->addStretch();

    setPlayerName(playerName);
    setPlayerIcon(iconPath);
}

void PlayerPanel::setPlayerName(QString newName) {
    if (playerLabel) {
        playerLabel->setText(newName);
    }
}

void PlayerPanel::setPlayerIcon(QString iconPath) {
    if (playerIconLabel) {
        QIcon playerIcon(iconPath);
        QSize iconSize(40, 40);
        playerIconLabel->setPixmap(playerIcon.pixmap(iconSize));
        playerIconLabel->setFixedSize(iconSize);
    }
}

void PlayerPanel::setLeftMargin(int pixels) {
    this->setContentsMargins(pixels, 0, 0, 0);
}

void PlayerPanel::deleteItems() {
    QLayoutItem* childItem;
    while ((childItem = this->takeAt(0)) != nullptr) {
        if (QWidget* w = childItem->widget()) {
            w->deleteLater();
        }
        delete childItem;
    }
}
