#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &ChessViewModel::handleClockTick);
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

    loadFEN(currentSettings.boardSettings.beginnerPosFEN);
    isGameRunning = true;

    if (currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
        startClock();
    }

    if (bm.isRobotToMove()) {
        makeRobotMove();
    }
}

void ChessViewModel::loadNewGame() {
    endGame();

    isBeginnerPos = true;
    bm.resetForNewGame();
    resetClock();

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
    stopClock();
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
    currentSearchId++;

    bm.stopRobotCalculation();

    if (robotThread) {
        robotThread->wait();
        delete robotThread;
        robotThread = nullptr;
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

bool ChessViewModel::movePiece(int fromX, int fromY, int toX, int toY, PieceType promotionPiece) {

    Move m = bm.getMove(fromX, fromY, toX, toY, promotionPiece);
    bool isMoveValid = m.isValid();

    if (!isMoveValid) return false;

    if (isUnderReview()) {
        reviewEnded();
        emit boardChanged();
        return false;
    }

    if (!isGameRunning && isBeginnerPos && !bm.isRobotToMove())
        startGame();

    if (isRobotUnderSearch() || !isGameRunning)
        return false;

    bool isMoveLegal = bm.MakeMove(m);
    if (isMoveLegal)
        afterMoveBeenMade();

    return isMoveValid;
}

void ChessViewModel::afterMoveBeenMade() {

    if (!isInBotSimulation)
        reviewEnded();

    emit boardChanged();

    updatePlayerPanelAtNewPos();

    if (isBeginnerPos)
        isBeginnerPos = false;

    if (bm.didGameEnd())
        endGame();

    int ply = bm.getPly();

    Color currentPlayer = bm.getSideToMove();
    Color lastMovedColor = (Color)(currentPlayer ^ 1);

    QString moveSAN = QString::fromStdString(bm.convertMoveToSAN(ply, false));
    Move m = bm.getMove(ply);

    emit moveMade(ply, moveSAN, lastMovedColor, m.getPieceType());

    if (isGameRunning && bm.isRobotToMove())
        makeRobotMove();
}

void ChessViewModel::loadSettings(AllSettings& allS) {

    AllSettings a;

    updateSettings(a, allS);
}

void ChessViewModel::updateSettings(AllSettings& oldS, AllSettings& newS) {

    currentSettings = newS;
    bm.setSettings(newS);

    RobotSettings& oldRs = oldS.robotSettings;
    RobotSettings& newRs = newS.robotSettings;
    BoardSettings& oldBs = oldS.boardSettings;
    BoardSettings& newBs = newS.boardSettings;
    TimeSettings& oldTs = oldS.timeSettings;
    TimeSettings& newTs = newS.timeSettings;

    bool robotConfigChanged = newRs != oldRs;
    bool boardConfigChanged = newBs != oldBs;
    bool timeConfigChanged = newTs != oldTs;

    bool botVsBotChanged = newRs.isBotVsBot != oldRs.isBotVsBot;
    bool botSearchTimeChanged = newRs.botSearchTimeMs != oldRs.botSearchTimeMs;
    bool whiteRobotChanged = newRs.isWhiteRobot != oldRs.isWhiteRobot || newRs.whiteRobotDifficulty != oldRs.whiteRobotDifficulty;
    bool blackRobotChanged = newRs.isBlackRobot != oldRs.isBlackRobot || newRs.blackRobotDifficulty != oldRs.blackRobotDifficulty;

    bool botVsBotGotTurnedOn = newRs.isBotVsBot && !oldRs.isBotVsBot;
    bool botVsBotGotTurnedOff = !newRs.isBotVsBot && oldRs.isBotVsBot;

    bool FENChanged = newBs.beginnerPosFEN != oldBs.beginnerPosFEN;

    bool mustStartNewGame = botVsBotChanged || whiteRobotChanged || blackRobotChanged || FENChanged || timeConfigChanged;

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
            bm.prepareImprovedBotVsOldBot();
        }

        if (whiteRobotChanged || blackRobotChanged || botVsBotGotTurnedOff) {

            if (!isWhiteRobot) bm.setPlayer(WHITE);
            else bm.setRobot(WHITE);
            if (!isBlackRobot) bm.setPlayer(BLACK);
            else bm.setRobot(BLACK);

            bm.setupBotsForNormalGame(currentSettings.robotSettings);
        }

        if (botSearchTimeChanged) {
            bm.setFixedTimePerMove(newRs.botSearchTimeMs);
        }

        updatePlayerPanelsIconAndLabel();
    }

    if (boardConfigChanged) {
        isBoardFlipped = newBs.isBoardFlipped;
        if (oldS.boardSettings.isBoardFlipped != newS.boardSettings.isBoardFlipped)
            flipBoardToRequest(isBoardFlipped);
        emit boardChanged();
    }

    if (timeConfigChanged) {

        bool isTimerVisible = newTs.gm == GameMode::TOURNAMENT_MODE;
        emit setTimersVisibility(isTimerVisible);

        if (newTs.gm == GameMode::TOURNAMENT_MODE)
            currentMsBeforeRobotMove = minMsBeforeRobotMove;
        else
            currentMsBeforeRobotMove = baseMsBeforeRobotMove;
    }

    if (mustStartNewGame) {
        loadFEN(newBs.beginnerPosFEN);
    }
}

void ChessViewModel::refreshView() {
    if (!isRobotUnderSearch())
        emit boardChanged();
}

