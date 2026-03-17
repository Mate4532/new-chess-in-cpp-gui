#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chessview.h"
#include "chessscene.h"
#include "infoview.h"

#include <QMainWindow>

class QGraphicsView;
class QGraphicsScene;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    BoardManager* board;
    ChessView* chessView;
    ChessViewModel *chessViewModel;
    ChessScene* chessScene;
    InfoView* infoContainer;

    AllSettings allS;
};

#endif
