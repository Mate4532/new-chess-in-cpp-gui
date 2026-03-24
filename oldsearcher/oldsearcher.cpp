#include "oldsearcher.h"
#include "OldEvaluation.h"
#include <chrono>
#include <iostream>

using namespace OldEvaluation;
using namespace OldSearcher;
using namespace OldMoveOrdering;
using namespace OldTT;
using namespace OldLMR;

const int DELTA_MARGIN = 950;
const int lmp_table[] = { 0, 3, 6, 10, 16, 24 };
const int futility_margin[] = { 0, 150, 300, 500, 900, 1500 };

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Searcher::stopSearch() {
    stop = true;
    isStoppedManually = true;
}

int Searcher::see(Move m) {
    Square from = m.getFrom();
    Square to = m.getTo();
    PieceType victim = board.getPieceAt(to, (Color)(board.getSideToMove() ^ 1));
    if (m.getFlags() == EN_PASSANT) victim = PAWN;
    int gain[32];
    int d = 0;
    uint64_t occupied = board.getAllOccupancy();
    uint64_t attackers = board.getAttacksTo(to, occupied);

    gain[d] = Evaluation::GetPieceValue(victim);
    Color side = board.getSideToMove();

    occupied ^= (1ULL << from);
    attackers |= board.getNewXRayAttacks(from, occupied);

    while (true) {
        side = (Color)(side ^ 1);
        attackers &= occupied;

        PieceType nextAttacker;
        Square nextSq = board.getSmallestAttacker(attackers, side, nextAttacker);

        if (nextSq == SQUARE_NONE) break;

        d++;
        gain[d] = Evaluation::GetPieceValue(nextAttacker) - gain[d - 1];

        if (std::max(-gain[d - 1], gain[d]) < 0) break;

        occupied ^= (1ULL << nextSq);
        attackers |= board.getNewXRayAttacks(nextSq, occupied);
    }

    while (--d > 0) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }

    return gain[0];
}

