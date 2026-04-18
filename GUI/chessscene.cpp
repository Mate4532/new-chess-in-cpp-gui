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

QRectF ChessScene::getSquareRect(int col, int row, bool fromBoardCoordinates) const {

    bool isFlipped = cvm->getIsBoardFlipped();
    int visualCol, visualRow;

    if (fromBoardCoordinates) {
        visualCol = isFlipped ? (7 - col) : col;
        visualRow = isFlipped ? row : (7 - row);
    }
    else {
        visualCol = isFlipped ? (7 - col) : col;
        visualRow = isFlipped ? (7 - row) : row;
    }

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
        QRectF rect = getSquareRect(sq.first, sq.second, true);

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

    QRectF rectArea = getSquareRect(logicalFile, logicalRank, true);

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

    QRectF square = getSquareRect(logicalFile, logicalRank, true);
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

    highlightSquare(lastMove.fromFile, lastMove.fromRank, baseHighlightColor);
    highlightSquare(lastMove.toFile, lastMove.toRank, baseHighlightColor);
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

void ChessScene::highlightSelectedPiece() {
    if (activeItem) {
        int file = activeItem->data(FileKey).toInt();
        int rank = activeItem->data(RankKey).toInt();

        highlightSquare(file, rank, baseHighlightColor);
    }
}

void ChessScene::drawLegalMoveDots() {
    if (!cvm || currentLegalMoves.empty() || cvm->isUnderReview() || !cvm->showLegalMoves() || cvm->isBotVsBotMode()) return;

    auto boardMatrix = cvm->getBoardMatrix();

    QColor dotColor(0, 0, 0, 50);
    double normalRadius = TILE_SIZE * 0.15;
    double captureRadius = TILE_SIZE * 0.42;

    for (const auto& move : currentLegalMoves) {
        int file = move.first;
        int rank = move.second;

        QRectF rect = getSquareRect(file, rank, true);
        QPointF center = rect.center();

        bool isOccupied = false;
        if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            isOccupied = (boardMatrix[7 - rank][file].first != PieceType::PIECE_NONE);
        }

        if (isOccupied) {
            QGraphicsEllipseItem* ring = new QGraphicsEllipseItem(
                center.x() - captureRadius,
                center.y() - captureRadius,
                captureRadius * 2,
                captureRadius * 2
                );

            QPen ringPen(dotColor);
            ringPen.setWidth(TILE_SIZE * 0.08);
            ring->setPen(ringPen);
            ring->setBrush(Qt::NoBrush);
            ring->setZValue(5);
            addItem(ring);
        }
        else {
            QGraphicsEllipseItem* dot = new QGraphicsEllipseItem(
                center.x() - normalRadius,
                center.y() - normalRadius,
                normalRadius * 2,
                normalRadius * 2
                );

            dot->setBrush(QBrush(dotColor));
            dot->setPen(Qt::NoPen);
            dot->setZValue(5);
            addItem(dot);
        }
    }
}

void ChessScene::drawCheckHighlight() {
    if (!cvm) return;

    std::pair<int, int> kingPos = cvm->getKingInCheckCoords();

    if (kingPos.first == -1 || kingPos.second == -1) return;

    if (activeItem) {
        int activeFile = activeItem->data(FileKey).toInt();
        int activeRank = activeItem->data(RankKey).toInt();

        if (activeFile == kingPos.first && activeRank == kingPos.second) {
            return;
        }
    }

    QColor checkRed(230, 80, 80, 200);
    highlightSquare(kingPos.first, kingPos.second, checkRed);
}

void ChessScene::restoreDraggingState(const DraggingState& state)
{
    QGraphicsPixmapItem* newActiveItem = findPieceAt(state.file, state.rank);

    if (newActiveItem) {
        activeItem = newActiveItem;
        activeItemOriginalPos = getSquareRect(state.file, state.rank, true).topLeft();
        activeItem->setZValue(100);
        activeItem->setPos(state.lastScenePos - activeItem->boundingRect().center());
    }
}

QGraphicsPixmapItem* ChessScene::findPieceAt(int file, int rank)
{
    for (QGraphicsItem* item : items()) {
        auto* pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(item);
        if (pixmapItem && pixmapItem->zValue() >= 5) {
            if (pixmapItem->data(FileKey).toInt() == file &&
                pixmapItem->data(RankKey).toInt() == rank) {
                return pixmapItem;
            }
        }
    }
    return nullptr;
}

ChessScene::DraggingState ChessScene::captureDraggingState()
{
    DraggingState state;
    state.wasItemActive = activeItem != nullptr;

    if (state.wasItemActive) {
        state.file = activeItem->data(FileKey).toInt();
        state.rank = activeItem->data(RankKey).toInt();
        state.lastScenePos = activeItem->pos() + activeItem->boundingRect().center();
    }
    return state;
}

void ChessScene::renderBoard()
{
    clear();
    activeItem = nullptr;
    hoverHighlightItem = nullptr;

    drawMovedPieceBackground();
    drawCheckHighlight();
    drawLegalMoveDots();
    drawPieces();

    if (cvm->isBoardUnderPromoption()) {
        highlightPromotionSquares();
        drawPromotionPieces();
    }
}

void ChessScene::refreshHoverEffect()
{
    if (views().isEmpty() || activeItem == nullptr) return;

    QGraphicsView* view = views().first();
    QPointF currentScenePos = view->mapToScene(view->mapFromGlobal(QCursor::pos()));
    updateHoverHighlight(currentScenePos);
}

