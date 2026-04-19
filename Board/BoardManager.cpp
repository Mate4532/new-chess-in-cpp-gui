#include "Board.h"
#include "BoardManager.h"
#include "UCIParsing.h"
#include "openingloader.h"
#include "versioncontrol.h"

#include <sstream>

using ImpSearcher = ImprovedSearcher::Searcher;
using OSearcher = OldSearcher::Searcher;

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

BoardManager::BoardManager(): board(), resultManager() {
    Attacks::InitAll();
    OpeningLoader::loadOpenings(OPENING_PATH);
}

std::unique_ptr<ISearcher> BoardManager::createBot(SearcherType st) {
    switch (st) {
    case SearcherType::OLD_SEARCHER: return std::make_unique<OSearcher>(board);
    case SearcherType::IMRPOVED_SEARCHER: return std::make_unique<ImpSearcher>(board);
    default: return std::make_unique<ImpSearcher>(board);
    }
}

void BoardManager::goPerft(int perftDepth) {

    std::cout << "Perft(" << perftDepth << ") inditasa..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t nodes = board.PerftDivide(perftDepth);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();

    double nps = (seconds > 0) ? (nodes / seconds) : nodes;

    std::cout << "\n------------------------------------" << std::endl;
    std::cout << "Perft(" << perftDepth << ") eredmenye: " << nodes << " csomopont" << std::endl;
    std::cout << "Idotartam: " << std::fixed << std::setprecision(3) << seconds << " masodperc" << std::endl;
    std::cout << "Sebesseg: " << std::fixed << std::setprecision(0) << nps << " NPS (Nodes Per Second)" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    board.PrintBoard();
}

void BoardManager::resetForNewGame() {
    ClearSearchers();
    isClockRunning = false;
    whiteTimeLeftMs = 0;
    blackTimeLeftMs = 0;
    timeLeftAtPly.clear();
}

void BoardManager::loadBeginnerFEN() {
    loadFEN(newPosFen);
}

void BoardManager::loadFEN(std::string FEN) {
    board.LoadFEN(FEN);
}

std::string BoardManager::getRandomOpening() {
    return OpeningLoader::getRandomFen();
}

void BoardManager::printBestMove() {
    Move best_move = getBestMoveOnBoard();

    std::cout << "Best move: " + square_to_coordinates[best_move.getFrom()] + square_to_coordinates[best_move.getTo()] << std::endl;
}

bool BoardManager::isMovePromotion(int fromX, int fromY, int toX, int toY) {
    Square fromSq = (Square)(fromY * 8 + fromX);

    PieceType p = board.getPieceAt(fromSq, board.getSideToMove());
    if (p != PAWN) return false;

    int targetRank = toY;
    return (targetRank == 0 || targetRank == 7);
}

Move BoardManager::getMove(int fromX, int fromY, int toX, int toY, PieceType promotionPiece)
{
    Square fromSq = (Square)(fromY * 8 + fromX);
    Square toSq = (Square)(toY * 8 + toX);

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    for (int i = 0; i < moves.count; ++i){

        Move& m = moves[i];

        if (m.getFrom() == fromSq && m.getTo() == toSq) {

            bool isPromotion = m.getFlags() & MoveFlag::PROMOTION_FLAG;
            MoveFlag promotionFlag;

            if (isPromotion) {
                promotionFlag = getMoveFlagBasedOnPromotionPiece(promotionPiece);
            }

            const int promotionBits = 0b1011;
            if (isPromotion){
                if ((m.getFlags() & promotionBits) == promotionFlag) {
                    return moves[i];
                }
            }

            else {
                return m;
            }
        }
    }

    return Move();
}

bool BoardManager::MakeMove(Move m) {
    stopTurnClock();

    bool success = board.MakeMove(m);

    if (success && gameMode == GameMode::TOURNAMENT_MODE) {
        if (board.getSideToMove() == BLACK) whiteTimeLeftMs += incrementMs;
        else blackTimeLeftMs += incrementMs;

        saveRemainingTime();
        startTurnClock();
    }

    return success;
}

void BoardManager::undoMove(int plyToUndo) {

    stopTurnClock();

    for (int i = 0; i < plyToUndo; ++i) {
        if (board.getPly() > 0) {
            board.UndoMove(board.getLastMove());
        }
    }

    if (gameMode == GameMode::TOURNAMENT_MODE) {
        int currentPly = board.getPly();
        std::pair<long long, long long> timeRemainingAtPly = timeLeftAtPly[currentPly];
        whiteTimeLeftMs = timeRemainingAtPly.first;
        blackTimeLeftMs = timeRemainingAtPly.second;
        if (!timeLeftAtPly.empty()) {
            timeLeftAtPly.pop_back();
        }
        startTurnClock();
    }
}

