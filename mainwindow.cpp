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
    resize(ChessView::WHOLE_CHESSBOARD_WIDTH_PX / 2 + InfoView::MIN_WIDTH, ChessView::WHOLE_CHESSBOARD_HEIGHT_PX / 2);

    QWidget* central = new QWidget(this);
    central->setObjectName("CentralWidget");
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    board = new BoardManager();
    chessViewModel = new ChessViewModel(*board);

    RobotSettings rs;

    chessView = new ChessView(central);
    chessScene = new ChessScene(chessView);

    chessView->setScene(chessScene);
    chessScene->setViewModel(chessViewModel);

    infoContainer = new InfoView(allS);

    connect(infoContainer, &InfoView::settingsChanged, chessViewModel, &ChessViewModel::updateSettings);
    connect(infoContainer, &InfoView::newGameRequested, chessViewModel, &ChessViewModel::startGame);
    connect(infoContainer, &InfoView::undoRequested, chessViewModel, &ChessViewModel::undoLastMove);
    connect(infoContainer, &InfoView::giveUpRequested, chessViewModel, &ChessViewModel::currentPlayerGaveUp);

    chessViewModel->loadSettings(allS);

    mainLayout->addWidget(chessView, 5);
    mainLayout->addWidget(infoContainer, 1);

    central->setStyleSheet(R"(
        #CentralWidget {
            background-color: rgba(0, 0, 0, 180);
        }
    )");
}

MainWindow::~MainWindow() = default;
