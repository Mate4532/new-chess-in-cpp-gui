#include "playerpanel.h"
#include <QLabel>

PlayerPanel::PlayerPanel(Color playerColor, QString playerName, QString iconPath, QWidget* parent) : QHBoxLayout(parent) {

    this->playerColor = playerColor;
    this->setSpacing(0);
    this->setContentsMargins(0, 0, 0, 0);

    playerIconLabel = new QLabel();
    playerLabel = new QLabel();
    piecesContainer = new QWidget();
    materialScoreLabel = new QLabel();
    piecesContainer->setFixedHeight(25);

    playerLabel->setStyleSheet(R"(
        QLabel {
            color: #d5d5d5;
            font-family: 'Segoe UI', sans-serif;
            font-size: 17px;
            font-weight: 600;
            padding-left: 8px;
            padding-right: 8px;
        }
    )");

    materialScoreLabel->setStyleSheet(R"(
        QLabel {
            color: #d5d5d5;
            font-family: 'Segoe UI', sans-serif;
            font-size: 14px;
            font-weight: 700;
            padding-left: 5px;
        }
    )");


    this->addWidget(playerIconLabel, 0, Qt::AlignCenter);
    this->addWidget(playerLabel, 0, Qt::AlignCenter);
    this->addWidget(piecesContainer, 0, Qt::AlignCenter);
    this->addWidget(materialScoreLabel, 0, Qt::AlignCenter);
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

int PlayerPanel::getPieceValue(PieceType p) {
    switch (p) {
    case PAWN:   return 1;
    case KNIGHT: return 3;
    case BISHOP: return 3;
    case ROOK:   return 5;
    case QUEEN:  return 9;
    default:     return 0;
    }
    return 0;
}

int PlayerPanel::getPieceOrder(PieceType p) {
    switch (p) {
    case PAWN:   return 1;
    case KNIGHT: return 2;
    case BISHOP: return 3;
    case ROOK:   return 4;
    case QUEEN:  return 5;
    default:     return 0;
    }
    return 0;
}

void PlayerPanel::orderPieces(std::vector<PieceType>& pieces) {
    std::sort(pieces.begin(), pieces.end(), [this](PieceType a, PieceType b) {
        return getPieceOrder(a) > getPieceOrder(b);
    });
}

std::string PlayerPanel::getIconPathForPiece(PieceType piece) {
    std::string capturedPieceIconPath = "";

    switch (piece) {
    case PieceType::PAWN:
        return capturedPieceIconPath = playerColor == WHITE ? ":/resources/resources/black_pawn.png" : ":/resources/resources/white_pawn.png";

    case PieceType::KNIGHT:
        return capturedPieceIconPath = playerColor == WHITE ? ":/resources/resources/black_knight.png" : ":/resources/resources/white_knight.png";

    case PieceType::BISHOP:
        return capturedPieceIconPath = playerColor == WHITE ? ":/resources/resources/black_bishop.png" : ":/resources/resources/white_bishop.png";

    case PieceType::ROOK:
        return capturedPieceIconPath = playerColor == WHITE ? ":/resources/resources/black_rook.png" : ":/resources/resources/white_rook.png";

    case PieceType::QUEEN:
        return capturedPieceIconPath = playerColor == WHITE ? ":/resources/resources/black_queen.png" : ":/resources/resources/white_queen.png";

    case PieceType::KING:
        return capturedPieceIconPath = playerColor == WHITE ? ":/resources/resources/black_king.png" : ":/resources/resources/white_king.png";

    default:
        break;
    }

    return "";
}

void PlayerPanel::deletePieceLabels() {
    for (QLabel* label : takenPiecesLabels) {
        delete label;
    }
    takenPiecesLabels.clear();
}

void PlayerPanel::updateMaterialScore(int scoreDiff) {

    bool playerLeading = (playerColor == WHITE && scoreDiff > 0) ||
                      (playerColor == BLACK && scoreDiff < 0);

    if (scoreDiff == 0 || !playerLeading) {
        materialScoreLabel->setText("");
        return;
    }

    materialScoreLabel->setText("+" + QString::number(abs(scoreDiff)));
}

void PlayerPanel::updatePanel() {

    deletePieceLabels();
    orderPieces(orderedPieces);

    int currentX = 0;

    for (int i = 0; i < orderedPieces.size(); ++i) {
        QLabel* pieceLabel = new QLabel(piecesContainer);
        QString iconPath = QString::fromStdString(getIconPathForPiece(orderedPieces[i]));

        QPixmap pix(iconPath);
        pieceLabel->setPixmap(pix.scaled(playerPanelPieceSide, playerPanelPieceSide, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pieceLabel->setFixedSize(playerPanelPieceSide, playerPanelPieceSide);

        pieceLabel->move(currentX, 0);
        pieceLabel->show();
        takenPiecesLabels.push_back(pieceLabel);

        if (i < orderedPieces.size() - 1) {
            if (orderedPieces[i] == orderedPieces[i+1]) {
                currentX += samePiecesDistanePx[orderedPieces[i]];
            }
            else {
                currentX += diffPiecesDistanePx[orderedPieces[i]];
            }
        } else {
            currentX += diffPiecesDistanePx[orderedPieces[i]];
        }
    }

    piecesContainer->setFixedSize(currentX, playerPanelPieceSide);
}

void PlayerPanel::addPieceToPanel(PieceType piece) {
    if (piece == PieceType::PIECE_NONE) return;

    orderedPieces.push_back(piece);
    updatePanel();
}

void PlayerPanel::removePieceFromPanel(PieceType piece) {
    if (piece == PieceType::PIECE_NONE) return;

    auto it = std::find(orderedPieces.begin(), orderedPieces.end(), piece);
    if (it != orderedPieces.end()) {
        orderedPieces.erase(it);
    }

    updatePanel();
}

void PlayerPanel::clearPanel() {
    deletePieceLabels();
    orderedPieces.clear();
    materialScoreLabel->setText("");
}

int PlayerPanel::getMaterialScore() {

    int sum = 0;

    for (PieceType p : orderedPieces) {
        sum += getPieceValue(p);
    }

    return sum;
}
