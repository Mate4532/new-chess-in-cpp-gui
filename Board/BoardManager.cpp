#include "Board.h"
#include "BoardManager.h"
#include "UCIParsing.h"
#include "openingloader.h"
#include "versioncontrol.h"
#include "filemanager.h"
#include "HalfKAv2.h"

#include <sstream>
#include <atomic>

#ifndef NNUE_FILE
#error "NNUE_FILE is not defined, set it in CMakeLists.txt"
#endif

#ifndef NNUE_HALFKAV2_FILE
#define NNUE_HALFKAV2_FILE ""
#endif

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

BoardManager::BoardManager() : board(), resultManager() {
    static std::once_flag initFlag;
    std::call_once(initFlag, [this]() {
        Attacks::InitAll();
        const std::string nnuePath = FileManager::findNnueFile(NNUE_FILE);
        if (nnuePath.empty()) {
            std::cerr << "ERROR: NNUE file not found: nnues/" << NNUE_FILE << std::endl;
        } else {
            nnue_init(nnuePath.c_str());
        }

        const std::string halfkaFile = NNUE_HALFKAV2_FILE;
        if (!halfkaFile.empty()) {
            const std::string halfkaPath = FileManager::findNnueFile(halfkaFile);
            if (halfkaPath.empty()) {
                std::cerr << "ERROR: NNUE file not found: nnues/" << halfkaFile << std::endl;
            } else {
                HalfKAv2::Load(halfkaPath);
            }
        }
    });
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

void BoardManager::loadOpenings() {
    OpeningLoader::loadOpenings(OPENING_PATH);
}

void BoardManager::loadBeginnerFEN() {
    loadFEN(newPosFen);
}

void BoardManager::loadFEN(std::string FEN) {
    resetForNewGame();
    board.LoadFEN(FEN);
	updateRobotsState();
}

std::string BoardManager::getRandomOpening() {
    return OpeningLoader::getRandomFen();
}

std::string BoardManager::popRandomFen() {
    return OpeningLoader::popRandomFen();
}

void BoardManager::printBestMove() {
    Move best_move = getBestMoveOnBoard();

    std::cout << "Best move: " + square_to_coordinates[best_move.getFrom()] + square_to_coordinates[best_move.getTo()] << std::endl;
}

bool BoardManager::isMovePromotion(int fromX, int fromY, int toX, int toY) {
    Square fromSq = (Square)(fromY * 8 + fromX);

    PieceType p = board.getPieceAt(fromSq, board.getSideToMove());
    if (p != PAWN) return false;

    Move m = getMove(fromX, fromY, toX, toY, QUEEN);
    if (isRobotToMove() || !m.isValid()) return false;

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

    Color movedColor = board.getSideToMove();
    bool success = board.MakeMove(m);

    if (success && currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
        if (movedColor == WHITE) whiteTimeLeftMs += incrementMs;
        else blackTimeLeftMs += incrementMs;

        saveRemainingTime();
        startTurnClock();
    }

    updateRobotsState();

    return success;
}

void BoardManager::undoMove(int plyToUndo) {

    stopTurnClock();

    for (int i = 0; i < plyToUndo; ++i) {
        if (board.getPly() > 0) {
            board.UndoMove(board.getLastMove());
        }
    }

    if (currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
        int currentPly = board.getPly();
        std::pair<long long, long long> timeRemainingAtPly = timeLeftAtPly[currentPly];
        whiteTimeLeftMs = timeRemainingAtPly.first;
        blackTimeLeftMs = timeRemainingAtPly.second;
        if (!timeLeftAtPly.empty()) {
            timeLeftAtPly.resize(currentPly);
        }
        startTurnClock();
    }

    updateRobotsState();
}

Move BoardManager::MakeRobotMove() {

    if (currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE)
        updateRobotTournementTime();

    Move robot_move;
    LOG_DEBUG((board.getSideToMove() == WHITE ? (whiteRobot->getName() + " (feher) ") : (blackRobot->getName() + " (fekete) ")) <<"gondolkodik...");
	robot_move = board.getSideToMove() == WHITE ? whiteRobot->GetRobotMove() : blackRobot->GetRobotMove();
    if (!robot_move.isValid())
        return Move();

    MakeMove(robot_move);
    LOG_DEBUG("Robot lepese: " + robot_move.toAlgebraic());

    return robot_move;
}

GameResult BoardManager::getGameResult(){
    if (getTimeRemaining(WHITE) <= 0 && currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) return GameResult::BLACK_WON_ON_TIME;
    if (getTimeRemaining(BLACK) <= 0 && currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) return GameResult::WHITE_WON_ON_TIME;
    return board.getGameResult();
}

bool BoardManager::didGameEnd() {
    return getGameResult() != GameResult::GAME_DID_NOT_END;
}

std::string BoardManager::getGameResultString(GameResult gameResult) {
    if (gameResult == GAME_DID_NOT_END) {
        return "The game is still in progress.";
    }

    if (gameResult == DRAW) {
        return "Draw!";
    }

	Color winnerColor = (gameResult & WHITE_WON) != 0 ? WHITE : BLACK;
    bool isWinnerRobot = (winnerColor == WHITE && isRobot(WHITE)) || (winnerColor == BLACK && isRobot(BLACK));

    std::string winnerName;
    if (winnerColor == WHITE) {
        winnerName = isWinnerRobot ? whiteRobot->getName() + " (white)" : "White player";
    } else {
        winnerName = isWinnerRobot ? blackRobot->getName() + " (black)" : "Black player";
    }

    std::string reason;
    switch (gameResult) {
    case WHITE_WON_WITH_CHECKMATE:
    case BLACK_WON_WITH_CHECKMATE:
        reason = "won with checkmate";
        break;
    case WHITE_WON_ON_TIME:
    case BLACK_WON_ON_TIME:
        reason = "won on time";
        break;
    case WHITE_GAVE_UP:
    case BLACK_GAVE_UP:
        return winnerName + " won because the opponent gave up";
    default:
        reason = "won.";
        break;
    }

    return winnerName + " " + reason;
}

void BoardManager::writeGameResult(GameResult gameResult, std::vector<std::string> moveList, std::string beginnerFen) {

    if (!saveSimGames) return;

    if (!whiteRobot || !blackRobot || !isRobot(WHITE) || !isRobot(BLACK)) return;

    if (gameResult == GameResult::GAME_DID_NOT_END) return;

    bool whiteWon = (gameResult & GameResult::WHITE_WON) != 0;

    std::string whiteNameToSaveInFile = whiteRobot->getNameToSaveInFile();
    std::string blackNameToSaveInFile = blackRobot->getNameToSaveInFile();

    std::string whiteSourcePath = whiteRobot->getBotDirectoryPath();
    std::string blackSourcePath = blackRobot->getBotDirectoryPath();

    resultManager.saveGameResult(gameResult, whiteNameToSaveInFile, whiteSourcePath, blackNameToSaveInFile, blackSourcePath, moveList, beginnerFen, simSaveName);
}

void BoardManager::startGameLoop() {

    std::string userInput;

    while (true) {

        if (getGameResult() != GameResult::GAME_DID_NOT_END)
            break;

        if ((board.getSideToMove() == WHITE && isRobot(WHITE)) || (board.getSideToMove() == BLACK && isRobot(BLACK))) {
            MakeRobotMove();
        }

        if (getGameResult() != GameResult::GAME_DID_NOT_END)
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
            if (MakeMove(move)) {
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

void BoardManager::runUCIService() {
    std::string line;
    std::string token;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        iss >> token;

        if (token == "uci") {
            std::cout << "id name " << "Proudly bad bot" << std::endl;
            std::cout << "id author " << "Mate Szekely" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (token == "ucinewgame") {
            resetForNewGame();
        }
        else if (token == "position") {
            std::string type;
            iss >> type;
            if (type == "startpos") {
                loadBeginnerFEN();
            }
            else if (type == "fen") {
                std::string fen;
                std::string part;
                for (int i = 0; i < 6 && (iss >> part); ++i) {
                    fen += part + (i < 5 ? " " : "");
                }
                loadFEN(fen);
            }

            std::string nextToken;
            while (iss >> nextToken) {
                if (nextToken == "moves") continue;
                Move m = UCIParsing::Parse(nextToken, board);
                if (m.isValid()) {
                    board.MakeMove(m);
                }
            }
        }
        else if (token == "go") {
            std::string subToken;
            long long wtime = -1, btime = -1, winc = 0, binc = 0, movetime = -1;

            while (iss >> subToken) {
                if (subToken == "wtime") iss >> wtime;
                else if (subToken == "btime") iss >> btime;
                else if (subToken == "winc") iss >> winc;
                else if (subToken == "binc") iss >> binc;
                else if (subToken == "movetime") iss >> movetime;
            }

            if (movetime != -1) {
                setFixedTimePerMove(movetime);
                setRobotTimeUsageMode(RobotTimeUsageMode::FIXED_TIME);
            }
            else if (wtime != -1 || btime != -1) {
                this->whiteTimeLeftMs = (wtime != -1) ? wtime : 0;
                this->blackTimeLeftMs = (btime != -1) ? btime : 0;
                this->incrementMs = (board.getSideToMove() == WHITE) ? winc : binc;

                setRobotTimeUsageMode(RobotTimeUsageMode::TOURNEMENT_TIME);
                updateRobotTournementTime();
            }

            Move best = (board.getSideToMove() == WHITE)
                ? whiteRobot->GetRobotMove()
                : blackRobot->GetRobotMove();

            std::cout << "bestmove " << UCIParsing::MoveToUCI(best) << std::endl;
        }
        else if (token == "stop") {
            stopRobotCalculation();
        }
        else if (token == "quit") {
            break;
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

    updateClocks(elapsed);

    isClockRunning = false;
}

void BoardManager::restartClock() {
	const TimeSettings& ts = currentSettings.timeSettings;
	setTournementTime(ts.getWhiteTimeMs(), ts.getBlackTimeMs(), ts.getIncrementMs());
}

long long BoardManager::getTimeRemaining(Color player) const {
    if (isClockRunning && board.getCommittedSideToMove() == player && currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - turnStartTime).count();
        return std::max(0LL, (player == WHITE ? whiteTimeLeftMs : blackTimeLeftMs) - elapsed);
    }
    return player == WHITE ? whiteTimeLeftMs : blackTimeLeftMs;
}

void BoardManager::setPlayer(Color c) {
    if (c == WHITE) is_white_player = true;
    else is_black_player = true;

}

void BoardManager::setRobot(Color c){
    if (c == WHITE) is_white_player = false;
    else is_black_player = false;
}

void BoardManager::ClearSearchers() {
    if (whiteRobot != nullptr) whiteRobot->ClearSearcher();
    if (blackRobot != nullptr) blackRobot->ClearSearcher();
}

void BoardManager::setupBotsForNormalGame(const RobotSettings& rs) {

    if (rs.isWhiteRobot) {
        whiteRobot = BotFactory::createBot(SearcherType::IMPROVED_SEARCHER, currentSettings.robotSettings);
        whiteRobot->setDifficulty(rs.whiteRobotDifficulty);
        setRobot(WHITE);
    }

    if (rs.isBlackRobot) {
        blackRobot = BotFactory::createBot(SearcherType::IMPROVED_SEARCHER, currentSettings.robotSettings);
        blackRobot->setDifficulty(rs.blackRobotDifficulty);
        setRobot(BLACK);
    }

    setRobotTimeUsageMode(currentSettings.timeSettings.rtum);
}

static bool isSearcherOfType(ISearcher* searcher, SearcherType type) {
    if (searcher == nullptr) return false;

    switch (type) {
    case SearcherType::OLD_SEARCHER:
        return dynamic_cast<OSearcher*>(searcher) != nullptr;
    case SearcherType::IMPROVED_SEARCHER:
        return dynamic_cast<ImpSearcher*>(searcher) != nullptr;
    }
    return false;
}

static std::string searcherTypeName(SearcherType type) {
    switch (type) {
    case SearcherType::OLD_SEARCHER:
        return "Old searcher";
    case SearcherType::IMPROVED_SEARCHER:
        return "Improved searcher";
    }
    return "Unknown searcher";
}

void BoardManager::prepareImprovedBotVsOldBot() {
    prepareBotVsBot(SearcherType::IMPROVED_SEARCHER, SearcherType::OLD_SEARCHER);
}

void BoardManager::prepareBotVsBot(SearcherType whiteType, SearcherType blackType) {

    board.setVariant(GameVariant::NORMAL);
    loadOpenings();

    if (!isSearcherOfType(whiteRobot.get(), whiteType)) {
        whiteRobot = BotFactory::createBot(whiteType, currentSettings.robotSettings);
        setDifficulty(WHITE, Difficulty::IMPOSSIBLE);
        setRobot(WHITE);
    }

    if (!isSearcherOfType(blackRobot.get(), blackType)) {
        blackRobot = BotFactory::createBot(blackType, currentSettings.robotSettings);
        setDifficulty(BLACK, Difficulty::IMPOSSIBLE);
        setRobot(BLACK);
    }

    setRobotTimeUsageMode(currentSettings.timeSettings.rtum);

    if (saveSimGames) {
        VersionControl::manageBotVersion(whiteRobot->getNameToSaveInFile(), whiteRobot->getBotDirectoryPath());
        VersionControl::manageBotVersion(blackRobot->getNameToSaveInFile(), blackRobot->getBotDirectoryPath());
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

void BoardManager::setSettings(const AllSettings& settings) {

    RobotTimeUsageMode rtum;
    currentSettings = settings;

    board.setVariant(settings.robotSettings.isBotVsBot ? GameVariant::NORMAL : settings.variantSettings.variant);

    switch (settings.timeSettings.gm) {
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

void BoardManager::setTournementTime(long long whiteTimeMs, long long blackTimeMs, long long incrementMs) {
    this->whiteTimeLeftMs = whiteTimeMs;
    this->blackTimeLeftMs = blackTimeMs;
    this->incrementMs = incrementMs;

    if (whiteRobot != nullptr) whiteRobot->setTournamentTime(whiteTimeMs, incrementMs);
    if (blackRobot != nullptr) blackRobot->setTournamentTime(blackTimeMs, incrementMs);

    saveRemainingTime();
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
    if (currentSettings.timeSettings.gm != GameMode::TOURNAMENT_MODE) return;

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

void BoardManager::updateRobotsState() {
    if (whiteRobot != nullptr) whiteRobot->setState(board);
    if (blackRobot != nullptr) blackRobot->setState(board);
};

void BoardManager::startBotSimulation(int totalRounds, int threadId) {
    const int gamesPerRound = 2;
    isInBotSimulation = true;

    if (whiteRobot == nullptr || blackRobot == nullptr) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        prepareBotVsBot(simBotTypes[0], simBotTypes[1]);
    }

    int roundsFinished = 0;

    while (isInBotSimulation && roundsFinished < totalRounds) {
        std::string openingFen = popRandomFen();
        std::string roundSummary[2];
		GameResult roundResults[2];

        for (int gameInRound = 0; gameInRound < gamesPerRound; ++gameInRound) {
            loadFEN(openingFen);
            restartClock();

            if (currentSettings.timeSettings.gm == GameMode::TOURNAMENT_MODE) {
                startTurnClock();
            }

            while (!didGameEnd() && isInBotSimulation) {
                MakeRobotMove();
            }

            stopTurnClock();

            GameResult result = getGameResult();

            roundSummary[gameInRound] = getGameResultString(result);
            roundSummary[gameInRound] += ", " + std::to_string(board.getMoveHistory().size()) + " moves been made";
			roundResults[gameInRound] = result;

            if (result == GameResult::DRAW) simDraws++;
            else if (result & GameResult::WHITE_WON) simBotWins[gameInRound]++;
            else if (result & GameResult::BLACK_WON) simBotWins[1 - gameInRound]++;

            {
                std::lock_guard<std::mutex> lock(consoleMutex);
				if (gameInRound == 0) {
                    std::cout << std::endl;
					if (threadId != -1) {
                        std::cout << "[BoardManager] - [Thread " << threadId << "] ";
                    }
                    std::cout << "Game 1 result: ";
                    std::cout << "Fen: " << openingFen << std::endl;
                    std::cout << roundSummary[gameInRound] << std::endl;
                    writeGameResult(result, board.getMoveHistroyInSAN(), board.getBeginnerFen());
                    SwapRobots();
                }
            }
        }

        roundsFinished++;

        if (threadId != -1) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << std::endl;
            std::cout << "[BoardManager] - [Thread " << threadId << "] ROUND ";
            if (totalRounds != 1) std::cout << roundsFinished << "/" << totalRounds << " ";
            std::cout << "SUMMARY:" << std::endl;
            std::cout << "  FEN: " << openingFen << std::endl;
            std::cout << "  Game 1: " << roundSummary[0] << std::endl;
            std::cout << "  Game 2: " << roundSummary[1] << std::endl;
        }
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            writeGameResult(roundResults[1], board.getMoveHistroyInSAN(), board.getBeginnerFen());
        }
        SwapRobots();
    }
    isInBotSimulation = false;
}

void BoardManager::startMultiThreadedSimulation(int totalGames, int numThreads,
                                                SearcherType firstBot, SearcherType secondBot,
                                                bool saveGames, const std::string& saveName) {

	int totalRounds = totalGames / 2;

    std::atomic<int> roundsRemaining(totalRounds);
    std::atomic<int> totalWins[2] = { 0, 0 };
    std::atomic<int> totalDraws(0);
    std::vector<std::thread> threads;

    const std::string firstName = searcherTypeName(firstBot);
    const std::string secondName = searcherTypeName(secondBot);

    std::cout << "[BoardManager] Simulation started on " << numThreads << " threads: "
              << firstName << " vs " << secondName
              << (saveGames ? "" : " (games are not saved)") << std::endl;

    auto startTime = std::chrono::steady_clock::now();

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {

            BoardManager localManager;

			localManager.setSettings(this->currentSettings);
            localManager.simBotTypes[0] = firstBot;
            localManager.simBotTypes[1] = secondBot;
            localManager.saveSimGames = saveGames;
            localManager.simSaveName = saveName;

            while (roundsRemaining.fetch_sub(1) > 0) {
                localManager.startBotSimulation(1, i);
            }

            totalWins[0] += localManager.simBotWins[0];
            totalWins[1] += localManager.simBotWins[1];
            totalDraws += localManager.simDraws;
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    const int played = totalWins[0] + totalWins[1] + totalDraws;
    const double firstScore = played > 0 ? 100.0 * (totalWins[0] + 0.5 * totalDraws) / played : 0.0;

    std::cout << "\n[BoardManager] Simulation done: " << totalGames << " games." << std::endl;
    std::cout << "Simulation period of time: " << duration << " second." << std::endl;
    std::cout << "  " << firstName << " wins: " << totalWins[0] << std::endl;
    std::cout << "  " << secondName << " wins: " << totalWins[1] << std::endl;
    std::cout << "  Draws: " << totalDraws << std::endl;
    std::cout << "  " << firstName << " score: " << firstScore << "%" << std::endl;
}

void BoardManager::stopBotSimulation() {
    isInBotSimulation = false;
    stopRobotCalculation();
}
