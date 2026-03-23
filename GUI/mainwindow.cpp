#include "mainwindow.h"
#include "playerpanel.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QThread>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Chess GUI");
    setWindowIcon(QIcon(":/resources/resources/white_knight.png"));

    QWidget* central = new QWidget(this);
    central->setObjectName("CentralWidget");
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(mainLayoutMarginLeft, mainLayoutMarginTop, mainLayoutMarginRight, mainLayoutMarginBottom);
    mainLayout->setSpacing(mainLayoutSpacing);

    board = new BoardManager();
    chessViewModel = new ChessViewModel(*board);

    chessView = new ChessView(central);
    chessScene = new ChessScene(chessView);

    chessView->setScene(chessScene);
    chessScene->setViewModel(chessViewModel);

    infoContainer = new InfoView(allS);

    PlayerPanel* whitePlayer = new PlayerPanel(WHITE, "Fehér játékos", ":/resources/resources/white_pawn.png");
    PlayerPanel* blackPlayer = new PlayerPanel(BLACK, "Fekete játékos", ":/resources/resources/black_pawn.png");

    bapp = new BoardAndPlayerPanel(whitePlayer, blackPlayer, chessView);

    connect(chessViewModel, &ChessViewModel::gameEnded, infoContainer, &InfoView::writeGameResultToDisplay);
    connect(chessViewModel, &ChessViewModel::moveMade, infoContainer, &InfoView::addMoveToDisplay);
    connect(chessViewModel, &ChessViewModel::newGameStarted, infoContainer, &InfoView::clearMoveDisplay);
    connect(chessViewModel, &ChessViewModel::removeLastButFromInfoDisplayRequest, infoContainer, &InfoView::removeLastButFromDisplay);

    connect(chessViewModel, &ChessViewModel::moveBeenMade, bapp, &BoardAndPlayerPanel::moveMade);
    connect(chessViewModel, &ChessViewModel::flipBoardToRequest, chessView, &ChessView::flipBoardTo);
    connect(chessViewModel, &ChessViewModel::flipBoardToRequest, bapp, &BoardAndPlayerPanel::flipPlayerPanels);
    connect(chessViewModel, &ChessViewModel::playerPanelsUpdateRequest, bapp, &BoardAndPlayerPanel::playerPanelChanged);
    connect(chessViewModel, &ChessViewModel::addCapturedPieceToPlayerPanel, bapp, &BoardAndPlayerPanel::addPieceToPlayerPanel);
    connect(chessViewModel, &ChessViewModel::removePieceFromPlayerPanel, bapp, &BoardAndPlayerPanel::removePiecesFromPanel);
    connect(chessViewModel, &ChessViewModel::updateMaterialScoreAtPlayerPanel, bapp, &BoardAndPlayerPanel::updateMaterialScoreBasedOnPieces);
    connect(chessViewModel, &ChessViewModel::updatePlayerPanelsPieceAndScoreAtNewPosRequest, bapp, &BoardAndPlayerPanel::updateMaterialScoreAndCapturedPiecesAtNewPosLoaded);
    connect(chessViewModel, &ChessViewModel::reviewingAtPly, bapp, &BoardAndPlayerPanel::reviewHistory);
    connect(chessViewModel, &ChessViewModel::playerPanelUndoToLastPieceState, bapp, &BoardAndPlayerPanel::undoToLastPlayerPanelPiecesState);

    connect(infoContainer, &InfoView::reviewRequested, chessViewModel, &ChessViewModel::reviewHistory);
    connect(infoContainer, &InfoView::settingsChanged, chessViewModel, &ChessViewModel::updateSettings);
    connect(infoContainer, &InfoView::newGameRequested, chessViewModel, &ChessViewModel::startGame);
    connect(infoContainer, &InfoView::undoRequested, chessViewModel, &ChessViewModel::undoMove);
    connect(infoContainer, &InfoView::giveUpRequested, chessViewModel, &ChessViewModel::currentPlayerGaveUp);

    chessViewModel->loadSettings(allS);

    QHBoxLayout* centerBlock = new QHBoxLayout();
    centerBlock->setSpacing(centralBlockSpacing);

    centerBlock->addStretch();
    centerBlock->addWidget(bapp, RATIO_BOARD);
    centerBlock->addWidget(infoContainer, RATIO_INFO);
    centerBlock->addStretch();

    mainLayout->addLayout(centerBlock);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);


    central->setStyleSheet(R"(
        #CentralWidget {
            background-color: rgb(29, 27, 25);
        }
    )");

    resize(width(), height());
    calculateDynamicMinimumSize();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    if (!bapp || !infoContainer || !centralWidget() || !centralWidget()->layout()) return;

    QMargins mainMargins = centralWidget()->layout()->contentsMargins();
    int spacing = centralWidget()->layout()->spacing();

    int availW = event->size().width() - mainMargins.left() - mainMargins.right() - spacing;
    int availH = event->size().height() - mainMargins.top() - mainMargins.bottom();

    int targetBoardW = (availW * RATIO_BOARD) / TOTAL_RATIO;

    int finalBoardW = bapp->updateSize(targetBoardW, availH);

    int finalInfoW = (finalBoardW * RATIO_INFO) / RATIO_BOARD;

    infoContainer->setMaximumWidth(finalInfoW);
}

void MainWindow::calculateDynamicMinimumSize() {
    QSize bappMin = bapp->getMinimumOptimalSize();
    int infoMinW = InfoView::MIN_WIDTH;

    int boardWidthBasedOnInfo = (infoMinW * 10) / 3;

    int finalMinBoardW = qMax(bappMin.width(), boardWidthBasedOnInfo);
    int finalMinInfoW = (finalMinBoardW * 3) / 10;

    QMargins m = centralWidget()->layout()->contentsMargins();
    int spacing = centralWidget()->layout()->spacing();

    int totalMinW = finalMinBoardW + finalMinInfoW + m.left() + m.right() + spacing;
    int totalMinH = bappMin.height() + m.top() + m.bottom();

    this->setMinimumSize(totalMinW, totalMinH);
}

MainWindow::~MainWindow() = default;
