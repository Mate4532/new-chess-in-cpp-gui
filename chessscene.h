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
        RankKey
    };

    explicit ChessScene(QObject* parent = nullptr);

    static constexpr double CHESSBOARD_OFFSET_LEFT_PX = 47;
    static constexpr double CHESSBOARD_OFFSET_DOWN_PX = 47;
    static constexpr double CHESSBOARD_OFFSET_RIGHT_PX = 13;
    static constexpr double CHESSBOARD_OFFSET_UP_PX = 13;
    static constexpr double TILE_SIZE = std::min(ChessView::WHOLE_CHESSBOARD_WIDTH_PX - CHESSBOARD_OFFSET_LEFT_PX - CHESSBOARD_OFFSET_RIGHT_PX,
                                                 ChessView::WHOLE_CHESSBOARD_HEIGHT_PX - CHESSBOARD_OFFSET_UP_PX - CHESSBOARD_OFFSET_DOWN_PX) / 8;
    static constexpr double PIECE_SIZE_SQUARE_RATIO = 0.9;
    static constexpr double PIECE_SIZE = TILE_SIZE * 0.9;

    void setViewModel(ChessViewModel* cvm);
    void preloadPixmaps();

    bool scenePosToSquare(const QPointF& pos, int& file, int& visualRank) const;
    QPointF squareToScenePos(int file, int visualRank) const;

    void updateLayout();

public slots:
    void onBoardChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    ChessViewModel* cvm = nullptr;

    QGraphicsPixmapItem* activeItem = nullptr;
    QPointF activeItemOriginalPos;

    std::unordered_map<QString, QPixmap> originalPixmaps;
    static const std::unordered_map<PieceType, QString> whitePieceMap;
    static const std::unordered_map<PieceType, QString> blackPieceMap;

    void sceneRectChanged(const QRectF &rect);
    void drawPieces(bool isFlipped);

private slots:
    void onSceneRectChanged(const QRectF& rect);
};

#endif
