#ifndef CHESSVIEW_H
#define CHESSVIEW_H

#include <QGraphicsView>
#include <QPixmap>

class ChessView : public QGraphicsView {
    Q_OBJECT
public:
    static constexpr double WHOLE_CHESSBOARD_WIDTH_PX = 1358;
    static constexpr double WHOLE_CHESSBOARD_HEIGHT_PX = 1358;
    static constexpr const char* BACKGROUND_PATH = ":/resources/resources/chessboard.png";
    static constexpr const char* BACKGROUND_PATH_FLIPPED = ":/resources/resources/chessboard_flipped.png";
    explicit ChessView(QWidget* parent = nullptr);

    void resizeEvent(QResizeEvent* event) override;

signals:
    void visualOffsetChanged(int physicalPixels);

public slots:
    void flipBoardTo(bool isFlipped);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    QRectF boardRect() const;
    QPixmap background;

    void setBackgroundImage(const QString& imagePath);
};


#endif // CHESSVIEW_H
