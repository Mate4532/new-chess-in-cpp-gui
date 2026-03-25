#include "chesspieceitem.h"
#include "chessscene.h"
#include <QPainter>

ChessPieceItem::ChessPieceItem()
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::OpenHandCursor);
    setTransformationMode(Qt::SmoothTransformation);
}

QRectF ChessPieceItem::boundingRect() const {
    return QRectF(0, 0, ChessScene::TILE_SIZE, ChessScene::TILE_SIZE);
}

QPainterPath ChessPieceItem::shape() const {
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void ChessPieceItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPixmap pix = pixmap();
    if (pix.isNull()) return;

    qreal dx = (ChessScene::TILE_SIZE - pix.width() / pix.devicePixelRatio()) / 2.0;
    qreal dy = (ChessScene::TILE_SIZE - pix.height() / pix.devicePixelRatio()) / 2.0;

    painter->drawPixmap(QPointF(dx, dy), pix);
}