Move BoardManager::MakeRobotMove() {

    if (gameMode == GameMode::TOURNAMENT_MODE)
        updateRobotTournementTime();

    Move robot_move;
    std::cout << (board.getSideToMove() == WHITE ? (whiteRobot->getName() + " (feher) ") : (blackRobot->getName() + " (fekete) ")) <<"gondolkodik..." << std::endl;
    if (board.isDebugMode) {
        // uint64_t hash_before = board.getHash();
        // std::cout << "Hash kereses elott: " << board.getHash() << std::endl;
        robot_move = board.getSideToMove() == WHITE ? whiteRobot->GetRobotMove() : blackRobot->GetRobotMove();

        //uint64_t hash_after = board.getHash();
        // std::cout << "Hash kereses utan: " << hash_after << std::endl;
        // std::cout << "Repetition_history merete: " << board.getRepetitionHash().size() << std::endl;
        // if (hash_before != hash_after) {
        //    std::cout << "BAJ VAN\n\n\n\n\n" << std::endl;
        // }
    }
    else {
        robot_move = board.getSideToMove() == WHITE ? whiteRobot->GetRobotMove() : blackRobot->GetRobotMove();
    }
    if (!robot_move.isValid())
        return Move();

    MakeMove(robot_move);
    std::cout << "Robot lepese: " + robot_move.toAlgebraic() << std::endl;

    return robot_move;
}

bool BoardManager::didGameEnd() {

    if (board.IsDraw() || board.IsCheckMate())
        board.PrintBoard(is_white_player, is_black_player);

    if (board.IsDraw()) {
        std::cout << "\nDontetlen!" << std::endl;
        return true;
    }

    if (board.IsCheckMate()) {
        std::cout << "\nSakkmat, " << (board.getSideToMove() == WHITE ? "Fekete" : "Feher") << " nyert!" << std::endl;
        return true;
    }

    return false;
}

GameResult BoardManager::getGameResult(){
    if (whiteTimeLeftMs <= 0) return GameResult::BLACK_WON_ON_TIME;
    if (blackTimeLeftMs <= 0) return GameResult::WHITE_WON_ON_TIME;
    return board.getGameResult();
}

std::string BoardManager::getGameResultString(Color winnerColor, bool isWinnerRobot, GameResult gameResult) {
    if (gameResult == GAME_DID_NOT_END) {
        return "A jatek meg folyamatban van.";
    }

    if (gameResult == DRAW) {
        return "Dontetlen!";
    }

    std::string winnerName;
    if (winnerColor == WHITE) {
        winnerName = isWinnerRobot ? "Feher robot" : "Feher jatekos";
    } else {
        winnerName = isWinnerRobot ? "Fekete robot" : "Fekete jatekos";
    }

    std::string reason;
    switch (gameResult) {
    case WHITE_WON_WITH_CHECKMATE:
    case BLACK_WON_WITH_CHECKMATE:
        reason = "sakk-mattal gyozott.";
        break;
    case WHITE_WON_ON_TIME:
    case BLACK_WON_ON_TIME:
        reason = "idotullepessel gyozott.";
        break;
    case WHITE_GAVE_UP:
    case BLACK_GAVE_UP:
        return winnerName + " gyozott, mert az ellenfele feladta.";
    default:
        reason = "gyozott.";
        break;
    }

    return winnerName + " " + reason;
}

void BoardManager::writeGameResult() {

    if (!whiteRobot || !blackRobot || !isRobot(WHITE) || !isRobot(BLACK)) return;

    GameResult result = getGameResult();

    if (result == GameResult::GAME_DID_NOT_END) return;

    bool whiteWon = (result & GameResult::WHITE_WON) != 0;

    Color winnerColor = whiteWon ? WHITE : BLACK;
    ISearcher* winnerBot = (winnerColor == WHITE) ? whiteRobot.get() : blackRobot.get();

    std::cout << winnerBot->getName() << ", " << getGameResultString(winnerColor, true, result) << std::endl;

    std::string whiteNameToSaveInFile = whiteRobot->getNameToSaveInFile();
    std::string blackNameToSaveInFile = blackRobot->getNameToSaveInFile();

    std::string whiteSourcePath = whiteRobot->getBotDirectoryPath();
    std::string blackSourcePath = blackRobot->getBotDirectoryPath();

    VersionControl::manageBotVersion(whiteNameToSaveInFile, whiteSourcePath);
    VersionControl::manageBotVersion(blackNameToSaveInFile, blackSourcePath);

    resultManager.saveGameResult(result, whiteNameToSaveInFile, whiteSourcePath, blackNameToSaveInFile, blackSourcePath, board.getMoveHistroyInSAN(), board.getBeginnerFen());
}

