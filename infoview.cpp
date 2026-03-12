#include "infoview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <iostream>

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

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background-color: transparent;
        }
        QScrollBar:vertical {
            background: #2a2724;
            width: 10px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            min-height: 20px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #888888;
        }
        QWidget#scrollContent {
            background-color: transparent;
        }
    )");

    QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
    connect(vScrollBar, &QScrollBar::rangeChanged, this, [vScrollBar](int min, int max) {
        vScrollBar->setValue(max);
    });

    QWidget* scrollContent = new QWidget();
    scrollContent->setObjectName("scrollContent");

    movesLayout = new QGridLayout(scrollContent);
    movesLayout->setAlignment(Qt::AlignTop);
    movesLayout->setSpacing(10);

    movesLayout->setColumnStretch(0, 1);
    movesLayout->setColumnStretch(1, 2);
    movesLayout->setColumnStretch(2, 2);

    clearMoveDisplay();

    scrollContent->setLayout(movesLayout);
    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(scrollArea);

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

void InfoView::addMoveToDisplay(int moveNumber, int movePly, const QString& move, Color color) {

    QString btnStyle = R"(
        QPushButton {
            background-color: transparent;
            color: #cccccc;
            font-size: 14px;
            font-weight: bold;
            border: none;
            border-radius: 4px;
            padding: 2px 5px;
        }
        QPushButton:hover {
            background-color: #4f4b47;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #B48866;
            color: white;
        }
    )";

    if (color == WHITE) {
        currentRow++;
    }

    int currentCol = color == WHITE ? 1 : 2;

    if (color == WHITE) {
        QLabel* numLabel = new QLabel(QString::number(moveNumber) + ".");
        numLabel->setStyleSheet(numStyle);
        numLabel->setAlignment(Qt::AlignCenter);
        movesLayout->addWidget(numLabel, currentRow, 0);
    }

    QPushButton* moveBtn = new QPushButton(move);
    moveBtn->setStyleSheet(btnStyle);
    moveBtn->setCursor(Qt::PointingHandCursor);

    moveBtn->setProperty("ply", movePly);

    connect(moveBtn, &QPushButton::clicked, this, [this, moveBtn]() {
        int targetPly = moveBtn->property("ply").toInt();
        emit reviewRequested(targetPly);
    });

    movesLayout->addWidget(moveBtn, currentRow, currentCol);

    buttonAmount++;
}

void InfoView::removeLastButFromDisplay() {
    if (buttonAmount < 1)
        return;

    int lastIndex = movesLayout->count() - 1;

    int row, col, rowSpan, colSpan;
    movesLayout->getItemPosition(lastIndex, &row, &col, &rowSpan, &colSpan);

    QLayoutItem* item = movesLayout->takeAt(lastIndex);
    if (item != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if (col == 1) {
        int numIndex = movesLayout->count() - 1;
        QLayoutItem* numItem = movesLayout->takeAt(numIndex);

        if (numItem != nullptr) {
            if (QWidget* widget = numItem->widget()) {
                widget->deleteLater();
            }
            delete numItem;
        }
        currentRow--;
    }

    buttonAmount--;
}

void InfoView::writeGameResultToDisplay(GameResult gr) {
    currentRow++;

    QWidget* resultContainer = new QWidget();
    QHBoxLayout* resultLayout = new QHBoxLayout(resultContainer);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(8);
    resultLayout->setAlignment(Qt::AlignCenter);

    QLabel* textLabel = new QLabel();
    textLabel->setStyleSheet(numStyle);

    QLabel* whiteIconLabel = new QLabel();
    QLabel* blackIconLabel = new QLabel();

    QPixmap wpm(":/resources/resources/white_king.png");
    whiteIconLabel->setPixmap(wpm.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QPixmap bpm(":/resources/resources/black_king.png");
    blackIconLabel->setPixmap(bpm.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    resultLayout->addWidget(whiteIconLabel);

    if (gr == GameResult::WHITE_WON) {
        textLabel->setText("1 - 0");

        resultLayout->addWidget(textLabel);
    }
    else if (gr == GameResult::BLACK_WON) {
        textLabel->setText("0 - 1");

        resultLayout->addWidget(textLabel);
    }
    else if (gr == GameResult::DRAW) {
        textLabel->setText("½ - ½");
        resultLayout->addWidget(textLabel);
    }

    resultLayout->addWidget(blackIconLabel);

    movesLayout->addWidget(resultContainer, currentRow, 0, 1, 3);
}

void InfoView::clearMoveDisplay()
{
    QLayoutItem* item;
    while ((item = movesLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    movesLayout->addWidget(new QLabel(""), 0, 0);
    movesLayout->addWidget(new QLabel(""), 0, 1);
    movesLayout->addWidget(new QLabel(""), 0, 2);

    currentRow = 0;
    buttonAmount = 0;
}

void InfoView::openSettings()
{
    AllSettings oldAllS = currentAllS;

    SettingsDialog dlg(currentAllS, this);

    if (dlg.exec() == QDialog::Accepted) {

        emit settingsChanged(oldAllS, currentAllS);
    }
}
