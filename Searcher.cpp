#include "Searcher.h"
#include "Evaluation.h"
#include <chrono>
#include <iostream>

const int DELTA_MARGIN = 950;
const int lmp_table[] = { 0, 3, 6, 10, 16, 24 };
const int futility_margin[] = { 0, 150, 300, 500, 900, 1500 };

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline int Searcher::ScoreToTT(int score, int ply) {
    if (IsMateScore(score)) {
        return score > 0 ? score + ply : score - ply;
    }
    return score;
}

inline int Searcher::ScoreFromTT(int score, int ply) {
    if (IsMateScore(score)) {
        return score > 0 ? score - ply : score + ply;
    }
    return score;
}

int Searcher::see(Move m) {
    Square from = m.getFrom();
    Square to = m.getTo();
    PieceType attacker = m.getPieceType();
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

        bool isPromo = (m.getFlags() & PROMOTION_FLAG);

		if (!isPromo && see(m) < 0) {
            continue;
        }

        Color enemy = (Color)(board.getSideToMove() ^ 1);
        PieceType victim = board.getPieceAt(m.getTo(), enemy);

        if (!isPromo && standPat + Evaluation::GetPieceValue(victim) + 200 < alpha) {
            continue;
        }

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

    if (tt.Probe(hash, depth, alpha, beta, ttScore, ttMove)) {
        return ScoreFromTT(ttScore, ply);
    }

    bool inCheckBeforeMove = board.isSquareAttacked(
        board.getKingSquare(board.getSideToMove()),
        (Color)(board.getSideToMove() ^ 1));


    if (inCheckBeforeMove)
        depth++;

    if (depth <= 0)
        return quiescence(alpha, beta);

    bool futilityPrune = false;

    if (depth <= 4 && !inCheckBeforeMove && ply > 0 && abs(alpha) < 90000 && abs(beta) < 90000) {

        int staticEval = Evaluation::EvaluatePos(board);
        int evalMargin = 120 * depth;
        if (staticEval - evalMargin >= beta) {
            return staticEval;
        }
        if (staticEval + futility_margin[depth] <= alpha) {
            futilityPrune = true;
        }
    }

    if (allowNull && depth >= 3 && !inCheckBeforeMove && ply > 0 && beta < MATE_SCORE) {
        if (board.HasNonPawnMaterial(board.getSideToMove())) {

            board.MakeNullMove();
            int R = 3;
            if (depth > 6) R = 4;

            int score = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1, Move(), false, false);

            board.UndoNullMove();

            if (stop) return alpha;

            if (score >= beta) {
                return beta;
            }
        }
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    int important_move = 0;

    if (ply < MAX_KILLER_HISTORY) {

        Move currentKillers[2] = { Move(), Move() };
        currentKillers[0] = killerMoves[ply][0];
        currentKillers[1] = killerMoves[ply][1];

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

        Color us = board.getSideToMove();
        Color enemy = (Color)(us ^ 1);

        bool isCapture = m.getFlags() & CAPTURE_FLAG;
        bool quiet =
            !(isCapture) &&
            !(m.getFlags() & PROMOTION_FLAG);

        if ((futilityPrune && quiet && movesSearched > 0) || (!inCheckBeforeMove && depth <= 5 && movesSearched >= lmp_table[depth] && quiet)) {

            bool isKiller = false;
            if (ply < MAX_KILLER_HISTORY) {
                if (killerMoves[ply][0].isValid() && m.getMoveData() == killerMoves[ply][0].getMoveData()) isKiller = true;
                else if (killerMoves[ply][1].isValid() && m.getMoveData() == killerMoves[ply][1].getMoveData()) isKiller = true;
            }

            if (!isKiller) {
                board.UndoMove(m, true);
                continue;
            }
        }

        uint64_t hash_after_move = board.getHash();
        bool irreversible = (m.getPieceType() == PAWN) || (isCapture);
        repetitionTable.Push(hash_after_move, irreversible);

        int score;
        bool gives_check = false;
        int reduction = 0;
        if (depth >= 3 && quiet) {
            gives_check = board.isSquareAttacked(board.getKingSquare(enemy), us);
            if (!gives_check) {
                reduction = LMR::GetReduction(depth, movesSearched);
            }
        }

        if (movesSearched == 1) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1, m, isCapture, true);
        }
        else {
            int r = (movesSearched <= important_move) ? 0 : reduction;

            score = -negamax(depth - 1 - r, -alpha - 1, -alpha, ply + 1, m, isCapture, true);

            if (score > alpha && r > 0) {
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
            tt.Store(hash, ScoreToTT(score, ply), depth, TT_BETA, m);
            return score;
        }

        if (score > alpha) {
            alpha = score;
            bestMove = m;
        }
    }

    if (movesSearched == 0) {
        int score = inCheckBeforeMove ? -MATE_SCORE + ply : 0;
        return score;
    }

    TTFlag flag = (alpha <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    tt.Store(hash, ScoreToTT(alpha, ply), depth, flag, bestMove);

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
    nodes = 0;

	repetitionTable.Init(board);
    AgeHistory();
    ClearKillers();
    tt.NewWrite();

    int rawScore;
    Move tmpMove;

    Move bestMove;
    int lastScore = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        int window = 50;
        int alpha = lastScore - window;
        int beta = lastScore + window;
        int score;

        while (true) {
            score = negamax(depth, alpha, beta, 0);
            if (stop) break;
            if (score <= alpha) {
                alpha -= window;
            }
            else if (score >= beta) {
                beta += window;
            }
            else {
                break;
            }
            window *= 2;
        }
        if (tt.Probe(board.getHash(), depth, -MATE_SCORE, MATE_SCORE, rawScore, tmpMove)) {
            score = ScoreFromTT(rawScore, 0);
        }

        if (tmpMove.isValid()) {
            bestMove = tmpMove;
        }
        
        if (stop) break;

        lastScore = score;

        if (board.isDebugMode) {
            std::cout << "info depth " << depth << " score ";
            if (abs(score) > 90000)
                std::cout << "mate " << ((score > 0) ? (100001 - score) / 2 : -(100001 + score) / 2);
            else
                std::cout << "cp " << (board.getSideToMove() == WHITE ? score : -score);

            std::cout << " time " << (now_ms() - startTime)
                << " nodes " << nodes
                << " pv " << bestMove.toAlgebraic()
                << std::endl;

            if (abs(score) > 90000)
                break;
        }
    }

    if (board.isDebugMode) {
        std::cout << "Bestmove: " << bestMove.toAlgebraic()
            << " score cp "
            << (board.getSideToMove() == WHITE ? lastScore : -lastScore)
            << std::endl;

        std::vector<Move> baseLine = GetPVLine(50);
        PrintPvLine(50);
	}

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

        if (tt.Probe(currentHash, 0, -MATE_SCORE, MATE_SCORE, ttScore, ttMove)) {
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

        if (tt.Probe(currentHash, 0, -MATE_SCORE, MATE_SCORE, ttScore, ttMove)) {
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

Move Searcher::GetBestMove() {
    return IterativeDeepening();
}
