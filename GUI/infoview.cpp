#include "infoview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <iostream>

QFont InfoView::resizeFontSize(QFont f) {
    QFont mbf = f;
    mbf.setPixelSize(qMax(16, panelCurrentWidth / 20));
    return mbf;
}

int InfoView::getCurrentFontMinWidth() {
    int sSize = qMax(14, panelCurrentWidth / 15);
    return sSize * 3;
}

QPushButton* InfoView::createMoveButton(const QString& move, int movePly) {

    QFont btnFont;
    btnFont.setWeight(QFont::Bold);

    QFont resizedBtnFont = resizeFontSize(btnFont);

    QPushButton* moveBtn = new QPushButton(move);
    moveBtn->setStyleSheet(moveBtnStyle);
    moveBtn->setCursor(Qt::PointingHandCursor);
    moveBtn->setProperty("ply", movePly);
    moveBtn->setFont(resizedBtnFont);
    moveBtn->setMinimumWidth(getCurrentFontMinWidth());
    moveBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    return moveBtn;
}

InfoView::InfoView(AllSettings& allS, QWidget* parent) : currentAllS(allS), QWidget(parent)
{
    setMinimumWidth(MIN_WIDTH);
    this->setObjectName("InfoPanel");
    this->setAttribute(Qt::WA_StyledBackground, true);
    panelCurrentWidth = this->width();

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
            background-color: #555555;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #888888;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
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

    movesLayout->setColumnStretch(0, 2);
    movesLayout->setColumnStretch(1, 5);
    movesLayout->setColumnStretch(2, 5);

    movesLayout->setSpacing(0);
    movesLayout->setContentsMargins(0, 0, 0, 0);

    scrollContent->setLayout(movesLayout);
    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(scrollArea);

    resultBox = new QWidget(this);
    resultBox->setVisible(false);
    resultBox->setStyleSheet("background: transparent;");
    QVBoxLayout* vLay = new QVBoxLayout(resultBox);

    QHBoxLayout* hLay = new QHBoxLayout();
    whiteKing = new QLabel();
    blackKing = new QLabel();
    scoreLabel = new QLabel();

    whiteKing->setAlignment(Qt::AlignRight);
    blackKing->setAlignment(Qt::AlignLeft);

    scoreLabel->setAlignment(Qt::AlignCenter);
    scoreLabel->setStyleSheet("color: white; font-weight: bold;");

    hLay->addWidget(whiteKing);
    hLay->addWidget(scoreLabel);
    hLay->addWidget(blackKing);

    statusLabel = new QLabel();
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #888;");

    vLay->setAlignment(Qt::AlignVCenter);

    vLay->addLayout(hLay);
    vLay->addWidget(statusLabel);

    mainLayout->addWidget(resultBox);

    clearMoveDisplay();

    btnNewGame = new QPushButton("Új Játék");
    btnUndo = new QPushButton("Visszavonás");
    btnGiveUp = new QPushButton("Feladás");

    btnNewGame->setStyleSheet(mainButtonStyle);
    btnUndo->setStyleSheet(mainButtonStyle);
    btnGiveUp->setStyleSheet(mainButtonStyle);

    mainLayout->addWidget(btnNewGame);
    mainLayout->addWidget(btnUndo);
    mainLayout->addWidget(btnGiveUp);

    mainLayout->setStretch(1, 10);
    mainLayout->setStretch(2, 2);
    mainLayout->setStretch(3, 0);
    mainLayout->setStretch(4, 0);
    mainLayout->setStretch(5, 0);

    connect(btnNewGame, &QPushButton::clicked, this, &InfoView::newGameRequested);
    connect(btnUndo, &QPushButton::clicked, this, &InfoView::undoRequested);
    connect(btnGiveUp, &QPushButton::clicked, this, &InfoView::giveUpRequested);
    connect(btnSettings, &QPushButton::clicked, this, &InfoView::openSettings);

    updateInfoPanel();
}

