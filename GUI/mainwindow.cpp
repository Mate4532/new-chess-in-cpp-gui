#include "mainwindow.h"
#include "playerpanel.h"
#include "boardAndPlayerPanel.h"

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
    setWindowIcon(QIcon(":/resources/resources/white_knight.png"));
    resize(ChessView::WHOLE_CHESSBOARD_WIDTH_PX / 2 + InfoView::MIN_WIDTH, ChessView::WHOLE_CHESSBOARD_HEIGHT_PX / 2);

    QWidget* central = new QWidget(this);
    central->setObjectName("CentralWidget");
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    board = new BoardManager();
    chessViewModel = new ChessViewModel(*board);

    chessView = new ChessView(central);
    chessScene = new ChessScene(chessView);

    chessView->setScene(chessScene);
    chessScene->setViewModel(chessViewModel);

    infoContainer = new InfoView(allS);

    PlayerPanel* whitePlayer = new PlayerPanel("Fehér játékos", ":/resources/resources/white_pawn.png");
    PlayerPanel* blackPlayer = new PlayerPanel("Fekete játékos", ":/resources/resources/black_pawn.png");

    BoardAndPlayerPanel* bapp = new BoardAndPlayerPanel(whitePlayer, blackPlayer, chessView);

    connect(chessViewModel, &ChessViewModel::gameEnded, infoContainer, &InfoView::writeGameResultToDisplay);
    connect(chessViewModel, &ChessViewModel::moveMade, infoContainer, &InfoView::addMoveToDisplay);
    connect(chessViewModel, &ChessViewModel::clearInfoDisplay, infoContainer, &InfoView::clearMoveDisplay);
    connect(chessViewModel, &ChessViewModel::moveUndone, infoContainer, &InfoView::removeLastButFromDisplay);
    connect(chessViewModel, &ChessViewModel::flipBoardToRequest, chessView, &ChessView::flipBoardTo);
    connect(chessViewModel, &ChessViewModel::flipBoardToRequest, bapp, &BoardAndPlayerPanel::flipPlayerPanels);
    connect(chessViewModel, &ChessViewModel::playerPanelsUpdateRequest, bapp, &BoardAndPlayerPanel::playerPanelChanged);

    connect(infoContainer, &InfoView::reviewRequested, chessViewModel, &ChessViewModel::reviewHistory);
    connect(infoContainer, &InfoView::settingsChanged, chessViewModel, &ChessViewModel::updateSettings);
    connect(infoContainer, &InfoView::newGameRequested, chessViewModel, &ChessViewModel::startGame);
    connect(infoContainer, &InfoView::undoRequested, chessViewModel, &ChessViewModel::undoMove);
    connect(infoContainer, &InfoView::giveUpRequested, chessViewModel, &ChessViewModel::currentPlayerGaveUp);

    chessViewModel->loadSettings(allS);

    QHBoxLayout* centerBlock = new QHBoxLayout();

    centerBlock->addWidget(bapp, 10);
    centerBlock->addWidget(infoContainer, 3);

    mainLayout->addStretch();
    mainLayout->addLayout(centerBlock);
    mainLayout->addStretch();

    central->setStyleSheet(R"(
        #CentralWidget {
            background-color: rgb(29, 27, 25);
        }
    )");
}

MainWindow::~MainWindow() = default;