void ChessScene::updateLayout()
{
    if (!cvm) return;

    DraggingState state = captureDraggingState();

    if (state.wasItemActive) {
        currentLegalMoves = cvm->getLegalMovesForPiece(state.file, state.rank);
    }

    renderBoard();

    if (state.wasItemActive) {
        restoreDraggingState(state);
    }

    refreshHoverEffect();
    highlightSelectedPiece();
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
    int file, rank;
    if (scenePosToSquare(scenePos, file, rank)) {

        QRectF squareRect = getSquareRect(file, rank, false);

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
    if (event->button() == Qt::RightButton) {
        if (cvm->isBoardUnderPromoption()) {
            onPromotionEnded();
        } else if (activeItem) {
            activeItem->setPos(activeItemOriginalPos);
            activeItem->setZValue(10);
            activeItem = nullptr;
            currentLegalMoves.clear(); // Takarítás jobb klikknél is
            updateLayout();
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        QGraphicsItem* itemUnderMouse = itemAt(event->scenePos(), QTransform());

        if (itemUnderMouse && itemUnderMouse->data(IsPromotionKey).toBool()) {
            PieceType chosenType = static_cast<PieceType>(itemUnderMouse->data(PieceTypeKey).toInt());
            cvm->movePiece(promotionSquareFrom.first, promotionSquareFrom.second,
                           promotionSquareTo.first, promotionSquareTo.second, chosenType);
            onPromotionEnded();
            event->accept();
            return;
        }

        if (cvm->isBoardUnderPromoption()) {
            onPromotionEnded();
            return;
        }

        int clickedFile, clickedVisualRank;
        if (!scenePosToSquare(event->scenePos(), clickedFile, clickedVisualRank)) return;
        int clickedLogicalRank = 7 - clickedVisualRank;

        if (activeItem) {
            int fromFile = activeItem->data(FileKey).toInt();
            int fromRank = activeItem->data(RankKey).toInt();

            if (fromFile == clickedFile && fromRank == clickedLogicalRank) {
                isReclickingActiveItem = true;
                wasPieceDragged = false;

                activeItem->setPos(event->scenePos() - activeItem->boundingRect().center());
                activeItem->setZValue(100);
                event->accept();
                return;
            }

            if (fromFile != clickedFile || fromRank != clickedLogicalRank) {
                cvm->movePiece(fromFile, fromRank, clickedFile, clickedLogicalRank);
                if (!activeItem) {
                    currentLegalMoves.clear();
                    return;
                }
            }
        }

        isReclickingActiveItem = false;
        QGraphicsPixmapItem* foundPiece = nullptr;
        for (QGraphicsItem* item : items()) {
            auto* castedItem = dynamic_cast<QGraphicsPixmapItem*>(item);
            if (!castedItem || castedItem->zValue() < 5 || castedItem->data(IsPromotionKey).toBool()) continue;

            if (castedItem->data(FileKey).toInt() == clickedFile &&
                castedItem->data(RankKey).toInt() == clickedLogicalRank) {
                foundPiece = castedItem;
                break;
            }
        }

        if (foundPiece) {
            if (activeItem && activeItem != foundPiece) {
                activeItem->setPos(activeItemOriginalPos);
                activeItem->setZValue(10);
            }

            activeItem = foundPiece;
            activeItemOriginalPos = getSquareRect(clickedFile, clickedLogicalRank, true).topLeft();
            activeItem->setZValue(100);
            activeItem->setPos(event->scenePos() - activeItem->boundingRect().center());

            currentLegalMoves = cvm->getLegalMovesForPiece(clickedFile, clickedLogicalRank);

            updateLayout();
        } else {
            if (activeItem) {
                activeItem->setPos(activeItemOriginalPos);
                activeItem->setZValue(10);
            }
            activeItem = nullptr;
            currentLegalMoves.clear();
            updateLayout();
        }
        event->accept();
    }
}

void ChessScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (activeItem) {
        if (event->buttons() & Qt::LeftButton) {
            activeItem->setPos(event->scenePos() - activeItem->boundingRect().center());
            wasPieceDragged = true;
        }

        updateHoverHighlight(event->scenePos());
    }
    else {
        if (hoverHighlightItem) {
            hoverHighlightItem->setVisible(false);
        }
        QGraphicsScene::mouseMoveEvent(event);
    }
}

void ChessScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (activeItem) {
        if (wasPieceDragged) {
            int fromFile = activeItem->data(FileKey).toInt();
            int fromRank = activeItem->data(RankKey).toInt();

            int toFile, visualRank;
            if (scenePosToSquare(event->scenePos(), toFile, visualRank)) {
                int toLogicalRank = 7 - visualRank;
                cvm->movePiece(fromFile, fromRank, toFile, toLogicalRank);
            }

            if (activeItem) {
                activeItem->setPos(activeItemOriginalPos);
                activeItem->setZValue(10);
                activeItem = nullptr;
                currentLegalMoves.clear();
                updateLayout();
            }
        }
        else if (isReclickingActiveItem) {
            activeItem->setPos(activeItemOriginalPos);
            activeItem->setZValue(10);
            activeItem = nullptr;
            currentLegalMoves.clear();
            updateLayout();
        }
        else {
            activeItem->setPos(activeItemOriginalPos);
        }
    }

    wasPieceDragged = false;
    isReclickingActiveItem = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

void ChessScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    mousePressEvent(event);
}
