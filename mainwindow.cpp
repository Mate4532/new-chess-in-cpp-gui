#include "mainwindow.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QThread>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Chess GUI");
    resize(ChessView::WHOLE_CHESSBOARD_PX / 2 + InfoView::MIN_WIDTH, ChessView::WHOLE_CHESSBOARD_PX / 2);

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    board = new BoardManager(false, true);
    chessViewModel = new ChessViewModel(*board);

    chessView = new ChessView(central);
    chessScene = new ChessScene(chessView);

    chessView->setScene(chessScene);
    chessScene->setViewModel(chessViewModel);

    infoContainer = new InfoView();

    connect(infoContainer, &InfoView::newGameRequested, chessViewModel, &ChessViewModel::loadNewGame);
    connect(infoContainer, &InfoView::undoRequested, chessViewModel, &ChessViewModel::undoLastMove);
    connect(infoContainer, &InfoView::giveUp, chessViewModel, &ChessViewModel::currentPlayerGaveUp);

    mainLayout->addWidget(chessView, 5);
    mainLayout->addWidget(infoContainer, 1);

    central->setStyleSheet(R"(
        QWidget {
            background-color: rgba(0, 0, 0, 120);
        }
    )");
}

MainWindow::~MainWindow() = default;
