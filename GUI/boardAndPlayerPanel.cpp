#include "boardAndPlayerPanel.h"

BoardAndPlayerPanel::BoardAndPlayerPanel(PlayerPanel* whitePlayer, PlayerPanel* blackPlayer, ChessView* cv, QWidget* parent)
    : QWidget(parent)
{
    this->whitePlayer = whitePlayer;
    this->blackPlayer = blackPlayer;
    this->chessView = cv;

    mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(10, 10, 10, 10);

    mainLay->setAlignment(Qt::AlignCenter);

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

int BoardAndPlayerPanel::updateSize(int availableWidth, int availableHeight) {
    int marginsH = mainLay->contentsMargins().left() + mainLay->contentsMargins().right();
    int marginsV = mainLay->contentsMargins().top() + mainLay->contentsMargins().bottom();
    int spacings = mainLay->spacing() * 2;

    int pHeight = blackPlayer->sizeHint().height() + whitePlayer->sizeHint().height();
    if (pHeight < 20) pHeight = 100;

    int maxBoardW = availableWidth - marginsH;
    int maxBoardH = availableHeight - marginsV - spacings - pHeight;

    int side = qMax(100, qMin(maxBoardW, maxBoardH));

    chessView->setMaximumSize(side, side);

    int finalWidth = side + marginsH;
    int finalHeight = side + marginsV + spacings + pHeight;

    this->setMaximumSize(finalWidth, finalHeight);

    return finalWidth;
}

QSize BoardAndPlayerPanel::getMinimumOptimalSize() {
    int minBoardSide = ChessView::MIN_WIDTH;

    QMargins m = mainLay->contentsMargins();
    int pHeight = blackPlayer->sizeHint().height() + whitePlayer->sizeHint().height();
    int spacing = mainLay->spacing() * 2;

    int minW = minBoardSide + m.left() + m.right();
    int minH = minBoardSide + pHeight + m.top() + m.bottom() + spacing;

    return QSize(minW, minH);
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

void BoardAndPlayerPanel::syncPiecesWithPanels(int pieces[2][6]) {

    int displayPieces[2][6] = {{0}};

    for (int color = WHITE; color <= BLACK; ++color) {
        int promotedPawnsCount = 0;
        for (int type = KNIGHT; type <= QUEEN; ++type) {
            if (pieces[color][type] > basePieceCounts[type]) {
                promotedPawnsCount += (pieces[color][type] - basePieceCounts[type]);
            }
        }

        for (int type = PAWN; type <= KING; ++type) {

            int capturedCount = 0;

            if (type == PAWN) {
                int calc = basePieceCounts[PAWN] - pieces[color][PAWN] - promotedPawnsCount;
                capturedCount = std::max(0, calc);
            }
            else if (type != KING) {
                int calc = basePieceCounts[type] - pieces[color][type];
                capturedCount = std::max(0, calc);
            }

            Color opponent = (color == WHITE) ? BLACK : WHITE;
            displayPieces[opponent][type] = capturedCount;
        }
    }

    whitePlayer->syncPiecesWithPanel(displayPieces[WHITE]);
    blackPlayer->syncPiecesWithPanel(displayPieces[BLACK]);

    updateMaterialScoreBasedOnPieces(pieces);
}

void BoardAndPlayerPanel::onTimerChanged(Color playerColor, QString timerStr) {
    if (playerColor == WHITE) whitePlayer->setTimerText(timerStr);
    else blackPlayer->setTimerText(timerStr);
}

void BoardAndPlayerPanel::onSetTimerVisibility(Color playerColor, bool isVisible) {
    if (playerColor == WHITE) whitePlayer->setTimerVisibility(isVisible);
    else blackPlayer->setTimerVisibility(isVisible);
}

void BoardAndPlayerPanel::onActivateTimerColorAndDisableOther(Color timerToActivate) {
    whitePlayer->setTimerActive(timerToActivate == WHITE);
    blackPlayer->setTimerActive(timerToActivate == BLACK);
}

void BoardAndPlayerPanel::onDisableTimers() {
    whitePlayer->setTimerActive(false);
    blackPlayer->setTimerActive(false);
}

int BoardAndPlayerPanel::getMaterialScore(std::vector<PieceType> pieces) {

    int sum = 0;

    for (PieceType piece : pieces) {
        sum += PlayerPanel::getPieceValue(piece);
    }

    return sum;
}

void BoardAndPlayerPanel::updateMaterialScoreBasedOnPieces(int pieces[2][6]) {

    std::vector<PieceType> whitePieces;
    std::vector<PieceType> blackPieces;

    for (int pieceType = PAWN; pieceType <= KING; ++pieceType) {

        for (int i = 0; i < pieces[WHITE][pieceType]; ++i) {
            whitePieces.push_back(static_cast<PieceType>(pieceType));
        }
        for (int i = 0; i < pieces[BLACK][pieceType]; ++i) {
            blackPieces.push_back(static_cast<PieceType>(pieceType));
        }
    }

    int whiteSum = getMaterialScore(whitePieces);
    int blackSum = getMaterialScore(blackPieces);

    int scoreDiff = whiteSum - blackSum;

    whitePlayer->updateMaterialScore(scoreDiff);
    blackPlayer->updateMaterialScore(scoreDiff);
}

void BoardAndPlayerPanel:: clearPanels() {
    whitePlayer->clearPanel();
    blackPlayer->clearPanel();
}
