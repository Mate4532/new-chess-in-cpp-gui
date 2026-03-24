#include "chessscene.h"
#include "chesspieceitem.h"
#include "chessview.h"
#include "Utils.h"

#include <QDebug>
#include <QCursor>
#include <QGraphicsRectItem>

const std::unordered_map<PieceType, QString> ChessScene::whitePieceMap = {
    {PieceType::PAWN, ":/resources/resources/white_pawn.png"},
    {PieceType::ROOK, ":/resources/resources/white_rook.png"},
    {PieceType::KNIGHT, ":/resources/resources/white_knight.png"},
    {PieceType::BISHOP, ":/resources/resources/white_bishop.png"},
    {PieceType::QUEEN, ":/resources/resources/white_queen.png"},
    {PieceType::KING, ":/resources/resources/white_king.png"},
    };

const std::unordered_map<PieceType, QString> ChessScene::blackPieceMap = {
    {PieceType::PAWN, ":/resources/resources/black_pawn.png"},
    {PieceType::ROOK, ":/resources/resources/black_rook.png"},
    {PieceType::KNIGHT, ":/resources/resources/black_knight.png"},
    {PieceType::BISHOP, ":/resources/resources/black_bishop.png"},
    {PieceType::QUEEN, ":/resources/resources/black_queen.png"},
    {PieceType::KING, ":/resources/resources/black_king.png"},
    };

ChessScene::ChessScene(QObject* parent) : QGraphicsScene(parent)
{
    preloadPixmaps();
    connect(this, &QGraphicsScene::sceneRectChanged, this, &ChessScene::onSceneRectChanged);
    setSceneRect(0, 0, ChessView::WHOLE_CHESSBOARD_WIDTH_PX, ChessView::WHOLE_CHESSBOARD_HEIGHT_PX);
}

void ChessScene::setViewModel(ChessViewModel* cvm){
    this->cvm = cvm;
    updateLayout();
}

void ChessScene::preloadPixmaps()
{
    auto loadMap = [&](const std::unordered_map<PieceType, QString>& map) {
        for (const auto& [type, res] : map) {
            if (originalPixmaps.find(res) == originalPixmaps.end()) {
                QPixmap pix(res);
                if (!pix.isNull()) {
                    originalPixmaps.emplace(res, pix);
                } else {
                    qWarning() << "Failed to preload pixmap:" << res;
                }
            }
        }
    };
    loadMap(whitePieceMap);
    loadMap(blackPieceMap);
}

// PROFI MEGOLDÁS: A tábla belső területének arányos felosztása kerekítési hiba nélkül
QRectF ChessScene::getSquareRect(int visualCol, int visualRow) const {
    const double fullBoardSize = 8.0 * TILE_SIZE;

    // Kiszámoljuk az intervallumokat a teljes méret alapján
    double x1 = (static_cast<double>(visualCol) * fullBoardSize) / 8.0;
    double x2 = (static_cast<double>(visualCol + 1) * fullBoardSize) / 8.0;
    double y1 = (static_cast<double>(visualRow) * fullBoardSize) / 8.0;
    double y2 = (static_cast<double>(visualRow + 1) * fullBoardSize) / 8.0;

    return QRectF(
        CHESSBOARD_OFFSET_LEFT_PX + x1,
        CHESSBOARD_OFFSET_UP_PX + y1,
        x2 - x1,
        y2 - y1
        );
}

bool ChessScene::scenePosToSquare(const QPointF& pos, int& file, int& visualRank) const
{
    double xInsideBoard = pos.x() - CHESSBOARD_OFFSET_LEFT_PX;
    double yInsideBoard = pos.y() - CHESSBOARD_OFFSET_UP_PX;
    double fullBoardSize = 8.0 * TILE_SIZE;

    if (xInsideBoard < 0 || yInsideBoard < 0 || xInsideBoard >= fullBoardSize || yInsideBoard >= fullBoardSize)
        return false;

    int col = static_cast<int>((xInsideBoard * 8.0) / fullBoardSize);
    int row = static_cast<int>((yInsideBoard * 8.0) / fullBoardSize);

    file = cvm->getIsBoardFlipped() ? (7 - col) : col;
    visualRank = cvm->getIsBoardFlipped() ? (7 - row) : row;

    return true;
}

void ChessScene::drawMovedPieceBackground() {
    if (!cvm) return;

    MoveInfo lastMove = cvm->getMoveInfo();
    if (!lastMove.isValid) return;

    // Chess.com stílusú sárga kiemelés
    QColor highlightColor(246, 246, 105, 150);
    bool isFlipped = cvm->getIsBoardFlipped();

    auto highlightSquare = [&](int logicalFile, int logicalRank) {
        int visualCol = isFlipped ? (7 - logicalFile) : logicalFile;
        int visualRow = isFlipped ? logicalRank : (7 - logicalRank);

        QRectF rectArea = getSquareRect(visualCol, visualRow);

        // Minimális igazítás az élsimítás (anti-aliasing) miatt, hogy ne legyen rés
        QGraphicsRectItem* rect = new QGraphicsRectItem(rectArea.adjusted(-0.1, -0.1, 0.1, 0.1));
        rect->setBrush(QBrush(highlightColor));
        rect->setPen(Qt::NoPen);
        rect->setZValue(1); // Bábuk alatt

        addItem(rect);
    };

    highlightSquare(lastMove.fromFile, lastMove.fromRank);
    highlightSquare(lastMove.toFile, lastMove.toRank);
}

