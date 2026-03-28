#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
}

void ChessViewModel::startGame() {

    if (isInBotSimulation) {
        stopRobotGameLoop();
    }

    if (isGameRunning) {
        endGame();
    }

    if (currentSettings.robotSettings.isBotVsBot) {
        startRobotGameLoop();
        return;
    }

    bm.setupBotsForNormalGame(currentSettings.robotSettings);

    loadBeginnerFEN();
    isGameRunning = true;

    if (bm.isRobotToMove()) {
        makeRobotMove();
    }
}

void ChessViewModel::loadNewGame() {
    endGame();

    isBeginnerPos = true;
    bm.resetForNewGame();

    emit newGameStarted();
    reviewEnded();
    updatePlayerPanelAtNewPos();
    emit boardChanged();
}

void ChessViewModel::endGame() {

    if (isRobotUnderSearch()) {
        stopRobotSearch();
    }

    if (isUnderPromotion)
        emit endPromotion();

    isGameRunning = false;
    GameResult gr = bm.getGameResult();
    if (!isInBotSimulation) {
        emit gameEnded(gr);
    }
    else {
        emit robotSimulationEnded();
    }
}

bool ChessViewModel::isMovePromotion(int fromX, int fromY, int toX, int toY) {
    return bm.isMovePromotion(fromX, fromY, toX, toY);
}

bool ChessViewModel::isRobotUnderSearch() const {
    return isUnderSearch;
}

void ChessViewModel::stopRobotSearch() {
    if (!isUnderSearch) return;

    isUnderSearch = false;
    bm.stopRobotCalculation();

    if (robotThread->isRunning()){
        robotThread->wait();
    }
}

void ChessViewModel::currentPlayerGaveUp() {
    if (!isGameRunning) return;

    if (isInBotSimulation) {
        stopRobotGameLoop();
    }

    bm.currentPlayerGaveUp();
    endGame();
}

void ChessViewModel::movePiece(int fromX, int fromY, int toX, int toY, PieceType promotionPiece) {

    if (reviewingPly != -1) {
        reviewEnded();
        emit boardChanged();
        return;
    }

    if (!isGameRunning && isBeginnerPos && !bm.isRobotToMove())
        startGame();

    if (isRobotUnderSearch() || !isGameRunning)
        return;

    if (isBeginnerPos)
        isBeginnerPos = false;

    Move m = bm.getMove(fromX, fromY, toX, toY, promotionPiece);
    bool isMoveValid = m.isValid();

    if (isMoveValid) {

        bool isMoveLegal = bm.MakeMove(m);
        if (isMoveLegal)
            afterMoveBeenMade(m);
    }
}

void ChessViewModel::afterMoveBeenMade(Move m) {

    if (!isInBotSimulation)
        reviewEnded();

    emit boardChanged();

    Color currentPlayer = bm.getSideToMove();
    Color lastMovedColor = (Color)(currentPlayer ^ 1);

    int pieces[2][6];

    if (bm.wasMoveCapture(m) || bm.wasMovePromotion(m)) {
        bm.getPieceCounts(pieces);
        syncPiecesWithPanelsRequest(pieces);
    }

    if (bm.didGameEnd())
        endGame();

    int ply = bm.getPly();
    std::string checkString = bm.wasMoveCheck(ply) ? "+" : "";

    emit moveMade(ply, QString::fromStdString(m.toHumanReadable(false) + checkString), lastMovedColor, m.getPieceType());

    if (isGameRunning && bm.isRobotToMove())
        makeRobotMove();
}

void ChessViewModel::loadSettings(AllSettings& allS) {

    AllSettings a;

    updateSettings(a, allS);
}

