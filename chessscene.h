#ifndef CHESSSCENE_H
#define CHESSSCENE_H

#include "chessview.h"
#include "chessviewmodel.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QString>
#include <unordered_map>

class ChessScene : public QGraphicsScene {
    Q_OBJECT
public:
    static constexpr double PIECE_SIZE_SQUARE_RATIO = 0.9;

    enum PieceData {
        FileKey = Qt::UserRole + 1,
        RankKey
    };

    explicit ChessScene(QObject* parent = nullptr);

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
    const double leftMarginRatio = static_cast<double>(ChessView::CHESSBOARD_OFFSET_LEFT_PX) / ChessView::WHOLE_CHESSBOARD_PX;
    const double rightMarginRatio = static_cast<double>(ChessView::CHESSBOARD_OFFSET_RIGHT_PX) / ChessView::WHOLE_CHESSBOARD_PX;
    const double upMarginRatio = static_cast<double>(ChessView::CHESSBOARD_OFFSET_UP_PX) / ChessView::WHOLE_CHESSBOARD_PX;
    const double downMarginRatio = static_cast<double>(ChessView::CHESSBOARD_OFFSET_DOWN_PX) / ChessView::WHOLE_CHESSBOARD_PX;

    double currentWholeBoardWidth;
    double currentWholeBoardHeight;
    double currentLeftMarginPx;
    double currentRightMarginPx;
    double currentUpMarginPx;
    double currentDownMarginPx;
    double currentRealBoardSize;
    double currentTileSize;
    double currentPieceSize;

private slots:
    void onSceneRectChanged(const QRectF& rect);
};

#endif
