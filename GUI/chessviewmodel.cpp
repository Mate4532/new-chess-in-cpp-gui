#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
}

void ChessViewModel::startGame() {
    if (isGameRunning)
        endGame();

    if (currentSettings.robotSettings.isBotVsBot) {
        startRobotGameLoop();
        return;
    }

    else {
        bm.setupBotsForNormalGame(currentSettings.robotSettings);
    }

    loadNewGame();
    isGameRunning = true;

    if (bm.isRobotToMove()) {
        makeRobotMove();
    }
}

void ChessViewModel::loadNewGame() {
    endGame();

    isBeginnerPos = true;
    bm.loadNewGame();

    visualHistory.clear();
    visualHistory.push_back(bm.getBoardMatrix());
    reviewingPly = -1;

    emit boardChanged();
    emit clearInfoDisplay();
}

void ChessViewModel::endGame() {

    if (isRobotUnderSearch()) {
        stopRobotSearch();
    }

    isGameRunning = false;
    GameResult gr = bm.getGameResult();
    emit gameEnded(gr);
}

bool ChessViewModel::isMovePromotion(int fromX, int fromY, int toX, int toY) const {
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

    if (currentSettings.robotSettings.isBotVsBot) {
        isInBotSimulation = false;
    }

    endGame();
}

void ChessViewModel::movePiece(int fromX, int fromY, int toX, int toY, PieceType promotionPiece) {

    if (reviewingPly != -1) {
        reviewingPly = -1;
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

    if (!isInBotSimulation) {
        visualHistory.push_back(bm.getBoardMatrix());
        reviewingPly = -1;
    }

    emit boardChanged();

    Color moveColor = (Color)(bm.getSideToMove() ^ 1);
    emit moveMade(bm.getPly(), QString::fromStdString(m.toHumanReadable()), moveColor);

    if (bm.didGameEnd())
        endGame();

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
    bool botBsBotSearchTimeChanged = newRs.botVsBotSearchTimeMs != oldRs.botVsBotSearchTimeMs;
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
            bm.setSearchTime(newRs.botVsBotSearchTimeMs);
        }

        else if (whiteRobotChanged || blackRobotChanged || botVsBotGotTurnedOff) {

            isWhiteRobot? bm.setRobot(WHITE) : bm.setPlayer(WHITE);
            bm.setDifficulty(WHITE, newRs.whiteRobotDifficulty);

            isBlackRobot ? bm.setRobot(BLACK) : bm.setPlayer(BLACK);
            bm.setDifficulty(BLACK, newRs.blackRobotDifficulty);

            bm.setupBotsForNormalGame(currentSettings.robotSettings);
        }

        if (botBsBotSearchTimeChanged) {
            bm.setSearchTime(newRs.botVsBotSearchTimeMs);
        }

        updatePlayerPanels();
    }

    if (boardConfigChanged) {
        isBoardFlipped = newBs.isBoardFlipped;
        if (oldS.boardSettings.isBoardFlipped != newS.boardSettings.isBoardFlipped)
            flipBoardToRequest(isBoardFlipped);
        emit boardChanged();
    }

    if (mustStartNewGame) {
        loadNewGame();
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

    robotThread = QThread::create([this]() {
        Move m = bm.MakeRobotMove();
        QMetaObject::invokeMethod(this, [this, m]() {
            this->onRobotMoveFinished(m);
        }, Qt::QueuedConnection);
    });

    connect(robotThread, &QThread::finished, robotThread, &QObject::deleteLater);
    robotThread->start();
}

void ChessViewModel::startRobotGameLoop() {
    if (isRobotUnderSearch()) stopRobotSearch();

    bm.prepareImprovedBotVsOldBot();
    bm.setSearchTime(currentSettings.robotSettings.botVsBotSearchTimeMs);

    simI = 0;
    simJ = 0;
    isInBotSimulation = true;

    disconnect(this, &ChessViewModel::gameEnded, this, &ChessViewModel::advanceSimulation);
    connect(this, &ChessViewModel::gameEnded, this, &ChessViewModel::advanceSimulation);

    QTimer::singleShot(100, this, &ChessViewModel::runNextSimGame);
}

void ChessViewModel::runNextSimGame() {
    if (!isInBotSimulation || simI >= ROBOT_GAMES / 2 || !currentSettings.robotSettings.isBotVsBot) {
        qDebug() << "Szimuláció véget ért.";
        isInBotSimulation = false;
        isGameRunning = false;
        return;
    }

    disconnect(this, &ChessViewModel::gameEnded, this, &ChessViewModel::advanceSimulation);

    if (simJ == 0) {
        currentSimFen = QString::fromStdString(bm.getRandomOpening());
    }

    bm.loadNewGame();
    clearInfoDisplay();
    bm.loadFEN(currentSimFen.toStdString());
    isGameRunning = true;

    emit boardChanged();
    emit clearInfoDisplay();

    connect(this, &ChessViewModel::gameEnded, this, &ChessViewModel::advanceSimulation);

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
    updatePlayerPanels();
}

void ChessViewModel::updatePlayerPanels() {

    bool isWhiteRobot = currentSettings.robotSettings.isWhiteRobot;
    bool isBlackRobot = currentSettings.robotSettings.isBlackRobot;

    QString whitePlayerName = isWhiteRobot ? QString::fromStdString(bm.getRobotNameWithDifficulty(WHITE)) : "Fehér játékos";
    QString whitePlayerIcontPath = isWhiteRobot ? ":/resources/resources/white_robot.png" : ":/resources/resources/white_pawn.png";

    QString blackPlayerName = isBlackRobot ? QString::fromStdString(bm.getRobotNameWithDifficulty(BLACK)) : "Fekete játékos";
    QString blackPlayerIcontPath = isBlackRobot ? ":/resources/resources/black_robot.png" : ":/resources/resources/black_pawn.png";

    emit playerPanelsUpdateRequest(whitePlayerName, whitePlayerIcontPath, WHITE);
    emit playerPanelsUpdateRequest(blackPlayerName, blackPlayerIcontPath, BLACK);
}

void ChessViewModel::onRobotMoveFinished(Move robotMove) {
    if (!isUnderSearch) return;
    isUnderSearch = false;

    if (robotMove.isValid() && isGameRunning) {
        afterMoveBeenMade(robotMove);
    }
}

void ChessViewModel::undoMove() {

    if (!isGameRunning || isInBotSimulation)
        return;

    if (!visualHistory.empty())
        visualHistory.pop_back();
    reviewingPly = -1;

    if (isRobotUnderSearch())
        stopRobotSearch();

    int plyToUndo = bm.isEnemyRobot() ? 2 : 1;

    bm.undoMove(plyToUndo);
    emit boardChanged();
    emit moveUndone(plyToUndo);

    if (bm.isRobotToMove())
        makeRobotMove();
}

std::vector<std::vector<std::pair<PieceType, Color>>> ChessViewModel::getBoardMatrix() const {
    if (reviewingPly >= 0 && reviewingPly < visualHistory.size()) {
        return visualHistory[reviewingPly];
    }
    if (isUnderSearch) {
        return cachedMatrix;
    }
    return bm.getBoardMatrix();
}

void ChessViewModel::reviewHistory(int targetPly) {
    if (targetPly >= 0 && targetPly < visualHistory.size()) {
        if (bm.getPly() == targetPly)
            reviewingPly = -1;
        else
            reviewingPly = targetPly;
        emit boardChanged();
    }
}
