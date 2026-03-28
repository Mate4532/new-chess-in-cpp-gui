#include "Searcher.h"
#include "Evaluation.h"
#include "WorseEvaluation.h"
#include <chrono>
#include <iostream>
#include <random>

using namespace ImprovedSearcher;
using namespace ImprovedEvaluation;
using namespace ImprovedMoveOrdering;
using namespace ImprovedLMR;
using namespace ImprovedTT;

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

int Searcher::quiescence(int alpha, int beta, int ply) {
    nodes++;

    if ((nodes & 2047) == 0 && now_ms() - startTime >= robot_thinking_time_ms)
        stop = true;
    if (stop) return alpha;

    int standPat = currentSettings.worseEvaluationEnabled ? WorseEvaluation::Evaluation::EvaluatePos(board) : ImprovedEvaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, true);

    Move dummyKillers[2] = { Move(), Move() };
    MoveOrdering::SortMoves(board, moves, Move(), historyMoves, dummyKillers);

    for (const Move& m : moves) {

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        PieceType mPieceType = m.getPieceType();
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        bool isEp = flags == EN_PASSANT;

        if (isEp) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

            nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(PAWN, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0]   = mTo;

            int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
            nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(PAWN, enemy);
            nnue_state[ply + 1].dirtyPiece.from[1] = epSquare;
            nnue_state[ply + 1].dirtyPiece.to[1]   = 64;
        }
        else if (isCapture) {
            if (isPromo) {
                PieceType promoPiece = Board::GetPromotionPiece(flags);
                PieceType capPiece = board.getPieceAt(mTo, enemy);

                nnue_state[ply + 1].dirtyPiece.dirtyNum = 3;

                nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(PAWN, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0]   = 64;

                nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1]   = 64;

                nnue_state[ply + 1].dirtyPiece.pc[2]   = Evaluation::GetNnuePieceNum(promoPiece, player);
                nnue_state[ply + 1].dirtyPiece.from[2] = 64;
                nnue_state[ply + 1].dirtyPiece.to[2]   = mTo;
            }

            else {
                PieceType capPiece = board.getPieceAt(mTo, enemy);
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

                nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(mPieceType, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0]   = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1]   = 64;
            }
        }

        if (!board.MakeMove(m, true)) continue;

        int score = -quiescence(-beta, -alpha, ply + 1);
        board.UndoMove(m, true);

        if (stop) return alpha;
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply, Move prev_move, bool prev_was_capture, bool allowNull) {

    nodes++;
    bool isPvNode = (beta - alpha > 1);

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

    bool foundInTT = tt.Probe(hash, ply, depth, alpha, beta, ttScore, ttMove);

    if (foundInTT && ply > 0) {
        return ttScore;
    }

    bool inCheck = board.isSquareAttacked(
        board.getKingSquare(board.getSideToMove()),
        (Color)(board.getSideToMove() ^ 1));

    int staticEval = currentSettings.worseEvaluationEnabled ? WorseEvaluation::Evaluation::EvaluatePos(board) : ImprovedEvaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

    if (depth <= 4 && !inCheck && ply > 0 && abs(beta) < MATE_SCORE_BOUND) {
        int evalMargin = 120 * depth;
        if (staticEval - evalMargin >= beta) {
            return staticEval;
        }
    }

    if (allowNull && depth >= 3 && !inCheck && ply > 0 && abs(beta) < MATE_SCORE_BOUND) {
        if (staticEval >= beta - 50 && board.HasNonPawnMaterial(board.getSideToMove())) {
            int R = 3 + (depth / 6);
            nnue_state[ply + 1] = nnue_state[ply];
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
            board.MakeNullMove();
            int score = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1, Move(), false, false);
            board.UndoNullMove();
            if (stop) return alpha;
            if (score >= beta) return beta;
        }
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    int legalEvasions = 0;
    if (inCheck) {
        if (isPvNode || depth <= 4){
            depth++;
        }
    }

    if (depth <= 0)
        return quiescence(alpha, beta, ply);

    Move currentKillers[2] = { Move(), Move() };

    if (ply < MAX_KILLER_HISTORY) {
        currentKillers[0] = killerMoves[ply][0];
        currentKillers[1] = killerMoves[ply][1];
    }

    MoveOrdering::SortMoves(
        board,
        moves,
        ttMove,
        historyMoves,
        currentKillers
        );

    Move bestMove;
    int movesSearched = 0;

    for (const Move& m : moves) {

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool quiet = !isCapture && !isPromo;
        bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE);
        bool isEp = flags == EN_PASSANT;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        PieceType mPieceType = m.getPieceType();
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        if (isCapture) {

            if (isEp){
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

                nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(PAWN, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0]   = mTo;

                int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
                nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(PAWN, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = epSquare;
                nnue_state[ply + 1].dirtyPiece.to[1]   = 64;
            }

            else {
                nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(mPieceType, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0]   = isPromo ? 64 : mTo;

                PieceType capPiece = board.getPieceAt(mTo, enemy);
                nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1]   = 64;

                if (isPromo) {
                    PieceType promoPiece = Board::GetPromotionPiece(flags);
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 3;

                    nnue_state[ply + 1].dirtyPiece.pc[2]   = Evaluation::GetNnuePieceNum(promoPiece, player);
                    nnue_state[ply + 1].dirtyPiece.from[2] = 64;
                    nnue_state[ply + 1].dirtyPiece.to[2]   = mTo;
                }

                else {
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                }
            }
        }

        else if (isPromo){
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

            nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(PAWN, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0]   = 64;

            PieceType promoPiece = Board::GetPromotionPiece(flags);
            nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(promoPiece, player);
            nnue_state[ply + 1].dirtyPiece.from[1] = 64;
            nnue_state[ply + 1].dirtyPiece.to[1]   = mTo;
        }

        else if (isCastle) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
            if (flags & KINGSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0]   = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1]   = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_TO;

            }

            else if (flags & QUEENSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0]   = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1]   = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1]   = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_TO;
            }
        }

        else {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 1;

            nnue_state[ply + 1].dirtyPiece.pc[0]   = Evaluation::GetNnuePieceNum(mPieceType, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0]   = mTo;
        }

        if (!board.MakeMove(m, true)) {
            continue;
        }
        movesSearched++;

        if (!bestMove.isValid())
            bestMove = m;

        bool givesCheck = board.isSquareAttacked(board.getKingSquare(enemy), player);

        bool isAdvancedPawnPush = false;
        if (m.getPieceType() == PAWN) {
            int rank = m.getTo() >> 3;
            Color us = (Color)(board.getSideToMove() ^ 1);

            if (us == WHITE) {
                if (rank >= 5) isAdvancedPawnPush = true;
            } else {
                if (rank <= 2) isAdvancedPawnPush = true;
            }
        }

        if (!isPvNode && movesSearched > 1 && !isAdvancedPawnPush && depth <= 4 && !inCheck && !givesCheck && quiet && abs(alpha) < MATE_SCORE_BOUND && abs(beta) < MATE_SCORE_BOUND) {

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

        if (!isPvNode && movesSearched > 1 && !inCheck && !givesCheck && quiet && !isAdvancedPawnPush && depth <= 5 && abs(alpha) < MATE_SCORE_BOUND && abs(beta) < MATE_SCORE_BOUND) {
            int lmp_threshold = 3 + (2 * depth * depth);

            if (movesSearched >= lmp_threshold) {
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
        if (depth >= 3 && !inCheck && quiet && !givesCheck && !isAdvancedPawnPush) {
            reduction = LMR::GetReduction(depth, movesSearched);

            if (isPvNode) {
                reduction -= 1;
            }

            reduction = std::max(0, reduction);
        }

        if (movesSearched == 1) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1, m, isCapture, true);
        }
        else {
            score = -negamax(depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, m, isCapture, true);

            if (score > alpha) {
                if (reduction > 0 || score < beta) {
                    score = -negamax(depth - 1, -beta, -alpha, ply + 1, m, isCapture, true);
                }
            }
        }

        repetitionTable.TryPop();
        board.UndoMove(m, true);
        if (stop)
            return alpha;

        if (score >= beta) {
            if (quiet) {
                if (ply < MAX_KILLER_HISTORY && m.isValid() && m != killerMoves[ply][0]) {
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
        int score = inCheck ? -MATE_SCORE + ply : 0;
        return score;
    }

    TTFlag flag = (alpha <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    tt.Store(hash, alpha, ply, depth, flag, bestMove);

    return alpha;
}

void Searcher::PrepareSearcher() {
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

    nnue_state[0].dirtyPiece.dirtyNum = 0;
    nnue_state[0].accumulator.computedAccumulation = 0;
}

void Searcher::ClearHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < SQUARE_COUNT; f++)
            for (int t = 0; t < SQUARE_COUNT; t++)
                historyMoves[c][f][t] = 0;
}

void Searcher::AgeHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < SQUARE_COUNT; f++)
            for (int t = 0; t < SQUARE_COUNT; t++)
                historyMoves[c][f][t] >>= 1;
}