void ChessScene::drawPieces() {
    auto boardMatrix = cvm->getBoardMatrix();
    bool isFlipped = cvm->getIsBoardFlipped();
    const qreal qualityMultiplier = 1.5;

    for (int visualRow = 0; visualRow < 8; ++visualRow) {
        for (int visualCol = 0; visualCol < 8; ++visualCol) {

            int matrixRow = isFlipped ? (7 - visualRow) : visualRow;
            int matrixCol = isFlipped ? (7 - visualCol) : visualCol;

            PieceType type = boardMatrix[matrixRow][matrixCol].first;
            Color color = boardMatrix[matrixRow][matrixCol].second;

            if (type == PieceType::PIECE_NONE) continue;

            QString resource;
            if (color == Color::WHITE) {
                auto it = whitePieceMap.find(type);
                if (it != whitePieceMap.end()) resource = it->second;
            } else {
                auto it = blackPieceMap.find(type);
                if (it != blackPieceMap.end()) resource = it->second;
            }

            if (resource.isEmpty()) continue;

            QGraphicsPixmapItem* item = new ChessPieceItem();

            if (originalPixmaps.count(resource)) {
                int highResSize = static_cast<int>(PIECE_SIZE * qualityMultiplier);
                QPixmap scaled = originalPixmaps[resource].scaled(
                    highResSize, highResSize,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation
                    );
                scaled.setDevicePixelRatio(qualityMultiplier);
                item->setPixmap(scaled);
            }

            // A bábu pozícionálása a közös rács alapján
            QRectF square = getSquareRect(visualCol, visualRow);
            qreal offX = (square.width() - PIECE_SIZE) / 2.0;
            qreal offY = (square.height() - PIECE_SIZE) / 2.0;
            item->setPos(square.topLeft() + QPointF(offX, offY));

            int logicalRank = 7 - matrixRow;
            int logicalFile = matrixCol;

            item->setData(FileKey, logicalFile);
            item->setData(RankKey, logicalRank);
            item->setZValue(10); // Kiemelés felett

            addItem(item);
        }
    }
}

void ChessScene::updateLayout()
{
    if (!cvm) return;
    clear();
    activeItem = nullptr;
    drawMovedPieceBackground();
    drawPieces();
}

void ChessScene::onBoardChanged() {
    updateLayout();
}

void ChessScene::onSceneRectChanged(const QRectF& rect)
{
    Q_UNUSED(rect);
    updateLayout();
}

void ChessScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (activeItem) {
            activeItem->setPos(activeItemOriginalPos);
            activeItem->setZValue(10);
            activeItem->setCursor(Qt::OpenHandCursor);
            activeItem = nullptr;
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int clickedFile, clickedVisualRank;
        if (!scenePosToSquare(event->scenePos(), clickedFile, clickedVisualRank)) {
            QGraphicsScene::mousePressEvent(event);
            return;
        }

        int clickedLogicalRank = 7 - clickedVisualRank;
        QGraphicsPixmapItem* foundPiece = nullptr;

        for (QGraphicsItem* item : items()) {
            auto* castedItem = dynamic_cast<QGraphicsPixmapItem*>(item);
            if (!castedItem || castedItem->zValue() < 5) continue; // Csak bábuk

            int itemFile = castedItem->data(FileKey).toInt();
            int itemRank = castedItem->data(RankKey).toInt();

            if (itemFile == clickedFile && itemRank == clickedLogicalRank) {
                foundPiece = castedItem;
                break;
            }
        }

        activeItem = foundPiece;
        if (activeItem) {
            activeItemOriginalPos = activeItem->pos();
            activeItem->setZValue(100);
            activeItem->setCursor(Qt::ClosedHandCursor);
            activeItem->setPos(event->scenePos() - activeItem->boundingRect().center());
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void ChessScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (activeItem) {
        activeItem->setPos(event->scenePos() - activeItem->boundingRect().center());
    } else {
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void ChessScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (activeItem) {
        activeItem->setCursor(Qt::OpenHandCursor);
        activeItem->setZValue(10);

        int fromFile = activeItem->data(FileKey).toInt();
        int fromRank = activeItem->data(RankKey).toInt();

        int toFile, visualRank;
        bool insideBoard = scenePosToSquare(event->scenePos(), toFile, visualRank);

        if (insideBoard && cvm) {
            int toLogicalRank = 7 - visualRank;
            bool isMovePromotion = cvm->isMovePromotion(fromFile, fromRank, toFile, toLogicalRank);
            PieceType promotionPiece = isMovePromotion ? PieceType::QUEEN : PieceType::PIECE_NONE;
            cvm->movePiece(fromFile, fromRank, toFile, toLogicalRank, promotionPiece);
        }

        if (items().contains(activeItem)) {
            activeItem->setPos(activeItemOriginalPos);
        }
        activeItem = nullptr;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}