int Searcher::quiescence(int alpha, int beta) {
    nodes++;

    if ((nodes & 2047) == 0 && now_ms() - startTime >= robot_thinking_time_ms)
        stop = true;
    if (stop) return alpha;

    int standPat = Evaluation::EvaluatePos(board);

    if (standPat >= beta) return standPat;
    if (standPat < alpha - DELTA_MARGIN) {
        return alpha;
    }

    if (standPat > alpha) alpha = standPat;

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, true);

    Move dummyKillers[2] = { Move(), Move() };
    MoveOrdering::SortMoves(board, moves, Move(), historyMoves, dummyKillers);

    for (const Move& m : moves) {

        if (!board.MakeMove(m, true)) continue;

        int score = -quiescence(-beta, -alpha);
        board.UndoMove(m, true);

        if (stop) return alpha;

        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply, Move prev_move, bool prev_was_capture, bool allowNull) {

    nodes++;
    if ((nodes & 2047) == 0 && now_ms() - startTime >= robot_thinking_time_ms)
        stop = true;
    if (stop)
        return alpha;

    int originalAlpha = alpha;
    uint64_t hash = board.getHash();
    if (ply > 0) {
        if (board.getHalfMoveClock() >= 100 || repetitionTable.Contains(hash) || board.IsInsufficientMaterial()) {
            return 0;
        }

        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply);
        if (alpha >= beta) return alpha;
    }

    int ttScore;
    Move ttMove;

    if (tt.Probe(hash, ply, depth, alpha, beta, ttScore, ttMove)) {
        return ttScore;
    }

    bool inCheck = board.isSquareAttacked(
        board.getKingSquare(board.getSideToMove()),
        (Color)(board.getSideToMove() ^ 1));

    if (inCheck)
        depth++;

    if (depth <= 0)
        return quiescence(alpha, beta);

    int staticEval = 0;
    if (!inCheck) {
        staticEval = Evaluation::EvaluatePos(board);
    }

    if (depth <= 4 && !inCheck && ply > 0 && abs(beta) < MATE_SCORE_BOUND) {
        int evalMargin = 120 * depth;
        if (staticEval - evalMargin >= beta) {
            return staticEval;
        }
    }

    if (allowNull && depth >= 3 && !inCheck && ply > 0 && beta < MATE_SCORE) {
        if (board.HasNonPawnMaterial(board.getSideToMove())) {
            int R = 3 + (depth / 6);
            board.MakeNullMove();
            int score = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1, Move(), false, false);
            board.UndoNullMove();
            if (stop) return alpha;
            if (score >= beta) return beta;
        }
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    int important_move = 0;

    if (ply < MAX_KILLER_HISTORY) {

        Move currentKillers[2] = { killerMoves[ply][0], killerMoves[ply][1] };

        important_move = MoveOrdering::SortMoves(
            board,
            moves,
            ttMove,
            historyMoves,
            currentKillers
            );
    }

    int important_move_min = 3;
    important_move = std::max(important_move, important_move_min);

    Move bestMove;
    int movesSearched = 0;

    for (const Move& m : moves) {
        if (!board.MakeMove(m, true)) {
            continue;
        }
        movesSearched++;

        if (!bestMove.isValid())
            bestMove = m;

        Color enemy = (Color)(board.getSideToMove());
        bool givesCheck = board.isSquareAttacked(board.getKingSquare(enemy), (Color)(enemy ^ 1));

        bool isCapture = m.getFlags() & CAPTURE_FLAG;
        bool isPromo = m.getFlags() & PROMOTION_FLAG;
        bool quiet = !isCapture && !isPromo;

        if (depth <= 4 && !inCheck && !givesCheck && quiet && abs(alpha) < MATE_SCORE_BOUND && abs(beta) < MATE_SCORE_BOUND) {
            int futilityMargin = 150 * depth;
            if (staticEval + futilityMargin <= alpha) {
                bool isKiller = (ply < MAX_KILLER_HISTORY) &&
                                (m == killerMoves[ply][0] || m == killerMoves[ply][1]);
                if (!isKiller) {
                    board.UndoMove(m, true);
                    continue;
                }
            }
        }

        if (!inCheck && !givesCheck && quiet && depth <= 5) {
            if (movesSearched >= lmp_table[depth]) {
                bool isKiller = (ply < MAX_KILLER_HISTORY) &&
                                (m == killerMoves[ply][0] || m == killerMoves[ply][1]);
                if (!isKiller) {
                    board.UndoMove(m, true);
                    continue;
                }
            }
        }

        uint64_t hash_after_move = board.getHash();
        bool irreversible = (m.getPieceType() == PAWN) || (isCapture);
        repetitionTable.Push(hash_after_move, irreversible);

        int score;

        int reduction = 0;
        if (depth >= 3 && movesSearched > important_move && quiet && !givesCheck) {
            reduction = LMR::GetReduction(depth, movesSearched);

            if (!inCheck && (staticEval + 100 <= alpha)) {
                reduction++;
            }
        }

        if (movesSearched == 1) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1, m, isCapture, true);
        }
        else {
            score = -negamax(depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, m, isCapture, true);

            if (score > alpha && reduction > 0) {
                score = -negamax(depth - 1, -alpha - 1, -alpha, ply + 1, m, isCapture, true);
            }

            if (score > alpha && score < beta) {
                score = -negamax(depth - 1, -beta, -alpha, ply + 1, m, isCapture, true);
            }
        }

        repetitionTable.TryPop();
        board.UndoMove(m, true);
        if (stop)
            return alpha;

        if (score >= beta) {
            if (quiet) {
                if (ply < MAX_KILLER_HISTORY && m.isValid() && m.getMoveData() != killerMoves[ply][0].getMoveData()) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = m;
                }
                historyMoves[board.getSideToMove()][m.getFrom()][m.getTo()] += depth * depth;
            }
            tt.Store(hash, score, ply, depth, TT_BETA, m);
            return score;
        }

        if (score > alpha) {
            alpha = score;
            bestMove = m;
        }
    }

    if (movesSearched == 0) {
        int score = inCheck ? -MATE_SCORE + ply: 0;
        return score;
    }

    TTFlag flag = (alpha <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    tt.Store(hash, alpha, ply, depth, flag, bestMove);

    return alpha;
}

void Searcher::ClearHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < MAX_KILLER_HISTORY; f++)
            for (int t = 0; t < MAX_KILLER_HISTORY; t++)
                historyMoves[c][f][t] = 0;
}

void Searcher::AgeHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < MAX_KILLER_HISTORY; f++)
            for (int t = 0; t < MAX_KILLER_HISTORY; t++)
                historyMoves[c][f][t] >>= 1;
}

void Searcher::ClearKillers() {
    for (int i = 0; i < MAX_KILLER_HISTORY; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }
}

Move Searcher::IterativeDeepening() {
    startTime = now_ms();
    stop = false;
    isStoppedManually = false;
    isSearching = true;
    nodes = 0;

    repetitionTable.Init(board);
    repetitionTable.Push(board.getHash(), false);
    AgeHistory();
    ClearKillers();
    tt.NewWrite();

    int rawScore;
    Move tmpMove;

    Move bestMove;
    int lastScore = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        int score = 0;
        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;
        int delta = 50;

        if (depth >= 5) {
            alpha = std::max(-MATE_SCORE, score - delta);
            beta = std::min(MATE_SCORE, score + delta);
        }

        score = negamax(depth, alpha, beta, 0);

        if (score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = -MATE_SCORE;
            score = negamax(depth, alpha, beta, 0);
        }

        else if (score >= beta) {
            alpha = (alpha + beta) / 2;
            beta = MATE_SCORE;
            score = negamax(depth, alpha, beta, 0);
        }

        tt.Probe(board.getHash(), 0, depth, -MATE_SCORE, MATE_SCORE, rawScore, tmpMove);

        if (tmpMove.isValid()) {
            bestMove = tmpMove;
        }

        if (stop) break;

        lastScore = score;

        if (board.isDebugMode) {
            std::cout << "info depth " << depth << " score ";
            if (abs(score) > MATE_SCORE_BOUND)
                std::cout << "mate " << ((score > 0) ? (MATE_SCORE + 1 - score) / 2 : -(MATE_SCORE + 1 + score) / 2);
            else
                std::cout << "cp " << (board.getSideToMove() == WHITE ? score : -score);

            std::cout << " time " << (now_ms() - startTime)
                      << " nodes " << nodes
                      << " pv " << bestMove.toAlgebraic()
                      << std::endl;
        }

        if (IsMateScore(score))
            break;
    }

    if (board.isDebugMode) {
        std::cout << "Bestmove: " << bestMove.toAlgebraic()
        << " score cp "
        << (board.getSideToMove() == WHITE ? lastScore : -lastScore)
        << std::endl;

        std::vector<Move> baseLine = GetPVLine(50);
    }

    isSearching = false;

    if (isStoppedManually)
        return Move();

    return bestMove;
}

