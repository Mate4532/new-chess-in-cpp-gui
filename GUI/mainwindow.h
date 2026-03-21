#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chessview.h"
#include "chessscene.h"
#include "infoview.h"
#include "boardAndPlayerPanel.h"

#include <QMainWindow>

class QGraphicsView;
class QGraphicsScene;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    BoardManager* board;
    ChessView* chessView;
    ChessViewModel *chessViewModel;
    ChessScene* chessScene;
    InfoView* infoContainer;
    BoardAndPlayerPanel* bapp ;

    AllSettings allS;

    void calculateDynamicMinimumSize();

    const int RATIO_BOARD = 10;
    const int RATIO_INFO = 3;
    const int TOTAL_RATIO = RATIO_BOARD + RATIO_INFO;

    const int mainLayoutSpacing = 15;
    const int mainLayoutMarginLeft = 15;
    const int mainLayoutMarginRight = 15;
    const int mainLayoutMarginTop = 15;
    const int mainLayoutMarginBottom = 15;

    const int centralBlockSpacing = 10;
};

#endif
