#include "Playerpanel.h"

const std::unordered_map<PieceType, QString> PlayerPanel::whitePieceMap = {
    {PieceType::PAWN, ":/resources/resources/white_pawn.png"},
    {PieceType::ROOK, ":/resources/resources/white_rook.png"},
    {PieceType::KNIGHT, ":/resources/resources/white_knight.png"},
    {PieceType::BISHOP, ":/resources/resources/white_bishop.png"},
    {PieceType::QUEEN, ":/resources/resources/white_queen.png"},
    {PieceType::KING, ":/resources/resources/white_king.png"},
    };

const std::unordered_map<PieceType, QString> PlayerPanel::blackPieceMap = {
    {PieceType::PAWN, ":/resources/resources/black_pawn.png"},
    {PieceType::ROOK, ":/resources/resources/black_rook.png"},
    {PieceType::KNIGHT, ":/resources/resources/black_knight.png"},
    {PieceType::BISHOP, ":/resources/resources/black_bishop.png"},
    {PieceType::QUEEN, ":/resources/resources/black_queen.png"},
    {PieceType::KING, ":/resources/resources/black_king.png"},
    };

PlayerPanel::PlayerPanel(PlayerInfo& player, bool isTimerLabelVisible, QWidget* parent) : QHBoxLayout(parent) {

    playerColor = player.getPlayerColor();
    preloadPixmaps();

    this->setSpacing(0);
    this->setContentsMargins(0, 0, 0, 0);

    playerIconLabel = new QLabel();
    playerLabel = new QLabel();
    piecesContainer = new QWidget();
    materialScoreLabel = new QLabel();
    piecesContainer->setFixedHeight(25);

    timerLabel = new QLabel("00:00");

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

    setTimerActive(false);

    QSizePolicy sp = timerLabel->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    timerLabel->setSizePolicy(sp);

    timerLabel->setAlignment(Qt::AlignCenter);
    timerLabel->setVisible(isTimerLabelVisible);

    this->addWidget(playerIconLabel, 0, Qt::AlignCenter);
    this->addWidget(playerLabel, 0, Qt::AlignCenter);
    this->addWidget(piecesContainer, 0, Qt::AlignCenter);
    this->addWidget(materialScoreLabel, 0, Qt::AlignCenter);

    this->addStretch();

    this->addWidget(timerLabel, 0, Qt::AlignRight | Qt::AlignVCenter);

    setPlayerName(player.getPlayerName());
    setPlayerIcon(player.getPlayerIcon());
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

QString PlayerPanel::getIconPathForPiece(PieceType piece) {
    if (playerColor == WHITE) {
        auto it = blackPieceMap.find(piece);
        if (it != blackPieceMap.end()) return it->second;
    }
    else {
        auto it = whitePieceMap.find(piece);
        if (it != whitePieceMap.end()) return it->second;
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
    orderPieces(orderedTakenPieces);

    int currentX = 0;

    for (int i = 0; i < orderedTakenPieces.size(); ++i) {
        QLabel* pieceLabel = new QLabel(piecesContainer);

        QString iconPath = getIconPathForPiece(orderedTakenPieces[i]);

        QPixmap pix;

        auto it = loadedPixmaps.find(iconPath);
        if (it != loadedPixmaps.end()) {
            pix = it->second;
        } else {
            pix = QPixmap(iconPath);
            qWarning() << "Cache miss for:" << iconPath;
        }

        pieceLabel->setPixmap(pix.scaled(playerPanelPieceSide, playerPanelPieceSide, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pieceLabel->setFixedSize(playerPanelPieceSide, playerPanelPieceSide);

        pieceLabel->move(currentX, 0);
        pieceLabel->show();
        takenPiecesLabels.push_back(pieceLabel);

        if (i < orderedTakenPieces.size() - 1) {
            if (orderedTakenPieces[i] == orderedTakenPieces[i+1]) {
                currentX += samePiecesDistanePx[orderedTakenPieces[i]];
            }
            else {
                currentX += diffPiecesDistanePx[orderedTakenPieces[i]];
            }
        } else {
            currentX += diffPiecesDistanePx[orderedTakenPieces[i]];
        }
    }

    piecesContainer->setFixedSize(currentX, playerPanelPieceSide);
}

void PlayerPanel::syncPiecesWithPanel(int pieces[6]) {

    clearPanel();

    for (int type = PAWN; type < KING; ++type) {

        int count = pieces[type];

        for (int i = 0; i < count; ++i) {
            orderedTakenPieces.push_back(static_cast<PieceType>(type));
        }
    }

    updatePanel();
}

void PlayerPanel::setTimerVisibility(bool isVisible) {
    if (timerLabel) {
        timerLabel->setVisible(isVisible);
    }
}

void PlayerPanel::setTimerActive(bool isActive) {
    if (!timerLabel) return;

    if (playerColor == WHITE) {
        if (isActive) {
            timerLabel->setStyleSheet(R"(
            QLabel {
                background-color: #f0f0f0;
                color: #262421;
                font-family: 'Segoe UI', 'Helvetica Neue', sans-serif;
                font-size: 20px;
                font-weight: 800;
                border-radius: 6px;
                padding: 8px 18px;
                margin-right: 15px;
            }
        )");
        } else {
            timerLabel->setStyleSheet(R"(
            QLabel {
                background-color: #7a7a7a;
                color: #c0c0c0;
                font-family: 'Segoe UI', 'Helvetica Neue', sans-serif;
                font-size: 20px;
                font-weight: 800;
                border-radius: 6px;
                padding: 8px 18px;
                margin-right: 15px;
            }
        )");
        }
    } else {
        if (isActive) {
            timerLabel->setStyleSheet(R"(
            QLabel {
                background-color: #262421;
                color: #f0f0f0;
                font-family: 'Segoe UI', 'Helvetica Neue', sans-serif;
                font-size: 20px;
                font-weight: 800;
                border-radius: 6px;
                padding: 8px 18px;
                margin-right: 15px;
            }
        )");
        } else {
            timerLabel->setStyleSheet(R"(
            QLabel {
                background-color: #1a1a1a;
                color: #6e6e6e;
                font-family: 'Segoe UI', 'Helvetica Neue', sans-serif;
                font-size: 20px;
                font-weight: 800;
                border-radius: 6px;
                padding: 8px 18px;
                margin-right: 15px;
            }
        )");
        }
    }
}


void PlayerPanel::setTimerText(const QString& timeStr) {
    if (timerLabel) {
        timerLabel->setText(timeStr);
    }
}

void PlayerPanel::clearPanel() {
    deletePieceLabels();
    orderedTakenPieces.clear();

    materialScoreLabel->setText("");
}

int PlayerPanel::getMaterialScore() {

    int sum = 0;

    for (PieceType piece : orderedTakenPieces) {
        sum += getPieceValue(piece);
    }

    return sum;
}


void PlayerPanel::preloadPixmaps()
{
    auto loadMap = [&](const std::unordered_map<PieceType, QString>& map) {
        for (const auto& [type, res] : map) {
            if (loadedPixmaps.find(res) == loadedPixmaps.end()) {
                QPixmap pix(res);
                if (!pix.isNull()) {
                    loadedPixmaps.emplace(res, pix);
                } else {
                    qWarning() << "Failed to preload pixmap:" << res;
                }
            }
        }
    };

    Color enemyPieceColor = playerColor == WHITE ? BLACK : WHITE;

    if (enemyPieceColor == WHITE) loadMap(whitePieceMap);
    else loadMap(blackPieceMap);
}