void BoardManager::startGameLoop() {

    std::string userInput;

    while (true) {

        if (didGameEnd())
            break;

        if ((board.getSideToMove() == WHITE && isRobot(WHITE)) || (board.getSideToMove() == BLACK && isRobot(BLACK))) {
            MakeRobotMove();
        }

        if (didGameEnd())
            break;

        if (isRobot(WHITE) && isRobot(BLACK)) {
            continue;
        }

        board.PrintBoard(is_white_player, is_black_player);

        std::cout << (board.getSideToMove() == WHITE ? "Feher" : "Fekete") << " van lepesben!" << std::endl;
        std::cout << "Add meg a lepest (pl. e2e4): ";
        std::getline(std::cin >> std::ws, userInput);

        std::vector<std::string> trimmed = tokenize(userInput);

        if (trimmed.size() == 0) {
            continue;
        }

        if (trimmed[0] == "exit") break;
        else if (trimmed[0] == "undo") {

            int n = 0;

            if (trimmed.size() == 1) {
                n = 1;
            }
            else {
                n = trimmed[1][0] - '0';
            }
            for (int i = 0; i < n; ++i) {
                if (board.getPly() > 0) {
                    board.UndoMove(board.getLastMove());
                }
            }

            board.PrintBoard(is_white_player, is_black_player);
            continue;
        }

        Move move = UCIParsing::Parse(userInput, board);

        MoveList moves;
        MoveGenerator::GenerateMoves(board, moves);

        if (!moves.contains(move)) {
            std::cout << "A lepes nem ervenyes!" << std::endl;
            continue;
        }

        if (move.getPieceType() != PIECE_NONE) {
            if (board.MakeMove(move)) {
                std::cout << "Sikeres lepes!" << std::endl;
            }
            else {
                std::cout << "Szabalytalan lepes (sakkban maradsz)!" << std::endl;
            }
        }
        else {
            std::cout << "Ervenytelen koordinatak vagy ures mezo!" << std::endl;
        }
    }
}

void BoardManager::startTurnClock() {
    turnStartTime = std::chrono::steady_clock::now();
    isClockRunning = true;
}

void BoardManager::stopTurnClock() {
    if (!isClockRunning) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - turnStartTime).count();

    if (board.getCommittedSideToMove() == WHITE) whiteTimeLeftMs -= elapsed;
    else blackTimeLeftMs -= elapsed;

    isClockRunning = false;
}

long long BoardManager::getTimeRemaining(Color player) const {
    if (isClockRunning && board.getCommittedSideToMove() == player && gameMode == GameMode::TOURNAMENT_MODE) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - turnStartTime).count();
        return std::max(0LL, (player == WHITE ? whiteTimeLeftMs : blackTimeLeftMs) - elapsed);
    }
    return player == WHITE ? whiteTimeLeftMs : blackTimeLeftMs;
}

void BoardManager::setPlayer(Color c) {
    if (c == WHITE) {
        is_white_player = true;
    }

    else {
        is_black_player = true;
    }

}

void BoardManager::setRobot(Color c){
    if (c == WHITE) {
        is_white_player = false;
    }

    else {
        is_black_player = false;
    }
}

void BoardManager::ClearSearchers() {
    if (whiteRobot != nullptr) whiteRobot->ClearSearcher();
    if (blackRobot != nullptr) blackRobot->ClearSearcher();
}

void BoardManager::setupBotsForNormalGame(const RobotSettings& rs) {

    if (rs.isWhiteRobot && (whiteRobot == nullptr || dynamic_cast<ImpSearcher*>(whiteRobot.get()) == nullptr)) {
        whiteRobot = createBot(SearcherType::IMRPOVED_SEARCHER);
        whiteRobot->setDifficulty(rs.whiteRobotDifficulty);
        setRobot(WHITE);
    }

    if (rs.isBlackRobot && (blackRobot == nullptr || dynamic_cast<ImpSearcher*>(blackRobot.get()) == nullptr)) {
        blackRobot = createBot(SearcherType::IMRPOVED_SEARCHER);
        blackRobot->setDifficulty(rs.blackRobotDifficulty);
        setRobot(BLACK);
    }
}

void BoardManager::prepareImprovedBotVsOldBot() {
    if (whiteRobot == nullptr || dynamic_cast<ImpSearcher*>(whiteRobot.get()) == nullptr) {
        whiteRobot = createBot(SearcherType::IMRPOVED_SEARCHER);
        setDifficulty(WHITE, Difficulty::IMPOSSIBLE);
        setRobot(WHITE);
    }

    if (blackRobot == nullptr || dynamic_cast<OSearcher*>(blackRobot.get()) == nullptr) {
        blackRobot = createBot(SearcherType::OLD_SEARCHER);
        setDifficulty(BLACK, Difficulty::IMPOSSIBLE);
        setRobot(BLACK);
    }
}

