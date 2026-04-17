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

    allS.boardSettings.beginnerPosFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    infoContainer = new InfoView(allS);

    PlayerInfo whitePlayer(WHITE, "Fehér játékos", ":/resources/resources/white_pawn.png");
    PlayerInfo blackPlayer(BLACK, "Fekete játékos", ":/resources/resources/black_pawn.png");

    bool isPlayerPanelTimerVisible = allS.timeSettings.gm == GameMode::TOURNAMENT_MODE;

    PlayerPanel* whitePlayerPanel = new PlayerPanel(whitePlayer, isPlayerPanelTimerVisible);
    PlayerPanel* blackPlayerPanel = new PlayerPanel(blackPlayer, isPlayerPanelTimerVisible);

    bapp = new BoardAndPlayerPanel(whitePlayerPanel, blackPlayerPanel, chessView);

    connect(chessViewModel, &ChessViewModel::boardChanged, chessScene, &ChessScene::onBoardChanged);
    connect(chessViewModel, &ChessViewModel::endPromotion, chessScene, &ChessScene::onPromotionEnded);

    connect(chessViewModel, &ChessViewModel::gameEnded, infoContainer, &InfoView::writeGameResultToDisplay);
    connect(chessViewModel, &ChessViewModel::moveMade, infoContainer, &InfoView::addMoveToDisplay);
    connect(chessViewModel, &ChessViewModel::newGameStarted, infoContainer, &InfoView::clearMoveDisplay);
    connect(chessViewModel, &ChessViewModel::reviewEndedRequest, infoContainer, &InfoView::onReviewEnded);
    connect(chessViewModel, &ChessViewModel::removeLastButFromInfoDisplayRequest, infoContainer, &InfoView::removeLastButFromDisplay);

    connect(chessViewModel, &ChessViewModel::flipBoardToRequest, chessView, &ChessView::flipBoardTo);
    connect(chessViewModel, &ChessViewModel::flipBoardToRequest, bapp, &BoardAndPlayerPanel::flipPlayerPanels);
    connect(chessViewModel, &ChessViewModel::playerPanelsUpdateRequest, bapp, &BoardAndPlayerPanel::playerPanelChanged);
    connect(chessViewModel, &ChessViewModel::syncPiecesWithPanelsRequest, bapp, &BoardAndPlayerPanel::syncPiecesWithPanels);
    connect(chessViewModel, &ChessViewModel::timerChanged, bapp, &BoardAndPlayerPanel::onTimerChanged);
    connect(chessViewModel, &ChessViewModel::setTimersVisible, bapp, &BoardAndPlayerPanel::onSetTimersVisible);
    connect(chessViewModel, &ChessViewModel::activateTimerColorAndDisableOther, bapp, &BoardAndPlayerPanel::onActivateTimerColorAndDisableOther);
    connect(chessViewModel, &ChessViewModel::disableTimers, bapp, &BoardAndPlayerPanel::onDisableTimers);

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

MainWindow::~MainWindow() = default;
