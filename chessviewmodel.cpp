#include "chessviewmodel.h"
#include <QThread>

ChessViewModel::ChessViewModel(BoardManager& b, QObject* parent)
    : QObject(parent), bm(b)
{
    connect(&robotWatcher, &QFutureWatcher<void>::finished, this, &ChessViewModel::onRobotMoveFinished);
}

void ChessViewModel::startGame() {
    if (isGameRunning || isRobotUnderSearch())
        endGame();

    if (currentSettings.robotSettings.isBotVsBot) {
        startRobotGameLoop();
        return;
    }

    else {
        bm.setupBotsForNormalGame();
    }

    if (!isBeginnerPos)
        bm.loadNewGame();

    isGameRunning = true;
    isBeginnerPos = true;
    emit boardChanged();

    if (bm.isRobotToMove()) {
        makeRobotMove();
    }
}

 void ChessViewModel::loadNewGame() {

     endGame();

     isBeginnerPos = true;
     bm.loadNewGame();
     emit boardChanged();
 }

void ChessViewModel::endGame() {

    if (isRobotUnderSearch()) {
        stopRobotSearch();
    }

    isGameRunning = false;
    emit gameEnded();
}

bool ChessViewModel::isMovePromotion(int fromX, int fromY, int toX, int toY) const {
    return bm.isMovePromotion(fromX, fromY, toX, toY);
}


bool ChessViewModel::isRobotUnderSearch() const {
    return isUnderSearch;
}

void ChessViewModel::switchBots() {

}

void ChessViewModel::stopRobotSearch() {

    if (!isRobotUnderSearch())
        return;

    bm.stopRobotCalculation();
}

void ChessViewModel::currentPlayerGaveUp() {
    if (!isGameRunning)
        return;
    endGame();
}

void ChessViewModel::movePiece(int fromX, int fromY, int toX, int toY, PieceType promotionPiece) {

    if (!isGameRunning && isBeginnerPos && !bm.isRobotToMove())
        startGame();

    if (isRobotUnderSearch() || !isGameRunning)
        return;

    if (isBeginnerPos)
        isBeginnerPos = false;

    bool validMove = bm.MakeMove(fromX, fromY, toX, toY, promotionPiece);

    if (validMove) {
        afterMoveBeenMade();
    }
}

void ChessViewModel::afterMoveBeenMade() {
    emit boardChanged();

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

    QFuture<void> future = QtConcurrent::run(&BoardManager::MakeRobotMove, &bm);

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

    afterMoveBeenMade();
}

void ChessViewModel::undoLastMove(){
    if (!isGameRunning)
        return;

    if (isRobotUnderSearch())
        stopRobotSearch();

    bm.undoLastMove();
    emit boardChanged();

    if (bm.isRobotToMove())
        makeRobotMove();
}

std::vector<std::vector<std::pair<PieceType, Color>>> ChessViewModel::getBoardMatrix() const {
    if (isRobotUnderSearch()) {
        return cachedMatrix;
    }
    return bm.getBoardMatrix();
}