void Searcher::ClearKillers() {
    for (int i = 0; i < MAX_KILLER_HISTORY; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }
}

Move Searcher::IterativeDeepening() {
    PrepareSearcher();

    int rawScore;
    Move tmpMove;

    Move bestMove;
    int lastScore = 0;

    for (int depth = 1; depth <= currentSettings.maxDepth; depth++) {
        int score = lastScore;
        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;
        int delta = 50;

        if (depth >= 5) {
            if (std::abs(lastScore) >= MATE_SCORE_BOUND) {
                alpha = -MATE_SCORE;
                beta = MATE_SCORE;
            } else {
                alpha = std::max(-MATE_SCORE, score - delta);
                beta = std::min(MATE_SCORE, score + delta);
            }
        }

        score = negamax(depth, alpha, beta, 0);

        if (score <= alpha || score >= beta) {
            alpha = -MATE_SCORE;
            beta = MATE_SCORE;
            score = negamax(depth, alpha, beta, 0);
        }

        if (stop) break;

        tt.Probe(board.getHash(), 0, depth, -MATE_SCORE, MATE_SCORE, rawScore, tmpMove);

        if (tmpMove.isValid()) {
            bestMove = tmpMove;
        }

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
        << " score cp " << (board.getSideToMove() == WHITE ? lastScore : -lastScore) << std::endl;
    }

    isSearching = false;

    if (isStoppedManually) return Move();

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

Move Searcher::GetBestAmongTopMoves(const SearcherSettings& settings) {
    static std::mt19937 gen(now_ms());
    std::uniform_int_distribution<> dis(1, 100);

    PrepareSearcher();

    if (settings.minNormalMovesAfterBlunder > movesWithoutBlunderOnPropuse || dis(gen) > settings.chanceToActivatePossBlunder) {
        movesWithoutBlunderOnPropuse++;
        return IterativeDeepening();
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    Move dummyKillers[2] = { Move(), Move() };
    MoveOrdering::SortMoves(board, moves, Move(), historyMoves, dummyKillers);

    struct ScoredMove { Move m; int score; };
    std::vector<ScoredMove> lastCompletedScores;

    for (int i = 0; i < moves.size(); ++i) {
        if (board.MakeMove(moves[i], true)) {
            lastCompletedScores.push_back({moves[i], 0});
            board.UndoMove(moves[i], true);
        }
    }

    if (lastCompletedScores.empty()) return IterativeDeepening();

    int targetDepth = std::max(settings.maxDepth - 3, 3);

    for (int d = 1; d <= targetDepth; d++) {
        std::vector<ScoredMove> currentDepthScores;
        bool depthFinished = true;

        for (auto& sm : lastCompletedScores) {
            nnue_state[1].dirtyPiece.dirtyNum = 0;
            nnue_state[1].accumulator.computedAccumulation = 0;

            if (!board.MakeMove(sm.m, true)) continue;
            int score = -negamax(d - 1, -MATE_SCORE, MATE_SCORE, 1, Move(), false, false);
            board.UndoMove(sm.m, true);

            if (stop) {
                depthFinished = false;
                break;
            }
            currentDepthScores.push_back({sm.m, score});
        }

        if (depthFinished && !currentDepthScores.empty()) {
            std::sort(currentDepthScores.begin(), currentDepthScores.end(),
                      [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });
            lastCompletedScores = currentDepthScores;
            if (std::abs(lastCompletedScores[0].score) > MATE_SCORE_BOUND) break;
        } else {
            break;
        }
    }

    isSearching = false;

    if (board.isDebugMode) {
        std::cout << "info string --- Top 10 Initial Candidates ---" << std::endl;
        int printLimit = std::min((int)lastCompletedScores.size(), 10);
        for (int i = 0; i < printLimit; ++i) {
            std::cout << "info string rank " << (i + 1)
            << ": " << lastCompletedScores[i].m.toAlgebraic()
            << " | score: " << (board.getSideToMove() == WHITE ? lastCompletedScores[i].score : -lastCompletedScores[i].score) << std::endl;
        }
    }

    int bestScore = lastCompletedScores[0].score;
    int limit = settings.topNMoveOff ? (int)lastCompletedScores.size() : std::min((int)lastCompletedScores.size(), settings.topNmove);

    std::vector<int> validIndices;
    validIndices.push_back(0);

    if (board.isDebugMode && !lastCompletedScores.empty()) {
        std::cout << "info string [FILTER] Removed best move: " << lastCompletedScores[0].m.toAlgebraic() << std::endl;
    }

    for (int i = 1; i < limit; i++) {
        if (std::abs(bestScore - lastCompletedScores[i].score) <= settings.blunderThreshold) {
            bool isEmbarrassingBlunder = false;

            if (settings.preventEmbarrassingBlunders) {
                Move candidateMove = lastCompletedScores[i].m;
                if (board.MakeMove(candidateMove, true)) {
                    MoveList enemyCaptures;
                    MoveGenerator::GenerateMoves(board, enemyCaptures, true);

                    for (const auto& enemyMove : enemyCaptures) {
                        if (enemyMove.getFlags() == EN_PASSANT) continue;

                        PieceType capturedPiece = board.getPieceAt(enemyMove.getTo(), (Color)(board.getSideToMove() ^ 1));
                        PieceType attackingPiece = enemyMove.getPieceType();

                        if (capturedPiece >= KNIGHT && capturedPiece <= QUEEN) {
                            bool isProtectedByUs = board.isSquareAttacked(enemyMove.getTo(), (Color)(board.getSideToMove() ^ 1));

                            int valCap = Evaluation::GetPieceValue(capturedPiece);
                            int valAtk = Evaluation::GetPieceValue(attackingPiece);

                            if (!isProtectedByUs || attackingPiece == PAWN || valCap > valAtk) {
                                isEmbarrassingBlunder = true;
                                break;
                            }
                        }
                    }
                    board.UndoMove(candidateMove, true);
                }
            }

            if (!isEmbarrassingBlunder) {
                validIndices.push_back(i);
            } else if (board.isDebugMode) {
                std::cout << "info string [FILTER] Removed embarrassing blunder: " << lastCompletedScores[i].m.toAlgebraic() << std::endl;
            }
        }
    }

    if (validIndices.empty()) {
        if (lastCompletedScores.size() > 1) validIndices.push_back(1);
        else validIndices.push_back(0);
    }

    if (board.isDebugMode) {
        std::cout << "info string --- Final Valid Candidates (After Filtering) ---" << std::endl;
        for (int idx : validIndices) {
            std::cout << "info string rank " << (idx + 1)
            << ": " << lastCompletedScores[idx].m.toAlgebraic()
            << " | score: " << (board.getSideToMove() == WHITE ? lastCompletedScores[idx].score : -lastCompletedScores[idx].score)
            << (idx == 0 ? " (FILTERED BEST)" : "") << std::endl;
        }
    }

    std::uniform_int_distribution<> topDis(0, validIndices.size() - 1);
    int chosenIndex = validIndices[topDis(gen)];

    if (board.isDebugMode) {
        std::cout << "info string Bot picked move: " << lastCompletedScores[chosenIndex].m.toAlgebraic()
                  << " (rank " << (chosenIndex + 1) << ", score " << lastCompletedScores[chosenIndex].score << ")" << std::endl;
    }

    movesWithoutBlunderOnPropuse = 0;
    return lastCompletedScores[chosenIndex].m.isValid() ? lastCompletedScores[chosenIndex].m : lastCompletedScores[0].m;
}

Move Searcher::GetRobotMove() {
    Move m;

    if (currentSettings.areBlundersOnPurposeEnabled)
        return GetBestAmongTopMoves(currentSettings);

    return IterativeDeepening();
}

void Searcher::ClearSearcher() {
    ClearKillers();
    ClearHistory();
    tt.Clear();
    repetitionTable.Clear();
    movesWithoutBlunderOnPropuse = 0;
}

void Searcher::setDifficulty(Difficulty diff) {

    currentDiff = diff;

    switch (diff) {
    case Difficulty::EASY:
        currentSettings = SearcherSettings::getSettings(Difficulty::EASY);
        break;

    case Difficulty::MEDIUM:
        currentSettings = SearcherSettings::getSettings(Difficulty::MEDIUM);
        break;

    case Difficulty::HARD:
        currentSettings = SearcherSettings::getSettings(Difficulty::HARD);
        break;

    case Difficulty::IMPOSSIBLE:
        currentSettings = SearcherSettings::getSettings(Difficulty::IMPOSSIBLE);
        currentSettings.maxDepth = MAXIMUM_DEPTH;
        break;

    default:
        break;
    }
}

SearcherType Searcher::getType() const {
    return SearcherType::IMRPOVED_SEARCHER;
}

std::string Searcher::getName() const {
    return "Új robot";
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

    return "";
}
