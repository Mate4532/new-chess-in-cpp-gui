#include "chessscene.h"
#include "chesspieceitem.h"
#include "chessview.h"

#include <QDebug>
#include <QCursor>

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
    connect(cvm, &ChessViewModel::boardChanged, this, &ChessScene::onBoardChanged);
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

bool ChessScene::scenePosToSquare(const QPointF& pos, int& file, int& visualRank) const
{
    double xInsideBoard = pos.x() - CHESSBOARD_OFFSET_LEFT_PX;
    double yInsideBoard = pos.y() - CHESSBOARD_OFFSET_UP_PX;

    if (xInsideBoard < 0 || yInsideBoard < 0) return false;

    file = cvm->getIsBoardFlipped() ? 7 - static_cast<int>(xInsideBoard / TILE_SIZE) : static_cast<int>(xInsideBoard / TILE_SIZE);
    visualRank = cvm->getIsBoardFlipped() ? 7 - static_cast<int>(yInsideBoard / TILE_SIZE) : static_cast<int>(yInsideBoard / TILE_SIZE);

    if (file < 0 || file > 7 || visualRank < 0 || visualRank > 7)
        return false;

    return true;
}

QPointF ChessScene::squareToScenePos(int file, int visualRank) const
{
    double x = CHESSBOARD_OFFSET_LEFT_PX + file * TILE_SIZE + (TILE_SIZE - PIECE_SIZE) / 2;
    double y = CHESSBOARD_OFFSET_UP_PX + visualRank * TILE_SIZE + (TILE_SIZE - PIECE_SIZE) / 2;
    return QPointF(x, y);
}

void ChessScene::drawPieces(bool isFlipped) {
    auto boardMatrix = cvm->getBoardMatrix();

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
                    highResSize,
                    highResSize,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                    );

                scaled.setDevicePixelRatio(qualityMultiplier);

                item->setPixmap(scaled);
            }

            item->setPos(squareToScenePos(visualCol, visualRow));

            int logicalRank = 7 - matrixRow;
            int logicalFile = matrixCol;

            item->setData(FileKey, logicalFile);
            item->setData(RankKey, logicalRank);
            item->setZValue(10);

            addItem(item);
        }
    }
}

void ChessScene::updateLayout()
{
    if (!cvm) return;

    clear();
    activeItem = nullptr;

    bool isFlipped = cvm->getIsBoardFlipped();

    drawPieces(isFlipped);
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
            if (!castedItem) continue;

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