void Searcher::PrintPvLine(int depth) {

    std::vector<Move> pvLine = GetPVLine(depth);

    for (int i = 0; i < pvLine.size(); ++i) {
        const Move& m = pvLine[i];
        std::cout << m.toAlgebraic() << (i != pvLine.size() - 1 ? " -> " : "");
    }

    std::cout << std::endl;
}

std::vector<Move> Searcher::GetPVLine(int depth) {
    std::vector<Move> pvLine;
    uint64_t currentHash = board.getHash();

    for (int i = 0; i < depth; i++) {
        int ttScore;
        Move ttMove;
        TTFlag flag;

        if (tt.Probe(currentHash, 0, 0, -MATE_SCORE, MATE_SCORE, ttScore, ttMove)) {
            if (ttMove.isValid()) {

                if (board.MakeMove(ttMove, true)) {
                    pvLine.push_back(ttMove);
                    currentHash = board.getHash();
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }

    for (int i = (int)pvLine.size() - 1; i >= 0; i--) {
        board.UndoMove(pvLine[i], true);
    }

    return pvLine;
}

void Searcher::PrintWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth) {

    std::vector<Move> pvLine = GetWhatIfPV(baseLine, alternativeMove, depth);

    for (int i = 0; i < pvLine.size(); ++i) {
        const Move& m = pvLine[i];
        std::cout << m.toAlgebraic() << (i != pvLine.size() - 1 ? " -> " : "");
    }

    std::cout << std::endl;
}

std::vector<Move> Searcher::GetWhatIfPV(const std::vector<Move>& baseLine, Move alternativeMove, int depth) {
    std::vector<Move> fullHistory;
    std::vector<Move> resultPV;

    for (const Move& m : baseLine) {
        if (board.MakeMove(m, true)) {
            fullHistory.push_back(m);
        }
        else {
            for (int i = (int)fullHistory.size() - 1; i >= 0; i--) board.UndoMove(fullHistory[i], true);
            return {};
        }
    }

    if (!board.MakeMove(alternativeMove, true)) {
        for (int i = (int)fullHistory.size() - 1; i >= 0; i--) board.UndoMove(fullHistory[i], true);
        return {};
    }
    fullHistory.push_back(alternativeMove);
    resultPV.push_back(alternativeMove);
    board.PrintBoard();

    uint64_t currentHash = board.getHash();

    for (int i = 0; i < depth; i++) {
        int ttScore;
        Move ttMove;

        if (tt.Probe(currentHash, 0, 0, -MATE_SCORE, MATE_SCORE, ttScore, ttMove)) {
            if (board.MakeMove(ttMove, true)) {
                resultPV.push_back(ttMove);
                fullHistory.push_back(ttMove);
                currentHash = board.getHash();
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }

    for (int i = (int)fullHistory.size() - 1; i >= 0; i--) {
        board.UndoMove(fullHistory[i], true);
    }

    return resultPV;
}

Move Searcher::GetRobotMove() {
    return IterativeDeepening();
}

void Searcher::ClearSearcher() {
    ClearKillers();
    ClearHistory();
    tt.Clear();
    repetitionTable.Clear();
}

void Searcher::setDifficulty(Difficulty diff) {
    switch (diff) {
    case Difficulty::EASY:
        max_depth = 5;
        break;

    case Difficulty::MEDIUM:
        max_depth = 7;
        break;

    case Difficulty::HARD:
        max_depth = 9;
        break;

    case Difficulty::IMPOSSIBLE:
        max_depth = MAXIMUM_DEPTH;
        break;

    default:
        break;
    }
}

SearcherType Searcher::getType() const {
    return SearcherType::OLD_SEARCHER;
}

std::string Searcher::getName() const {
    return "Régi robot";
}

Difficulty Searcher::getDifficulty() const {
    return currentDiff;
}

std::string Searcher::getDifficultyString() const {
    switch (currentDiff) {
    case Difficulty::EASY:
        return "Kezdő";

    case Difficulty::MEDIUM:
        return "Haladó";

    case Difficulty::HARD:
        return "Nehéz";

    case Difficulty::IMPOSSIBLE:
        return "Mester";

    default:
        break;
    }
}
