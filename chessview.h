#ifndef CHESSVIEW_H
#define CHESSVIEW_H

#include <QGraphicsView>
#include <QPixmap>

class ChessView : public QGraphicsView {
    Q_OBJECT
public:
    static constexpr double WHOLE_CHESSBOARD_PX = 1358;
    static constexpr double CHESSBOARD_OFFSET_LEFT_PX = 47;
    static constexpr double CHESSBOARD_OFFSET_DOWN_PX = 47;
    static constexpr double CHESSBOARD_OFFSET_RIGHT_PX = 13;
    static constexpr double CHESSBOARD_OFFSET_UP_PX = 13;
    explicit ChessView(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    QRectF boardRect() const;
    QPixmap background;
};


#endif // CHESSVIEW_H
