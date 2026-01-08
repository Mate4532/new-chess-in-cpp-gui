#ifndef CHESSVIEW_H
#define CHESSVIEW_H

#include <QGraphicsView>
#include <QPixmap>

class ChessView : public QGraphicsView {
    Q_OBJECT
public:
    static constexpr double WHOLE_CHESSBOARD_WIDTH_PX = 1358;
    static constexpr double WHOLE_CHESSBOARD_HEIGHT_PX = 1358;
    explicit ChessView(QWidget* parent = nullptr);

    void resizeEvent(QResizeEvent* event) override;

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    QRectF boardRect() const;
    QPixmap background;
};


#endif // CHESSVIEW_H