void BoardManager::SwapRobots() {
    std::swap(whiteRobot, blackRobot);
}

std::string BoardManager::getRobotNameWithDifficulty(Color searcherColor) {
    if (searcherColor == WHITE) {
        if (whiteRobot != nullptr) {
            return whiteRobot->getName() + " (" + whiteRobot->getDifficultyString() + ")";
        }
    } else if (searcherColor == BLACK) {
        if (blackRobot != nullptr) {
            return blackRobot->getName() + " (" + blackRobot->getDifficultyString() + ")";
        }
    }
    return "";
}

void BoardManager::setDifficulty(Color c, Difficulty d) {
    if (c == WHITE && whiteRobot != nullptr) whiteRobot->setDifficulty(d);
    else if (blackRobot != nullptr) blackRobot->setDifficulty(d);
}

void BoardManager::setRobotTimeUsageMode(RobotTimeUsageMode rtum) {
    if (whiteRobot != nullptr) whiteRobot->setTimeUsageMode(rtum);
    if (blackRobot != nullptr) blackRobot->setTimeUsageMode(rtum);
}

void BoardManager::setFixedTimePerMove(long long timePerMoveMs) {
    if (whiteRobot != nullptr) whiteRobot->setFixedTimePerMove(timePerMoveMs);
    if (blackRobot != nullptr)blackRobot->setFixedTimePerMove(timePerMoveMs);
}

void BoardManager::updateRobotTournementTime() {
    if (whiteRobot != nullptr) whiteRobot->updateTournementTime(getTimeRemaining(WHITE));
    if (blackRobot != nullptr) blackRobot->updateTournementTime(getTimeRemaining(BLACK));
}

void BoardManager::setTournementTime(long long tournementTimeMs, long long incrementMs) {
    this->whiteTimeLeftMs = tournementTimeMs;
    this->blackTimeLeftMs = tournementTimeMs;
    this->incrementMs = incrementMs;

    if (whiteRobot != nullptr) whiteRobot->setTournamentTime(tournementTimeMs, incrementMs);
    if (blackRobot != nullptr) blackRobot->setTournamentTime(tournementTimeMs, incrementMs);

    saveRemainingTime();
}

void BoardManager::setGameMode(GameMode gm) {
    this->gameMode = gm;

    switch(gm) {
    case GameMode::UNLIMITED_THINKING_TIME:
        rtum = RobotTimeUsageMode::FIXED_TIME;
        break;

    case GameMode::TOURNAMENT_MODE:
        rtum = RobotTimeUsageMode::TOURNEMENT_TIME;
        break;

    default:
        rtum = RobotTimeUsageMode::FIXED_TIME;
        break;
    }

    setRobotTimeUsageMode(rtum);
}

void BoardManager::stopRobotCalculation() {
    if (whiteRobot != nullptr && whiteRobot->isUnderSearch()) whiteRobot->stopSearch();
    if (blackRobot != nullptr && blackRobot->isUnderSearch()) blackRobot->stopSearch();
}

std::vector<std::pair<int, int>> BoardManager::getLegalMovesForPiece(int file, int rank) {
    Square foundSquare = static_cast<Square>(rank * 8 + file);

    MoveList moves = board.getCurrentLegalMoves();

    std::vector<std::pair<int, int>> squares;

    for (int i = 0; i < moves.size(); ++i) {
        Move m = moves[i];

        if (m.getFrom() == foundSquare) {
            Square target = m.getTo();

            int targetFile = target % 8;
            int targetRank = target / 8;

            squares.push_back({targetFile, targetRank});
        }
    }
    std::sort(squares.begin(), squares.end());
    squares.erase(std::unique(squares.begin(), squares.end()), squares.end());

    return squares;
}

std::pair<int, int> BoardManager::getKingSquare(Color kingColor, int ply) {
    auto boardMatrix = board.getBoardMatrix(ply);

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            auto& square = boardMatrix[i][j];

            if (square.first == PieceType::KING && square.second == kingColor) {
                int file = j;
                int logicalRank = 7 - i;

                return {file, logicalRank};
            }
        }
    }

    return {-1, -1};
}

void BoardManager::updateClocks(long long elapsedMs) {
    if (gameMode != GameMode::TOURNAMENT_MODE) return;

    if (board.getSideToMove() == WHITE) {
        whiteTimeLeftMs -= elapsedMs;
        if (whiteTimeLeftMs < 0) whiteTimeLeftMs = 0;
    } else {
        blackTimeLeftMs -= elapsedMs;
        if (blackTimeLeftMs < 0) blackTimeLeftMs = 0;
    }
}

void BoardManager::saveRemainingTime() {
    std::pair<long long, long long> timeRemains(whiteTimeLeftMs, blackTimeLeftMs);
    timeLeftAtPly.push_back(timeRemains);
}

