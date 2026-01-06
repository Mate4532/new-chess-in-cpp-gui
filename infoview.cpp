#include "infoview.h"
#include <QVBoxLayout>
#include <QLabel>

InfoView::InfoView(QWidget* parent) : QWidget(parent){
    setMinimumWidth(200);

    this->setObjectName("InfoPanel");

    this->setAttribute(Qt::WA_StyledBackground, true);

    this->setStyleSheet(R"(
        #InfoPanel {
            background-color: rgba(30, 30, 30, 180);
            border-radius: 10px;
        }
    )");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    layout->addStretch();

    btnNewGame = new QPushButton("Új Játék");
    btnUndo = new QPushButton("Visszavonás");
    btnGiveUp = new QPushButton("Feladás");

    QString buttonStyle = R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 8px;
            font-size: 14px;
            font-weight: bold;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3e8e41;
        }
    )";

    btnNewGame->setStyleSheet(buttonStyle);
    btnUndo->setStyleSheet(buttonStyle + "QPushButton { background-color: #2196F3; } QPushButton:hover { background-color: #0b7dda; }");
    btnGiveUp->setStyleSheet(buttonStyle + "QPushButton { background-color: #f44336; } QPushButton:hover { background-color: #da190b; }");


    layout->addWidget(btnNewGame);
    layout->addWidget(btnUndo);
    layout->addWidget(btnGiveUp);

    connect(btnNewGame, &QPushButton::clicked, this, &InfoView::newGameRequested);
    connect(btnUndo, &QPushButton::clicked, this, &InfoView::undoRequested);
    connect(btnGiveUp, &QPushButton::clicked, this, &InfoView::giveUp);
}
