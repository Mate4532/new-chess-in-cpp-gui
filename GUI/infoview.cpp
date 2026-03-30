#include "infoview.h"
#include "chessscene.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QButtonGroup>

QFont InfoView::resizeFontSize(QFont f) {
    QFont mbf = f;
    mbf.setPixelSize(qMax(16, panelCurrentWidth / 20));
    return mbf;
}

int InfoView::getCurrentFontMinWidth() {
    int sSize = qMax(14, panelCurrentWidth / 15);
    return sSize * 3;
}

int InfoView::getCurrentIconSize() {
    int iconSizeVal = qMax(22, panelCurrentWidth / 14);
    return iconSizeVal;
}

InfoView::InfoView(AllSettings& allS, QWidget* parent) : currentAllS(allS), QWidget(parent)
{
    setupGeneralSettings();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    setupHeader(mainLayout);
    setupMoveListArea(mainLayout);
    setupResultArea(mainLayout);
    setupNavigationArea(mainLayout);
    setupActionButtons(mainLayout);

    mainLayout->setStretch(1, 10);
    mainLayout->setStretch(2, 2);

    clearMoveDisplay();
    updateInfoPanel();
}

void InfoView::setupGeneralSettings() {
    setMinimumWidth(MIN_WIDTH);
    this->setObjectName("InfoPanel");
    this->setAttribute(Qt::WA_StyledBackground, true);
    panelCurrentWidth = this->width();

    this->setStyleSheet(R"(
    #InfoPanel {
        background-color: rgb(49, 46, 43);
        border-radius: 10px;
    }
)");
}

