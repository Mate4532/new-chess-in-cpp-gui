#ifndef CHESSPIECEITEM_H
#define CHESSPIECEITEM_H

#include <QGraphicsPixmapItem>

class ChessPieceItem : public QGraphicsPixmapItem
{
public:
    ChessPieceItem();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // CHESSPIECEITEM_H