void ChessViewModel::makeRobotMove() {
    if (isUnderSearch || !isGameRunning) return;

    if (robotThread) {
        if (robotThread->isRunning()) {
            bm.stopRobotCalculation();
            robotThread->wait();
        }
        delete robotThread;
        robotThread = nullptr;
    }

    isUnderSearch = true;
    cachedMatrix = bm.getBoardMatrix();
    robotSearchTimer.restart();

    currentSearchId++;
    int searchIdForThisThread = currentSearchId;

    robotThread = QThread::create([this, searchIdForThisThread]() {
        Move m = bm.MakeRobotMove();

        QMetaObject::invokeMethod(this, [this, m, searchIdForThisThread]() {
            this->onRobotMoveFinished(m, searchIdForThisThread);
        }, Qt::QueuedConnection);
    });

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

    loadFEN(currentSimFen.toStdString());
    isGameRunning = true;

    if (currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
        startClock();
    }

    emit boardChanged();

    connect(this, &ChessViewModel::robotSimulationEnded, this, &ChessViewModel::advanceSimulation);

    QTimer::singleShot(100, this, &ChessViewModel::makeRobotMove);
}

void ChessViewModel::advanceSimulation() {
    if (!isInBotSimulation || !currentSettings.robotSettings.isBotVsBot) return;

    if (currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
        stopClock();
    }

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

void ChessViewModel::handleClockTick() {
    if (!isGameRunning || reviewingPly != -1) return;

    qint64 wTime = bm.getTimeRemaining(WHITE);
    qint64 bTime = bm.getTimeRemaining(BLACK);

    emit timerChanged(WHITE, formatTime(wTime));
    emit timerChanged(BLACK, formatTime(bTime));

    if (wTime <= 0 || bTime <= 0) {
        clockTimer->stop();
        endGame();
    }
}

void ChessViewModel::startClock() {
    clockTimer->start(clockPullTimeMs);
    bm.startTurnClock();
    updatePlayerTimers();
}

void ChessViewModel::stopClock() {
    clockTimer->stop();
    bm.stopTurnClock();
}

void ChessViewModel::resetClock() {
    TimeSettings ts = currentSettings.timeSettings;

    bm.setTournementTime(ts.getTournementTimeMs(), ts.getIncrementMs());
    QString timerString = formatTime(ts.getTournementTimeMs());
    emit tournementModeStarted();
    emit timerChanged(WHITE, timerString);
    emit timerChanged(BLACK, timerString);
    emit disableTimers();
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
    if (isUnderReview()) return;

    int allPieces[2][6];
    bm.getPieceCounts(allPieces);
    emit syncPiecesWithPanelsRequest(allPieces);
    updatePlayerTimers();
}

void ChessViewModel::updatePlayerPanelAtReview() {
    int allPieces[2][6];
    bm.getPieceCounts(allPieces, reviewingPly);
    updatePlayerTimers();
    emit syncPiecesWithPanelsRequest(allPieces);
    emit timerChanged(WHITE, formatTime(bm.getTimeLeft(WHITE, reviewingPly)));
    emit timerChanged(BLACK, formatTime(bm.getTimeLeft(BLACK, reviewingPly)));
}

void ChessViewModel::updatePlayerTimers() {
    bool isWhiteToMove = bm.getSideToMove() == WHITE;
    emit activateTimers(isWhiteToMove, !isWhiteToMove);
}

QString ChessViewModel::formatTime(qint64 remainingMs) const {
    if (remainingMs <= 0) {
        return "0:00.0";
    }

    qint64 displayMs;

    if (remainingMs <= 10000) {
        displayMs = ((remainingMs + 99) / 100) * 100;
    } else {
        displayMs = ((remainingMs + 999) / 1000) * 1000;
    }

    qint64 totalSeconds = displayMs / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;

    if (minutes >= 60) {
        qint64 hours = minutes / 60;
        minutes = minutes % 60;
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    if (displayMs < 10000) {
        qint64 tenths = (displayMs % 1000) / 100;
        return QString("%1:%2.%3")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'))
            .arg(tenths);
    }

    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void ChessViewModel::onRobotMoveFinished(Move robotMove, int searchId) {

    if (this->currentSearchId != searchId || !isUnderSearch) return;

    qint64 elapsed = robotSearchTimer.elapsed();
    qint64 remaining = currentMsBeforeRobotMove - elapsed;

    if (remaining > 0) {
        QTimer::singleShot(remaining, this, [this, robotMove, searchId]() {
            if (this->currentSearchId != searchId || !isUnderSearch || !isGameRunning) {
                return;
            }

            isUnderSearch = false;
            if (robotMove.isValid()) {
                afterMoveBeenMade();
            }
        });
    } else {
        isUnderSearch = false;
        if (robotMove.isValid() && isGameRunning) {
            afterMoveBeenMade();
        }
    }
}

void ChessViewModel::undoMove() {

    if (!isGameRunning || isInBotSimulation || bm.getPly() <= 0)
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
    updatePlayerPanelAtNewPos();

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

    if (isUnderReview()) {
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

        updatePlayerPanelAtReview();
    }
}

void ChessViewModel::reviewEnded() {
    reviewingPly = -1;
    emit reviewEndedRequest(bm.getPly());
}

std::pair<int, int> ChessViewModel::getKingInCheckCoords() {
    int targetPly = isUnderReview() ? reviewingPly : bm.getPly();

    if (!bm.wasMoveCheck(targetPly)) {
        return {-1, -1};
    }

    Color sideInCheck = bm.getSideToMove(targetPly);

    return bm.getKingSquare(sideInCheck, targetPly);
}