void InfoView::setupHeader(QVBoxLayout* layout) {
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
    QPushButton:hover { color: #ffffff; }
    QPushButton:pressed { color: #B48866; padding-top: 2px; padding-left: 2px; }
)";

    btnSettings->setStyleSheet(settingsStyle);
    headerLayout->addWidget(btnSettings);
    layout->addLayout(headerLayout);

    connect(btnSettings, &QPushButton::clicked, this, &InfoView::openSettings);
}

void InfoView::scrollToMove(int ply) {
    if (!moveScrollArea || !moveScrollArea->verticalScrollBar()) return;

    if (ply <= 0) {
        moveScrollArea->verticalScrollBar()->setValue(0);
        return;
    }

    QPushButton* targetBtn = nullptr;
    for (auto btn : moveButtonGroup->buttons()) {
        if (btn->property("ply").toInt() == ply) {
            targetBtn = qobject_cast<QPushButton*>(btn);
            break;
        }
    }

    if (!targetBtn) return;

    int idx = movesLayout->indexOf(targetBtn);
    int row, col, rs, cs;
    movesLayout->getItemPosition(idx, &row, &col, &rs, &cs);

    QFrame* rowFrame = nullptr;
    QLayoutItem* item = movesLayout->itemAtPosition(row, 0);
    if (item && item->widget()) {
        rowFrame = qobject_cast<QFrame*>(item->widget());
    }

    QWidget* targetWidget = rowFrame ? static_cast<QWidget*>(rowFrame) : static_cast<QWidget*>(targetBtn);

    int top = targetWidget->pos().y();
    int bottom = top + targetWidget->height();
    int viewTop = moveScrollArea->verticalScrollBar()->value();
    int viewBottom = viewTop + moveScrollArea->viewport()->height();

    if (top < viewTop) {
        moveScrollArea->verticalScrollBar()->setValue(top);
    }
    else if (bottom > viewBottom) {
        moveScrollArea->verticalScrollBar()->setValue(bottom - moveScrollArea->viewport()->height());
    }
}

void InfoView::setupMoveListArea(QVBoxLayout* layout) {
    moveScrollArea = new QScrollArea(this);
    moveScrollArea->setWidgetResizable(true);
    moveScrollArea->setStyleSheet(R"(
        QScrollArea { border: none; background-color: transparent; }
        QScrollBar:vertical { background: #2a2724; width: 10px; border-radius: 5px; }
        QScrollBar::handle:vertical { background-color: #555555; min-height: 20px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background-color: #888888; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");

    QScrollBar* vScrollBar = moveScrollArea->verticalScrollBar();
    connect(vScrollBar, &QScrollBar::rangeChanged, this, [this, vScrollBar](int min, int max) {
        if (!currentAllS.robotSettings.isBotVsBot || currentReviewPly == buttonAmount) {
            scrollToMove(buttonAmount);
        }
    });

    moveButtonGroup = new QButtonGroup(this);
    moveButtonGroup->setExclusive(true);

    QWidget* scrollContent = new QWidget();
    scrollContent->setObjectName("scrollContent");
    scrollContent->setStyleSheet("background-color: transparent;");

    movesLayout = new QGridLayout(scrollContent);
    movesLayout->setAlignment(Qt::AlignTop);
    movesLayout->setColumnStretch(0, 4);
    movesLayout->setColumnStretch(1, 5);
    movesLayout->setColumnStretch(2, 5);
    movesLayout->setSpacing(0);
    movesLayout->setContentsMargins(0, 0, 0, 0);

    moveScrollArea->setWidget(scrollContent);
    layout->addWidget(moveScrollArea);
}

void InfoView::setupResultArea(QVBoxLayout* layout) {
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

    layout->addWidget(resultBox);
}

void InfoView::setupNavigationArea(QVBoxLayout* layout) {
    QHBoxLayout* navLayout = new QHBoxLayout();

    btnFirst = new QPushButton("«");
    btnPrev = new QPushButton("‹");
    btnNext = new QPushButton("›");
    btnLast = new QPushButton("»");

    navButtonGroup = new QButtonGroup(this);
    navButtonGroup->addButton(btnFirst, 0);
    navButtonGroup->addButton(btnPrev, 1);
    navButtonGroup->addButton(btnNext, 2);
    navButtonGroup->addButton(btnLast, 3);

    QString navStyle = R"(
        QPushButton {
            background-color: #262421;
            color: #aaaaaa;
            border: none;
            border-radius: 4px;
            font-size: 26px;
            font-weight: bold;
            padding-bottom: 6px;
            min-width: 50px;
        }
        QPushButton:hover {
            background-color: #3d3a37;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #1a1917;
        }
    )";

    for (auto btn : {btnFirst, btnPrev, btnNext, btnLast}) {
        btn->setStyleSheet(navStyle);
        btn->setCursor(Qt::PointingHandCursor);
        navLayout->addWidget(btn);
    }

    layout->addLayout(navLayout);
    connect(navButtonGroup, &QButtonGroup::idClicked, this, &InfoView::handleNavigation);
}

void InfoView::setupActionButtons(QVBoxLayout* layout) {
    btnNewGame = new QPushButton("Új Játék");
    btnUndo = new QPushButton("Visszavonás");
    btnGiveUp = new QPushButton("Feladás");

    btnNewGame->setStyleSheet(mainButtonStyle);
    btnUndo->setStyleSheet(mainButtonStyle);
    btnGiveUp->setStyleSheet(mainButtonStyle);

    layout->addWidget(btnNewGame);
    layout->addWidget(btnUndo);
    layout->addWidget(btnGiveUp);

    connect(btnNewGame, &QPushButton::clicked, this, [this]() {
        static QElapsedTimer timer;
        if (timer.isValid() && !timer.hasExpired(500)) return;
        timer.restart();
        emit newGameRequested();
    });
    connect(btnUndo, &QPushButton::clicked, this, &InfoView::undoRequested);
    connect(btnGiveUp, &QPushButton::clicked, this, &InfoView::giveUpRequested);
}

void InfoView::handleNavigation(int id) {
    int target = currentReviewPly;
    if (id == 0) target = 0;
    else if (id == 1) target = qMax(0, currentReviewPly - 1);
    else if (id == 2) target = qMin(buttonAmount, currentReviewPly + 1);
    else if (id == 3) target = buttonAmount;

    if (target != currentReviewPly && buttonAmount > 0) {
        currentReviewPly = target;
        emit reviewRequested(target);

        scrollToMove(target);

        if (target == 0) {
            moveButtonGroup->setExclusive(false);
            if (auto checked = moveButtonGroup->checkedButton()) {
                checked->setChecked(false);
            }
            moveButtonGroup->setExclusive(true);
        }
        else {
            for (auto btn : moveButtonGroup->buttons()) {
                if (btn->property("ply").toInt() == target) {
                    btn->setChecked(true);
                    break;
                }
            }
        }
    }
}

QPushButton* InfoView::createMoveButton(const QString& move, int movePly, Color pieceColor, PieceType movedPiece) {
    QFont btnFont;
    btnFont.setWeight(QFont::Bold);

    QPushButton* moveBtn = new QPushButton(move);
    moveBtn->setCheckable(true);
    moveBtn->setStyleSheet(moveBtnStyle);
    moveBtn->setCursor(Qt::PointingHandCursor);
    moveBtn->setProperty("ply", movePly);
    moveBtn->setFont(resizeFontSize(btnFont));
    moveBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QString iconPath = (pieceColor == WHITE) ? ChessScene::whitePieceMap.at(movedPiece) : ChessScene::blackPieceMap.at(movedPiece);
    int iconSizeVal = getCurrentIconSize();

    if (movedPiece != PAWN && movedPiece != KING) {
        moveBtn->setIcon(QIcon(iconPath));
        moveBtn->setIconSize(QSize(iconSizeVal, iconSizeVal));
    }
    else{
        moveBtn->setStyleSheet(moveBtn->styleSheet() + "QPushButton { padding-left: 5px; padding-right: 5px; }");
    }

    moveButtonGroup->addButton(moveBtn);
    return moveBtn;
}

QPushButton* InfoView::getLastButton(int* r, int* c) {
    for (int i = movesLayout->count() - 1; i >= 0; --i) {
        if (auto btn = qobject_cast<QPushButton*>(movesLayout->itemAt(i)->widget())) {
            int rs, cs;
            if (r && c) movesLayout->getItemPosition(i, r, c, &rs, &cs);
            return btn;
        }
    }
    return nullptr;
}

void InfoView::clearRowWidgets(int row) {
    for (int i = movesLayout->count() - 1; i >= 0; --i) {
        int r, c, rs, cs;
        movesLayout->getItemPosition(i, &r, &c, &rs, &cs);
        if (r == row) {
            if (auto item = movesLayout->takeAt(i)) {
                if (item->widget()) item->widget()->deleteLater();
                delete item;
            }
        }
    }
}

void InfoView::addMoveToDisplay(int movePly, const QString& move, Color color, PieceType movedPiece) {
    int row = (movePly + 1) / 2;
    int col = (color == WHITE) ? 1 : 2;

    currentRow = row;

    if (!movesLayout->itemAtPosition(row, 0)) {
        if (row % 2 == 1) {
            QFrame* rowBg = new QFrame();
            rowBg->setObjectName("rowBg_" + QString::number(row));
            rowBg->setStyleSheet("background-color: rgba(255, 255, 255, 0.05); border-radius: 4px;");
            movesLayout->addWidget(rowBg, row, 0, 1, 3);
        }

        QLabel* numLabel = new QLabel(QString::number(movePly / 2 + 1) + ".");
        numLabel->setStyleSheet(numStyle);
        numLabel->setAlignment(Qt::AlignLeft);
        numLabel->setFont(resizeFontSize(numLabel->font()));
        movesLayout->addWidget(numLabel, row, 0);
    }

    QPushButton* moveBtn = createMoveButton(move, movePly, color, movedPiece);

    QString activeColor = "#5C5C5C";
    moveBtn->setStyleSheet(moveBtn->styleSheet() + QString("QPushButton:checked { background-color: %1; }").arg(activeColor));

    connect(moveBtn, &QPushButton::clicked, this, [this, moveBtn]() {
        currentReviewPly = moveBtn->property("ply").toInt();
        emit reviewRequested(currentReviewPly);
        scrollToMove(currentReviewPly);
    });

    movesLayout->addWidget(moveBtn, row, col, Qt::AlignLeft | Qt::AlignVCenter);
    if (!currentAllS.robotSettings.isBotVsBot || currentReviewPly == buttonAmount) {
        moveBtn->setChecked(true);
        currentReviewPly = movePly;
    }

    buttonAmount++;
}

void InfoView::removeLastButFromDisplay() {
    int r, c;
    QPushButton* lastBtn = getLastButton(&r, &c);
    if (!lastBtn) return;

    moveButtonGroup->removeButton(lastBtn);

    for (int i = 0; i < movesLayout->count(); ++i) {
        if (movesLayout->itemAt(i)->widget() == lastBtn) {
            QLayoutItem* item = movesLayout->takeAt(i);
            delete item->widget();
            delete item;
            break;
        }
    }

    buttonAmount--;

    bool hasRemainingButton = false;
    for (int i = 0; i < movesLayout->count(); ++i) {
        int row, col, rowSpan, colSpan;
        movesLayout->getItemPosition(i, &row, &col, &rowSpan, &colSpan);

        if (row == r) {
            QWidget* w = movesLayout->itemAt(i)->widget();
            if (qobject_cast<QPushButton*>(w)) {
                hasRemainingButton = true;
                break;
            }
        }
    }

    if (!hasRemainingButton) {
        clearRowWidgets(r);
        currentRow = qMax(0, r - 1);
    } else {
        currentRow = r;
    }

    if (auto nextLast = getLastButton()) {
        nextLast->setChecked(true);
        currentReviewPly = nextLast->property("ply").toInt();
    } else {
        currentReviewPly = 0;
        currentRow = 0;
    }

    if (movesLayout->parentWidget()) {
        movesLayout->parentWidget()->adjustSize();
    }
}

void InfoView::updateInfoPanel() {
    int scoreLabelSize = qMax(14, panelCurrentWidth / 12);
    QFont f = scoreLabel->font();
    f.setPixelSize(scoreLabelSize);
    scoreLabel->setFont(f);

    for (int i = 0; i < movesLayout->count(); ++i) {
        if (QLayoutItem* item = movesLayout->itemAt(i)) {
            if (QWidget* widget = item->widget()) {
                widget->setFont(resizeFontSize(widget->font()));
                if (QPushButton* btn = qobject_cast<QPushButton*>(widget)) {
                    int iconSizeVal = getCurrentIconSize();
                    btn->setIconSize(QSize(iconSizeVal, iconSizeVal));
                }
            }
        }
    }

    QFont resizedMainButFont = resizeFontSize(scoreLabel->font());
    btnGiveUp->setFont(resizedMainButFont);
    btnNewGame->setFont(resizedMainButFont);
    btnUndo->setFont(resizedMainButFont);

    if (gameRes != GameResult::GAME_DID_NOT_END) {
        int iSize = qMax(24, panelCurrentWidth / 7);
        QSize iconSize(iSize, iSize);
        whiteKing->setPixmap(QIcon(":/resources/resources/white_king.png").pixmap(iconSize));
        blackKing->setPixmap(QIcon(":/resources/resources/black_king.png").pixmap(iconSize));

        QFont sf = statusLabel->font();
        sf.setPixelSize(qMax(10, panelCurrentWidth / 22));
        statusLabel->setFont(sf);
    }
}

void InfoView::writeGameResultToDisplay(GameResult gr) {
    gameRes = gr;
    if (gr == GameResult::WHITE_WON) { scoreLabel->setText("1 - 0"); statusLabel->setText("Világos nyert"); }
    else if (gr == GameResult::BLACK_WON) { scoreLabel->setText("0 - 1"); statusLabel->setText("Fekete nyert"); }
    else if (gr == GameResult::DRAW) { scoreLabel->setText("½ - ½"); statusLabel->setText("Döntetlen"); }

    resultBox->setVisible(true);
    updateInfoPanel();
}

void InfoView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    panelCurrentWidth = this->width();
    updateInfoPanel();
}


void InfoView::clearMoveDisplay() {
    QLayoutItem* item;
    while ((item = movesLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) { w->hide(); w->deleteLater(); }
        delete item;
    }
    movesLayout->invalidate();
    if (QWidget* contentWidget = movesLayout->parentWidget()) contentWidget->adjustSize();

    resultBox->setVisible(false);
    currentRow = 0;
    buttonAmount = 0;
    currentReviewPly = 0;
    gameRes = GameResult::GAME_DID_NOT_END;
}

void InfoView::onReviewEnded(int ply) {
    currentReviewPly = ply;

    if (ply <= 0) {
        moveButtonGroup->setExclusive(false);
        if (QAbstractButton* checked = moveButtonGroup->checkedButton()) {
            checked->setChecked(false);
        }
        moveButtonGroup->setExclusive(true);
    } else {
        const QList<QAbstractButton*> buttons = moveButtonGroup->buttons();
        for (QAbstractButton* btn : buttons) {
            if (btn->property("ply").toInt() == ply) {
                btn->setChecked(true);
                break;
            }
        }
    }

    scrollToMove(ply);
}

void InfoView::openSettings() {
    AllSettings oldAllS = currentAllS;
    SettingsDialog dlg(currentAllS, this);
    if (dlg.exec() == QDialog::Accepted) emit settingsChanged(oldAllS, currentAllS);
}
