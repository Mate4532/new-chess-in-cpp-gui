#include "chessscene.h"
#include "chesspieceitem.h"
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
    currentTileSize = 100;
    currentPieceSize = 90;
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
    file = int((pos.x() - currentLeftMarginPx) / currentTileSize);
    visualRank = int((pos.y() - currentUpMarginPx) / currentTileSize);

    if (file < 0 || file > 7 || visualRank < 0 || visualRank > 7)
        return false;

    return true;
}

QPointF ChessScene::squareToScenePos(int file, int visualRank) const
{
    double x = currentLeftMarginPx + file * currentTileSize + (currentTileSize - currentPieceSize) / 2;
    double y = currentUpMarginPx + visualRank * currentTileSize + (currentTileSize - currentPieceSize) / 2;
    return QPointF(x, y);
}

void ChessScene::updateLayout()
{
    if (!cvm) return;

    clear();
    activeItem = nullptr;

    auto boardMatrix = cvm->getBoardMatrix();

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {

            PieceType type = boardMatrix[row][col].first;
            Color color = boardMatrix[row][col].second;

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
                QPixmap scaled = originalPixmaps[resource].scaled(
                    currentPieceSize, currentPieceSize,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation
                    );
                item->setPixmap(scaled);
            }

            item->setPos(squareToScenePos(col, row));

            int logicalRank = 7 - row;

            item->setData(FileKey, col);
            item->setData(RankKey, logicalRank);

            item->setZValue(10);

            addItem(item);
        }
    }
}

void ChessScene::onBoardChanged() {
    updateLayout();
}

void ChessScene::onSceneRectChanged(const QRectF& rect)
{
    Q_UNUSED(rect);
    currentWholeBoardWidth = sceneRect().width();
    currentWholeBoardHeight = sceneRect().height();

    currentLeftMarginPx = leftMarginRatio * currentWholeBoardWidth;
    currentRightMarginPx = rightMarginRatio * currentWholeBoardWidth;
    currentUpMarginPx = upMarginRatio * currentWholeBoardHeight;
    currentDownMarginPx = downMarginRatio * currentWholeBoardHeight;

    currentRealBoardSize = std::min(currentWholeBoardWidth, currentWholeBoardHeight) - currentLeftMarginPx - currentRightMarginPx;
    currentTileSize = currentRealBoardSize / 8.0;
    currentPieceSize = currentTileSize * PIECE_SIZE_SQUARE_RATIO;

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

            cvm->movePiece(fromFile, fromRank, toFile, toLogicalRank);
        }

        if (items().contains(activeItem)) {
            activeItem->setPos(activeItemOriginalPos);
        }

        activeItem = nullptr;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}
