#include "infoview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTimer>

InfoView::InfoView(AllSettings& allS, QWidget* parent) : currentAllS(allS), QWidget(parent)
{

    setMinimumWidth(MIN_WIDTH);
    this->setObjectName("InfoPanel");
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->setStyleSheet(R"(
        #InfoPanel {
            background-color: rgb(49, 46, 43);
            border-radius: 10px;
        }
        QPushButton#settingsBtn {
            background-color: transparent;
            color: #888;
            font-size: 20px;
            border: none;
            font-family: "Segoe UI Symbol";
        }
        QPushButton#settingsBtn:hover { color: white; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->addStretch();

    btnSettings = new QPushButton("⚙");
    btnSettings->setObjectName("settingsBtn");
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setFixedSize(40, 40);

    QString settingsStyle = R"(
        QPushButton {
            background-color: transparent;
            color: #888888;
            font-size: 30px;
            border: none;
            border-radius: 22px;
            padding: 0px;
        }

        QPushButton:hover {
            color: #ffffff;
        }

        QPushButton:pressed {
            color: #B48866;
            padding-top: 2px;
            padding-left: 2px;
        }
    )";

    btnSettings->setStyleSheet(settingsStyle);
    headerLayout->addWidget(btnSettings);

    mainLayout->addLayout(headerLayout);

    mainLayout->addStretch();

    btnNewGame = new QPushButton("Új Játék");
    btnUndo = new QPushButton("Visszavonás");
    btnGiveUp = new QPushButton("Feladás");

    QString buttonStyle = R"(
        QPushButton {
            background-color: #B48866; color: white; border: none; padding: 10px;
            font-size: 14px; font-weight: bold; border-radius: 4px;
        }
        QPushButton:hover { background-color: #906C51; }
        QPushButton:pressed { background-color: #644B38; }
    )";
    btnNewGame->setStyleSheet(buttonStyle);
    btnUndo->setStyleSheet(buttonStyle);
    btnGiveUp->setStyleSheet(buttonStyle);

    mainLayout->addWidget(btnNewGame);
    mainLayout->addWidget(btnUndo);
    mainLayout->addWidget(btnGiveUp);

    connect(btnNewGame, &QPushButton::clicked, this, &InfoView::newGameRequested);
    connect(btnUndo, &QPushButton::clicked, this, &InfoView::undoRequested);
    connect(btnGiveUp, &QPushButton::clicked, this, &InfoView::giveUpRequested);
    connect(btnSettings, &QPushButton::clicked, this, &InfoView::openSettings);
}

void InfoView::openSettings()
{
    AllSettings oldAllS = currentAllS;

    SettingsDialog dlg(currentAllS, this);

    if (dlg.exec() == QDialog::Accepted) {

        emit settingsChanged(oldAllS, currentAllS);
    }
}
