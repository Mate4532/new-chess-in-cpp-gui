#include "chessview.h"
#include "chessscene.h"

#include <QPainter>

ChessView::ChessView(QWidget* parent) : QGraphicsView(parent),
    background(":/resources/resources/chessboard.png")
{
    setMinimumHeight(ChessView::WHOLE_CHESSBOARD_PX / 2);
    setMinimumWidth(ChessView::WHOLE_CHESSBOARD_PX / 2);
}

void ChessView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if (scene()) {
        QRectF board = boardRect();

        scene()->setSceneRect(0, 0, board.width(), board.height());

        if (auto* cs = dynamic_cast<ChessScene*>(scene())) {
            cs->updateLayout();
        }
    }
}

QRectF ChessView::boardRect() const
{
    QSizeF viewSize = viewport()->size();

    double side = qMin(viewSize.width(), viewSize.height());
    double x = (viewSize.width() - side) / 2.0;
    double y = (viewSize.height() - side) / 2.0;

    return QRectF(x, y, side, side);
}


void ChessView::drawBackground(QPainter* painter, const QRectF&)
{
    QRectF scene = sceneRect();

    QPixmap scaled = background.scaled(
        scene.size().toSize(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    painter->drawPixmap(0, 0, scaled);
}
