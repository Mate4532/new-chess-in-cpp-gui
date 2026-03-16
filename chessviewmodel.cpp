#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
    connect(&robotWatcher, &QFutureWatcher<Move>::finished, this, &ChessViewModel::onRobotMoveFinished);
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

    if (!isRobotUnderSearch())
        return;

    bm.stopRobotCalculation();
    robotWatcher.waitForFinished();
}

void ChessViewModel::currentPlayerGaveUp() {
    if (!isGameRunning)
        return;
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

    visualHistory.push_back(bm.getBoardMatrix());
    reviewingPly = -1;

    emit boardChanged();

    Color moveColor = (Color)(bm.getSideToMove() ^ 1);
    emit moveMade(bm.getFullMoveNumber(), bm.getPly(), QString::fromStdString(m.toHumanReadable()), moveColor);

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

    if (robotConfigChanged) {
        endGame();
    }

    if (robotConfigChanged) {

        if (newRs.isBotVsBot) {
            bm.setRobot(WHITE); bm.setDifficulty(WHITE, Difficulty::IMPOSSIBLE);
            bm.setRobot(BLACK); bm.setDifficulty(BLACK, Difficulty::IMPOSSIBLE);

            bm.setSearchTime(newRs.botVsBotSearchTimeMs);
        }

        else {

            newRs.isWhiteRobot ? bm.setRobot(WHITE) : bm.setPlayer(WHITE);
            bm.setDifficulty(WHITE, newRs.whiteRobotDifficulty);

            newRs.isBlackRobot ? bm.setRobot(BLACK) : bm.setPlayer(BLACK);
            bm.setDifficulty(BLACK, newRs.blackRobotDifficulty);
        }
    }

    if (boardConfigChanged) {
        isBoardFlipped = newBs.isBoardFlipped;
        if (oldS.boardSettings.isBoardFlipped != newS.boardSettings.isBoardFlipped)
            flipBoardToRequest(isBoardFlipped);
        emit boardChanged();
    }

    if (robotConfigChanged) {
        loadNewGame();
    }
}

void ChessViewModel::refreshView() {
    if (!isRobotUnderSearch())
        emit boardChanged();
}

void ChessViewModel::makeRobotMove() {
    if (isRobotUnderSearch() || !isGameRunning) return;

    if (isBeginnerPos)
        isBeginnerPos = false;

    cachedMatrix = bm.getBoardMatrix();
    isUnderSearch = true;

    QFuture<Move> future = QtConcurrent::run(&BoardManager::MakeRobotMove, &bm);

    robotWatcher.setFuture(future);
}

void ChessViewModel::startRobotGameLoop() {

    if (isRobotUnderSearch()) stopRobotSearch();

    bm.prepareImprovedBotVsOldBot();
    bm.setSearchTime(currentSettings.robotSettings.botVsBotSearchTimeMs);
    isGameRunning = false;
    stopBotSimulation = false;

    for (int i = 0; i < ROBOT_GAMES / 2; ++i) {

        if (stopBotSimulation) break;

        std::string fen = bm.getRandomOpening();

        for (int j = 0; j < 2; ++j) {

            if (stopBotSimulation) break;

            bm.ClearSearchers();
            bm.ClearBoard();
            bm.loadFEN(fen);
            isGameRunning = true;

            emit boardChanged();

            makeRobotMove();

            QEventLoop loop;

            connect(this, &ChessViewModel::gameEnded, &loop, &QEventLoop::quit);

            loop.exec();

            bm.writeGameResult();
            bm.SwapRobots();

        }
    }

    qDebug() << "Minden szimulacio lefutott.";
    isGameRunning = false;
}

void ChessViewModel::onRobotMoveFinished() {

    cachedMatrix.clear();
    isUnderSearch = false;

    Move robotMove = robotWatcher.result();

    afterMoveBeenMade(robotMove);
}

void ChessViewModel::undoMove() {

    if (!isGameRunning)
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

    if (isRobotUnderSearch()) {
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