void InfoView::addMoveToDisplay(int movePly, const QString& move, Color color) {

    QFont labelFont;
    labelFont.setWeight(QFont::Bold);

    QFont btnFont;
    btnFont.setWeight(QFont::Bold);

    QFont resizedLabelFont = resizeFontSize(labelFont);
    QFont resizedBtnFont = resizeFontSize(btnFont);

    int currentCol = color == WHITE ? 1 : 2;

    QLayoutItem* item = movesLayout->itemAtPosition(currentRow, 0);

    bool needsNumber = item == nullptr;

    if (needsNumber) {

        if (currentRow % 2 == 0) {
            QFrame* rowBg = new QFrame();
            rowBg->setStyleSheet("background-color: rgba(255, 255, 255, 0.05); border-radius: 4px;");
            movesLayout->addWidget(rowBg, currentRow, 0, 1, 3);
        }

        int rowNumber = movePly / 2 + 1;
        QLabel* numLabel = new QLabel(QString::number(rowNumber) + ".");
        numLabel->setStyleSheet(numStyle);
        numLabel->setAlignment(Qt::AlignCenter);
        movesLayout->addWidget(numLabel, currentRow, 0);
        numLabel->setFont(resizedLabelFont);
        numLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    QPushButton* moveBtn = createMoveButton(move, movePly);

    connect(moveBtn, &QPushButton::clicked, this, [this, moveBtn]() {
        int targetPly = moveBtn->property("ply").toInt();
        emit reviewRequested(targetPly);
    });

    movesLayout->addWidget(moveBtn, currentRow, currentCol, Qt::AlignCenter);

    if (color == BLACK) {
        currentRow++;
    }

    buttonAmount++;
}

void InfoView::removeLastButFromDisplay() {

    if (buttonAmount < 1)
        return;

    int lastIndex = movesLayout->count() - 1;
    int row, col, rowSpan, colSpan;
    movesLayout->getItemPosition(lastIndex, &row, &col, &rowSpan, &colSpan);

    QLayoutItem* item = movesLayout->takeAt(lastIndex);
    if (item) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if (col == 1) {
        while (movesLayout->count() > 0) {
            int nextIndex = movesLayout->count() - 1;
            int r, c, rs, cs;
            movesLayout->getItemPosition(nextIndex, &r, &c, &rs, &cs);

            if (r == row) {
                QLayoutItem* nextItem = movesLayout->takeAt(nextIndex);
                if (nextItem) {
                    if (QWidget* w = nextItem->widget()) {
                        w->deleteLater();
                    }
                    delete nextItem;
                }
            } else {
                break;
            }
        }
    }
    else{
        currentRow--;
    }

    buttonAmount--;
}

void InfoView::updateInfoPanel() {

    int scoreLabelSize = qMax(14, panelCurrentWidth / 12);
    QFont f = scoreLabel->font();
    f.setPixelSize(scoreLabelSize);
    scoreLabel->setFont(f);

    for (int i = 0; i < movesLayout->count(); ++i) {
        if (QLayoutItem* item = movesLayout->itemAt(i)) {
            if (QWidget* widget = item->widget()) {
                QFont bf = resizeFontSize(widget->font());
                widget->setFont(bf);

                if (QPushButton* btn = qobject_cast<QPushButton*>(widget)) {
                    btn->setMinimumWidth(getCurrentFontMinWidth());
                }
            }
        }
    }

    QFont mainButsFont = scoreLabel->font();
    QFont resizedMainButFont = resizeFontSize(mainButsFont);

    btnGiveUp->setFont(resizedMainButFont);
    btnNewGame->setFont(resizedMainButFont);
    btnUndo->setFont(resizedMainButFont);

    if (gameRes != GameResult::GAME_DID_NOT_END) {

        int iSize = qMax(24, panelCurrentWidth / 7);
        QSize iconSize(iSize, iSize);

        QIcon wIcon(":/resources/resources/white_king.png");
        QIcon bIcon(":/resources/resources/black_king.png");

        whiteKing->setPixmap(wIcon.pixmap(iconSize));
        blackKing->setPixmap(bIcon.pixmap(iconSize));

        QFont sf = statusLabel->font();
        sf.setPixelSize(qMax(10, panelCurrentWidth / 22));
        statusLabel->setFont(sf);
    }
}

void InfoView::writeGameResultToDisplay(GameResult gr) {

    gameRes = gr;

    switch(gr){
    case GameResult::WHITE_WON:
        scoreLabel->setText("1 - 0");
        statusLabel->setText("Világos nyert");
        break;

    case GameResult::BLACK_WON:
        scoreLabel->setText("0 - 1");
        statusLabel->setText("Fekete nyert");
        break;

    case GameResult::DRAW:
        scoreLabel->setText("½ - ½");
        statusLabel->setText("Döntetlen");
        break;

    default:
        break;
    }

    resultBox->setVisible(true);
    updateInfoPanel();
}

void InfoView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    panelCurrentWidth = this->width();
    updateInfoPanel();
}

void InfoView::clearMoveDisplay()
{
    QLayoutItem* item;
    while ((item = movesLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    resultBox->setVisible(false);

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
