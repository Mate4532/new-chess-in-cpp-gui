#include "chesspieceitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QCursor>

ChessPieceItem::ChessPieceItem()
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::OpenHandCursor);
    setTransformationMode(Qt::SmoothTransformation);
}
