#include "boardAndPlayerPanel.h"
#include <iostream>

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

void BoardAndPlayerPanel::addPieceToPlayerPanel(Color playerColor, PieceType piece) {
    playerColor == WHITE ? whitePlayer->addPieceToPanel(piece) : blackPlayer->addPieceToPanel(piece);
}

void BoardAndPlayerPanel::removePiecesFromPanel(Color playerColor, PieceType piece) {
    playerColor == WHITE ? whitePlayer->removePieceFromPanel(piece) : blackPlayer->removePieceFromPanel(piece);
}

void BoardAndPlayerPanel::updateMaterialScore(std::vector<std::pair<PieceType, Color>> pieces) {

    int piecesArray[2][6] = {{0}};

    for (auto piece : pieces) {

        PieceType pieceType = piece.first;
        Color pieceColor = piece.second;

        piecesArray[pieceColor][pieceType]++;
    }

    std::vector<PieceType> whiteStartingPieces;
    std::vector<PieceType> blackStartingPieces;

    for (int pieceType = PAWN; pieceType <= KING; ++pieceType) {

        for (int i = 0; i < piecesArray[WHITE][pieceType]; ++i) {
            whiteStartingPieces.push_back(static_cast<PieceType>(pieceType));
        }
        for (int i = 0; i < piecesArray[BLACK][pieceType]; ++i) {
            blackStartingPieces.push_back(static_cast<PieceType>(pieceType));
        }
    }

    int whiteSum = whitePlayer->getMaterialScore(whiteStartingPieces);
    int blackSum = blackPlayer->getMaterialScore(blackStartingPieces);

    int scoreDiff = whiteSum - blackSum;

    whitePlayer->updateMaterialScore(scoreDiff);
    blackPlayer->updateMaterialScore(scoreDiff);
}

void BoardAndPlayerPanel:: clearPanels() {
    whitePlayer->clearPanel();
    blackPlayer->clearPanel();
}
