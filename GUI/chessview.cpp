#include "chessview.h"
#include "chessscene.h"

#include <QPainter>

ChessView::ChessView(QWidget* parent) : QGraphicsView(parent),
    background(":/resources/resources/chessboard.png")
{
    //setMinimumHeight(WHOLE_CHESSBOARD_WIDTH_PX / 2);
    //setMinimumWidth(WHOLE_CHESSBOARD_HEIGHT_PX / 2);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::Antialiasing);

    this->setFrameShape(QFrame::NoFrame);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChessView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if (scene()) {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
        double currentZoomX = this->transform().m11();
        int physicalOffset = qRound(ChessScene::CHESSBOARD_OFFSET_LEFT_PX * currentZoomX);
        emit visualOffsetChanged(physicalOffset);
    }

}

QRectF ChessView::boardRect() const
{
    QSizeF viewSize = viewport()->size();

    double side = qMin(viewSize.width(), viewSize.height());
    double x = (viewSize.width() - side) / 2.0;
    double y = (viewSize.height() - side) / 2.0;

    return QRectF(x, y , side, side);
}

void ChessView::drawBackground(QPainter* painter, const QRectF& rect)
{
    QRectF scene = sceneRect();

    painter->fillRect(rect, QColor(49, 46, 43));

    QPixmap scaled = background.scaled(
        scene.size().toSize(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    painter->drawPixmap(0, 0, scaled);
}

void ChessView::flipBoardTo(bool isFlipped) {
    if (isFlipped) {
        setBackgroundImage(":/resources/resources/chessboard_flipped.png");
    } else {
        setBackgroundImage(":/resources/resources/chessboard.png");
    }
}

void ChessView::setBackgroundImage(const QString& imagePath)
{
    background = QPixmap(imagePath);

    viewport()->update();
}