void ChessViewModel::updateSettings(AllSettings& oldS, AllSettings& newS) {

    currentSettings = newS;

    RobotSettings& oldRs = oldS.robotSettings;
    RobotSettings& newRs = newS.robotSettings;
    BoardSettings& oldBs = oldS.boardSettings;
    BoardSettings& newBs = newS.boardSettings;

    bool robotConfigChanged = (newRs != oldRs);
    bool boardConfigChanged = (newBs != oldBs);

    bool botVsBotChanged = newRs.isBotVsBot != oldRs.isBotVsBot;
    bool botSearchTimeChanged = newRs.botSearchTimeMs != oldRs.botSearchTimeMs;
    bool whiteRobotChanged = newRs.isWhiteRobot != oldRs.isWhiteRobot || newRs.whiteRobotDifficulty != oldRs.whiteRobotDifficulty;
    bool blackRobotChanged = newRs.isBlackRobot != oldRs.isBlackRobot || newRs.blackRobotDifficulty != oldRs.blackRobotDifficulty;

    bool botVsBotGotTurnedOn = newRs.isBotVsBot && !oldRs.isBotVsBot;
    bool botVsBotGotTurnedOff = !newRs.isBotVsBot && oldRs.isBotVsBot;

    bool mustStartNewGame = botVsBotChanged || whiteRobotChanged || blackRobotChanged;

    if (mustStartNewGame) {
        endGame();
    }

    if (robotConfigChanged) {

        if (botVsBotChanged && oldRs.isBotVsBot) {
            isInBotSimulation = false;
        }

        bool isWhiteRobot = newRs.isWhiteRobot;
        bool isBlackRobot = newRs.isBlackRobot;

        if (botVsBotGotTurnedOn) {
            bm.setRobot(WHITE); bm.setDifficulty(WHITE, Difficulty::IMPOSSIBLE);
            bm.setRobot(BLACK); bm.setDifficulty(BLACK, Difficulty::IMPOSSIBLE);

            bm.prepareImprovedBotVsOldBot();
        }

        else if (whiteRobotChanged || blackRobotChanged || botVsBotGotTurnedOff) {

            isWhiteRobot? bm.setRobot(WHITE) : bm.setPlayer(WHITE);
            bm.setDifficulty(WHITE, newRs.whiteRobotDifficulty);

            isBlackRobot ? bm.setRobot(BLACK) : bm.setPlayer(BLACK);
            bm.setDifficulty(BLACK, newRs.blackRobotDifficulty);

            bm.setupBotsForNormalGame(currentSettings.robotSettings);
        }

        if (botSearchTimeChanged) {
            bm.setSearchTime(newRs.botSearchTimeMs);
        }

        updatePlayerPanelsIconAndLabel();
    }

    if (boardConfigChanged) {
        isBoardFlipped = newBs.isBoardFlipped;
        if (oldS.boardSettings.isBoardFlipped != newS.boardSettings.isBoardFlipped)
            flipBoardToRequest(isBoardFlipped);
        emit boardChanged();
    }

    if (mustStartNewGame) {
        loadBeginnerFEN();
    }
}

void ChessViewModel::refreshView() {
    if (!isRobotUnderSearch())
        emit boardChanged();
}

void ChessViewModel::makeRobotMove() {
    if (isUnderSearch || !isGameRunning) return;

    isUnderSearch = true;
    cachedMatrix = bm.getBoardMatrix();

    robotSearchTimer.restart();

    robotThread = QThread::create([this]() {
        Move m = bm.MakeRobotMove();
        QMetaObject::invokeMethod(this, [this, m]() {
            this->onRobotMoveFinished(m);
        }, Qt::QueuedConnection);
    });

    connect(robotThread, &QThread::finished, robotThread, &QObject::deleteLater);
    robotThread->start();
}

void ChessViewModel::stopRobotGameLoop() {
    isInBotSimulation = false;
    disconnect(this, &ChessViewModel::gameEnded, this, &ChessViewModel::advanceSimulation);
}

void ChessViewModel::loadBeginnerFEN() {
    bm.loadBeginnerFEN();

    loadNewGame();
}

void ChessViewModel::loadFEN(std::string fen) {
    bm.loadFEN(fen);

    loadNewGame();
}

void ChessViewModel::startRobotGameLoop() {
    if (isRobotUnderSearch()) stopRobotSearch();

    simI = 0;
    simJ = 0;
    isInBotSimulation = true;

    disconnect(this, &ChessViewModel::robotSimulationEnded, this, &ChessViewModel::advanceSimulation);
    connect(this, &ChessViewModel::robotSimulationEnded, this, &ChessViewModel::advanceSimulation);

    runNextSimGame();
}

void ChessViewModel::runNextSimGame() {
    if (!isInBotSimulation || simI >= ROBOT_GAMES / 2 || !currentSettings.robotSettings.isBotVsBot) {
        qDebug() << "Szimuláció véget ért.";
        isInBotSimulation = false;
        isGameRunning = false;
        return;
    }

    disconnect(this, &ChessViewModel::robotSimulationEnded, this, &ChessViewModel::advanceSimulation);

    if (simJ == 0) {
        currentSimFen = QString::fromStdString(bm.getRandomOpening());
    }

    loadNewGame();
    loadFEN(currentSimFen.toStdString());
    isGameRunning = true;

    emit boardChanged();

    connect(this, &ChessViewModel::robotSimulationEnded, this, &ChessViewModel::advanceSimulation);

    QTimer::singleShot(100, this, &ChessViewModel::makeRobotMove);
}

