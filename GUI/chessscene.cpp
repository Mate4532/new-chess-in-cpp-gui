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

QRectF ChessScene::getSquareRect(int visualCol, int visualRow) const {

    double x1 = (static_cast<double>(visualCol) * FULL_BOARD_SIZE) / 8.0;
    double x2 = (static_cast<double>(visualCol + 1) * FULL_BOARD_SIZE) / 8.0;
    double y1 = (static_cast<double>(visualRow) * FULL_BOARD_SIZE) / 8.0;
    double y2 = (static_cast<double>(visualRow + 1) * FULL_BOARD_SIZE) / 8.0;

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

void ChessScene::highlightPromotionSquares() {
    std::vector<std::pair<int, int>> promotionSquares = getPromotionSquares(promotionSquareTo.first, promotionSquareTo.second);
    QColor highlightColor(255, 255, 255);

    QRectF boundingBox;

    for (size_t i = 0; i < promotionSquares.size(); ++i) {
        auto sq = promotionSquares[i];

        highlightSquare(sq.first, sq.second, highlightColor);

        bool isFlipped = cvm->getIsBoardFlipped();
        int visualCol = isFlipped ? (7 - sq.first) : sq.first;
        int visualRow = isFlipped ? sq.second : (7 - sq.second);
        QRectF rect = getSquareRect(visualCol, visualRow);

        if (i == 0) boundingBox = rect;
        else boundingBox = boundingBox.united(rect);
    }

    QGraphicsRectItem* containerBorder = new QGraphicsRectItem(boundingBox);

    QColor black(0, 0, 0);
    QPen borderPen(black);
    borderPen.setWidth(4);
    borderPen.setJoinStyle(Qt::MiterJoin);

    containerBorder->setPen(borderPen);
    containerBorder->setBrush(Qt::NoBrush);
    containerBorder->setZValue(105);

    addItem(containerBorder);
}

void ChessScene::highlightSquare(int logicalFile, int logicalRank, QColor highlightColor) {

    bool isFlipped = cvm->getIsBoardFlipped();

    int visualCol = isFlipped ? (7 - logicalFile) : logicalFile;
    int visualRow = isFlipped ? logicalRank : (7 - logicalRank);

    QRectF rectArea = getSquareRect(visualCol, visualRow);

    QGraphicsRectItem* rect = new QGraphicsRectItem(rectArea.adjusted(-0.5, -0.5, 0.5, 0.5));
    rect->setBrush(QBrush(highlightColor));
    rect->setPen(Qt::NoPen);
    rect->setZValue(1);

    addItem(rect);
}

std::vector<std::pair<int, int>> ChessScene::getPromotionSquares(int promotionFile, int promotionRank) {
    std::vector<std::pair<int, int>> promotionSquares;

    for (int i = 0; i < 4; ++i) {
        int yDirection = promotionRank == 7 ?  -i : i;
        std::pair<int, int> promotionSquare(promotionFile, promotionRank + yDirection);
        promotionSquares.push_back(promotionSquare);
    }

    return promotionSquares;
}

void ChessScene::addPieceToBoard(PieceType type, Color color, int logicalFile, int logicalRank, bool isPromotion) {
    QString resource;
    const auto& map = (color == Color::WHITE) ? whitePieceMap : blackPieceMap;
    if (map.count(type)) resource = map.at(type);

    if (resource.isEmpty()) return;

    ChessPieceItem* item = new ChessPieceItem();

    if (originalPixmaps.count(resource)) {
        const qreal qualityMultiplier = 1.5;
        int highResSize = static_cast<int>(PIECE_SIZE * qualityMultiplier);
        QPixmap scaled = originalPixmaps[resource].scaled(
            highResSize, highResSize,
            Qt::KeepAspectRatio, Qt::SmoothTransformation
            );
        scaled.setDevicePixelRatio(qualityMultiplier);
        item->setPixmap(scaled);
    }

    bool isFlipped = cvm->getIsBoardFlipped();
    int visualCol = isFlipped ? (7 - logicalFile) : logicalFile;
    int visualRow = isFlipped ? logicalRank : (7 - logicalRank);

    QRectF square = getSquareRect(visualCol, visualRow);
    item->setPos(square.topLeft());

    if (isPromotion) {
        item->setData(IsPromotionKey, true);
        item->setData(PieceTypeKey, static_cast<int>(type));
        item->setZValue(110);
        item->setCursor(Qt::PointingHandCursor);
    } else {
        item->setData(FileKey, logicalFile);
        item->setData(RankKey, logicalRank);
        item->setZValue(10);
    }

    addItem(item);
}


void ChessScene::drawPromotionPieces() {
    if (!cvm->isBoardUnderPromoption() || !cvm) return;

    Color promoColor = (promotionSquareTo.second == 7) ? Color::WHITE : Color::BLACK;
    std::vector<PieceType> promoTypes = { PieceType::QUEEN, PieceType::ROOK, PieceType::BISHOP, PieceType::KNIGHT };
    std::vector<std::pair<int, int>> targetSquares = getPromotionSquares(promotionSquareTo.first, promotionSquareTo.second);

    for (size_t i = 0; i < promoTypes.size(); ++i) {
        addPieceToBoard(promoTypes[i], promoColor, targetSquares[i].first, targetSquares[i].second, true);
    }
}

void ChessScene::drawMovedPieceBackground() {
    if (!cvm) return;

    MoveInfo lastMove = cvm->getMoveInfo();
    if (!lastMove.isValid()) return;

    QColor highlightColor(246, 246, 105, 150);

    highlightSquare(lastMove.fromFile, lastMove.fromRank, highlightColor);
    highlightSquare(lastMove.toFile, lastMove.toRank, highlightColor);
}

void ChessScene::drawPieces() {
    auto boardMatrix = cvm->getBoardMatrix();
    bool isPromotion = cvm->isBoardUnderPromoption();
    std::vector<std::pair<int, int>> promoSquares;

    if (isPromotion) {
        promoSquares = getPromotionSquares(promotionSquareTo.first, promotionSquareTo.second);
    }

    for (int matrixRow = 0; matrixRow < 8; ++matrixRow) {
        for (int matrixCol = 0; matrixCol < 8; ++matrixCol) {

            int logicalRank = 7 - matrixRow;
            int logicalFile = matrixCol;

            if (isPromotion) {
                if (logicalFile == promotionSquareFrom.first && logicalRank == promotionSquareFrom.second) continue;

                bool isReserved = false;
                for (const auto& sq : promoSquares) {
                    if (sq.first == logicalFile && sq.second == logicalRank) { isReserved = true; break; }
                }
                if (isReserved) continue;
            }

            PieceType type = boardMatrix[matrixRow][matrixCol].first;
            Color color = boardMatrix[matrixRow][matrixCol].second;

            if (type != PieceType::PIECE_NONE) {
                addPieceToBoard(type, color, logicalFile, logicalRank, false);
            }
        }
    }
}

void ChessScene::updateLayout()
{
    if (!cvm) return;
    clear();
    activeItem = nullptr;
    hoverHighlightItem = nullptr;
    drawMovedPieceBackground();
    drawPieces();
    if (cvm->isBoardUnderPromoption()) {
        highlightPromotionSquares();
        drawPromotionPieces();
    }
}

void ChessScene::onBoardChanged() {
    updateLayout();
}

void ChessScene::onPromotionEnded() {
    cvm->setIsBoardUnderPromotion(false);
    updateLayout();
}

void ChessScene::handlePromotion(int fromX, int fromY, int toX, int toY) {

    cvm->setIsBoardUnderPromotion(true);
    promotionSquareFrom = std::pair<int, int>(fromX, fromY);
    promotionSquareTo = std::pair<int, int>(toX, toY);
    updateLayout();
}

void ChessScene::onSceneRectChanged(const QRectF& rect)
{
    Q_UNUSED(rect);
    updateLayout();
}

void ChessScene::updateHoverHighlight(const QPointF& scenePos) {
    int file, visualRank;
    if (scenePosToSquare(scenePos, file, visualRank)) {
        bool isFlipped = cvm->getIsBoardFlipped();
        int visualCol = isFlipped ? (7 - file) : file;

        QRectF squareRect = getSquareRect(visualCol, visualRank);

        const qreal penWidth = 6.0;

        if (!hoverHighlightItem) {
            hoverHighlightItem = new QGraphicsRectItem();
            QPen boldPen(Qt::white);
            boldPen.setWidthF(penWidth);
            boldPen.setJoinStyle(Qt::MiterJoin);
            boldPen.setCapStyle(Qt::FlatCap);
            hoverHighlightItem->setPen(boldPen);
            hoverHighlightItem->setBrush(Qt::NoBrush);
            hoverHighlightItem->setZValue(50);
            addItem(hoverHighlightItem);
        }

        qreal offset = penWidth / 2.0;
        hoverHighlightItem->setRect(squareRect.adjusted(offset, offset, -offset, -offset));

        hoverHighlightItem->setVisible(true);
    } else {
        if (hoverHighlightItem) hoverHighlightItem->setVisible(false);
    }
}

void ChessScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    bool isPromotion = cvm->isBoardUnderPromoption();

    if (event->button() == Qt::RightButton) {
        if (isPromotion) {
            onPromotionEnded();
            event->accept();
            return;
        }

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

        QGraphicsItem* itemUnderMouse = itemAt(event->scenePos(), QTransform());
        if (itemUnderMouse) {
            QVariant isPromoData = itemUnderMouse->data(IsPromotionKey);
            if (isPromoData.isValid() && isPromoData.toBool()) {
                PieceType chosenType = static_cast<PieceType>(itemUnderMouse->data(PieceTypeKey).toInt());

                cvm->movePiece(promotionSquareFrom.first, promotionSquareFrom.second,
                                  promotionSquareTo.first, promotionSquareTo.second,
                                  chosenType);

                onPromotionEnded();
                event->accept();
                return;
            }
        }

        if (isPromotion) {
            onPromotionEnded();
            return;
        }

        int clickedFile, clickedVisualRank;
        if (!scenePosToSquare(event->scenePos(), clickedFile, clickedVisualRank)) {
            QGraphicsScene::mousePressEvent(event);
            return;
        }

        int clickedLogicalRank = 7 - clickedVisualRank;
        QGraphicsPixmapItem* foundPiece = nullptr;

        for (QGraphicsItem* item : items()) {
            auto* castedItem = dynamic_cast<QGraphicsPixmapItem*>(item);
            if (!castedItem || castedItem->zValue() < 5) continue;

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

            updateHoverHighlight(event->scenePos());
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void ChessScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (activeItem) {
        activeItem->setPos(event->scenePos() - activeItem->boundingRect().center());

        updateHoverHighlight(event->scenePos());
    } else {
        if (hoverHighlightItem) hoverHighlightItem->setVisible(false);
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void ChessScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{

    if (hoverHighlightItem) {
        hoverHighlightItem->setVisible(false);
    }

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
            if (!isMovePromotion) cvm->movePiece(fromFile, fromRank, toFile, toLogicalRank);
            else handlePromotion(fromFile, fromRank, toFile, toLogicalRank);
        }

        if (items().contains(activeItem)) {
            activeItem->setPos(activeItemOriginalPos);
        }
        activeItem = nullptr;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void ChessScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    mousePressEvent(event);
}
