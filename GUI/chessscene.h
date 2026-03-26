#ifndef CHESSSCENE_H
#define CHESSSCENE_H

#include "chessviewmodel.h"
#include "chessview.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QString>
#include <unordered_map>

class ChessScene : public QGraphicsScene {
    Q_OBJECT
public:

    enum PieceData {
        FileKey = Qt::UserRole + 1,
        RankKey,
        PieceTypeKey,
        IsPromotionKey
    };

    explicit ChessScene(QObject* parent = nullptr);

    static constexpr double CHESSBOARD_OFFSET_LEFT_PX = 47;
    static constexpr double CHESSBOARD_OFFSET_DOWN_PX = 47;
    static constexpr double CHESSBOARD_OFFSET_RIGHT_PX = 15;
    static constexpr double CHESSBOARD_OFFSET_UP_PX = 15;
    static constexpr double TILE_SIZE = std::min(ChessView::WHOLE_CHESSBOARD_WIDTH_PX - CHESSBOARD_OFFSET_LEFT_PX - CHESSBOARD_OFFSET_RIGHT_PX,
                                                 ChessView::WHOLE_CHESSBOARD_HEIGHT_PX - CHESSBOARD_OFFSET_UP_PX - CHESSBOARD_OFFSET_DOWN_PX) / 8;
    static constexpr double PIECE_SIZE_SQUARE_RATIO = 0.9;
    static constexpr double PIECE_SIZE = TILE_SIZE * 0.9;
    static constexpr double FULL_BOARD_SIZE = TILE_SIZE * 8;

    void setViewModel(ChessViewModel* cvm);
    void preloadPixmaps();

    bool scenePosToSquare(const QPointF& pos, int& file, int& visualRank) const;
    QRectF getSquareRect(int col, int row, bool fromBoardCoordinates) const;

    void updateLayout();

public slots:
    void onBoardChanged();
    void onPromotionEnded();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    ChessViewModel* cvm = nullptr;

    QGraphicsPixmapItem* activeItem = nullptr;
    QGraphicsRectItem* hoverHighlightItem = nullptr;
    QPointF activeItemOriginalPos;

    std::unordered_map<QString, QPixmap> originalPixmaps;
    static const std::unordered_map<PieceType, QString> whitePieceMap;
    static const std::unordered_map<PieceType, QString> blackPieceMap;

    void sceneRectChanged(const QRectF &rect);
    void addPieceToBoard(PieceType type, Color color, int logicalFile, int logicalRank, bool isPromotion);
    void drawPromotionPieces();
    void drawMovedPieceBackground();
    void drawPieces();

    void updateHoverHighlight(const QPointF& scenePos);
    void handlePromotion(int fromX, int fromY, int toX, int toY);

    void highlightPromotionSquares();
    void highlightSquare(int logicalFile, int logicalRank, QColor highlightColor);
    std::vector<std::pair<int, int>> getPromotionSquares(int promotionFile, int promotionRank);

    std::pair<int, int> promotionSquareFrom;
    std::pair<int, int> promotionSquareTo;

private slots:
    void onSceneRectChanged(const QRectF& rect);
};

#endif