void ChessViewModel::advanceSimulation() {
    if (!isInBotSimulation || !currentSettings.robotSettings.isBotVsBot) return;

    bm.writeGameResult();
    swapRobots();

    simJ++;
    if (simJ >= 2) {
        simJ = 0;
        simI++;
    }

    QTimer::singleShot(200, this, &ChessViewModel::runNextSimGame);
}

void ChessViewModel::swapRobots() {
    bm.SwapRobots();
    updatePlayerPanelsIconAndLabel();
}

void ChessViewModel::updatePlayerPanelsIconAndLabel() {

    bool isWhiteRobot = bm.isRobot(WHITE);
    bool isBlackRobot = bm.isRobot(BLACK);

    QString whitePlayerName = isWhiteRobot ? QString::fromStdString(bm.getRobotNameWithDifficulty(WHITE)) : "Fehér játékos";
    QString whitePlayerIcontPath = isWhiteRobot ? ":/resources/resources/white_robot.png" : ":/resources/resources/white_pawn.png";

    QString blackPlayerName = isBlackRobot ? QString::fromStdString(bm.getRobotNameWithDifficulty(BLACK)) : "Fekete játékos";
    QString blackPlayerIcontPath = isBlackRobot ? ":/resources/resources/black_robot.png" : ":/resources/resources/black_pawn.png";

    emit playerPanelsUpdateRequest(whitePlayerName, whitePlayerIcontPath, WHITE);
    emit playerPanelsUpdateRequest(blackPlayerName, blackPlayerIcontPath, BLACK);
}

void ChessViewModel::updatePlayerPanelAtNewPos() {
    int allPieces[2][6];
    bm.getPieceCounts(allPieces);
    emit syncPiecesWithPanelsRequest(allPieces);
}


void ChessViewModel::onRobotMoveFinished(Move robotMove) {
    if (!isUnderSearch) return;

    qint64 elapsed = robotSearchTimer.elapsed();
    qint64 remaining = minMsBeforeRobotMove - elapsed;

    if (remaining > 0) {
        QTimer::singleShot(remaining, this, [this, robotMove]() {
            if (!isUnderSearch || !isGameRunning) return;

            isUnderSearch = false;
            if (robotMove.isValid()) {
                afterMoveBeenMade(robotMove);
            }
        });
    } else {
        isUnderSearch = false;
        if (robotMove.isValid() && isGameRunning) {
            afterMoveBeenMade(robotMove);
        }
    }
}

void ChessViewModel::undoMove() {

    if (!isGameRunning || isInBotSimulation)
        return;

    reviewEnded();
    if (isUnderPromotion)
        emit endPromotion();

    if (isRobotUnderSearch())
        stopRobotSearch();

    int plyToUndo = bm.isEnemyRobot() ? 2 : 1;

    for (int i = 0; i < plyToUndo; ++i) {
        emit removeLastButFromInfoDisplayRequest();
    }

    bm.undoMove(plyToUndo);
    emit boardChanged();

    int pieces[2][6];
    bm.getPieceCounts(pieces);
    emit syncPiecesWithPanelsRequest(pieces);

    if (bm.isRobotToMove())
        makeRobotMove();
}

MoveInfo ChessViewModel::getMoveInfo() {

    if (reviewingPly >= 0) {
        return bm.getMoveInfo(reviewingPly);
    }

    return bm.getMoveInfo();
}

std::vector<std::vector<std::pair<PieceType, Color>>> ChessViewModel::getBoardMatrix() const {

    if (reviewingPly >= 0) {
        return bm.getBoardMatrix(reviewingPly);
    }
    if (isUnderSearch) {
        return cachedMatrix;
    }
    return bm.getBoardMatrix();
}

void ChessViewModel::reviewHistory(int targetPly) {

    int currentPly = bm.getPly();
    if (isUnderPromotion)
        emit endPromotion();

    if (targetPly >= 0 && targetPly <= currentPly) {
        if (targetPly == currentPly)
            reviewEnded();
        else
            reviewingPly = targetPly;
        emit boardChanged();

        int pieces[2][6];
        bm.getPieceCounts(pieces, reviewingPly);
        emit syncPiecesWithPanelsRequest(pieces);
    }
}


void ChessViewModel::reviewEnded() {
    reviewingPly = -1;
}
