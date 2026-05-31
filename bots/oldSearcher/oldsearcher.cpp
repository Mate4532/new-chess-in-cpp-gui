#include "oldsearcher.h"
#include "OldEvaluation.h"
#include "OldWorseEvaluation.h"
#include "OldLMR.h"

#include <chrono>
#include <iostream>
#include <random>

using namespace OldEvaluation;
using namespace OldSearcher;
using namespace OldMoveOrdering;
using namespace OldTT;
using namespace OldLMR;

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Searcher::setTournamentTime(long long timeLeftMs, long long incrementMs) {
    this->timeLeftMs = timeLeftMs;
    this->incrementMs = incrementMs;
}

void Searcher::stopSearch() {
    stop = true;
    isStoppedManually = true;
}

int Searcher::quiescence(int alpha, int beta, int ply) {
    nodes++;

    if ((nodes & 511) == 0 && now_ms() - startTime >= hardTimeLimit) stop = true;
    if (stop) return alpha;

    bool inCheck = board.isSquareAttacked(board.getKingSquare(board.getSideToMove()), (Color)(board.getSideToMove() ^ 1));

    if (!inCheck) {
        int standPat = currentSettings.worseEvaluationEnabled
            ? OldWorseEvaluation::Evaluation::EvaluatePos(board)
            : OldEvaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

        if (standPat >= beta) return standPat;

        int BIG_DELTA = 975;
        if (board.hasPromotingPawn()) BIG_DELTA += 775;
        if (standPat < alpha - BIG_DELTA) return alpha;

        if (alpha < standPat) alpha = standPat;
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, !inCheck);

    Move dummyKillers[2] = { Move(), Move() };
    MoveOrdering::SortMoves(board, moves, Move(), historyMoves, dummyKillers);

    int movesSearched = 0;
    for (const Move& m : moves) {

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        PieceType mPieceType = m.getPieceType();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE) && mPieceType == KING;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        if (!inCheck && isCapture && !isPromo) {
            if (MoveOrdering::See(board, m) < 0) {
                continue;
            }
        }

        bool isEp = flags == EN_PASSANT;

        if (isCapture) {

            if (isEp) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(PAWN, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = epSquare;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;
            }

            else {
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = isPromo ? 64 : mTo;

                PieceType capPiece = board.getPieceAt(mTo, enemy);
                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;

                if (isPromo) {
                    PieceType promoPiece = Board::GetPromotionPiece(flags);
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 3;

                    nnue_state[ply + 1].dirtyPiece.pc[2] = Evaluation::GetNnuePieceNum(promoPiece, player);
                    nnue_state[ply + 1].dirtyPiece.from[2] = 64;
                    nnue_state[ply + 1].dirtyPiece.to[2] = mTo;
                }

                else {
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                }
            }
        }

        else if (isPromo) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = 64;

            PieceType promoPiece = Board::GetPromotionPiece(flags);
            nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(promoPiece, player);
            nnue_state[ply + 1].dirtyPiece.from[1] = 64;
            nnue_state[ply + 1].dirtyPiece.to[1] = mTo;
        }

        else if (isCastle) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
            if (flags == KINGSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_TO;

            }

            else if (flags == QUEENSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_TO;
            }
        }

        else {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 1;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = mTo;
        }

        if (!board.MakeMove(m, true)) continue;
        movesSearched++;

        int score = -quiescence(-beta, -alpha, ply + 1);
        board.UndoMove(m, true);

        if (stop) return alpha;
        if (score >= beta) {
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (inCheck && movesSearched == 0) {
        return -MATE_SCORE + ply;
    }

    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply) {

    nodes++;
    bool isPvNode = (beta - alpha > 1);

    if ((nodes & 511) == 0 && now_ms() - startTime >= hardTimeLimit)
        stop = true;
    if (stop)
        return alpha;

    int originalAlpha = alpha;
    uint64_t hash = board.getHash();
    if (ply > 0) {
        if (board.getHalfMoveClock() >= 100 || board.getRepetitionTable().Contains(hash) || board.IsInsufficientMaterial()) {
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

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    bool inCheck = board.isSquareAttacked(board.getKingSquare(board.getSideToMove()), (Color)(board.getSideToMove() ^ 1));

    if (inCheck) {
        depth++;
    }

    int staticEval = SCORE_NONE;
    evalHistory[ply] = SCORE_NONE;

    if (!inCheck) {
        staticEval = currentSettings.worseEvaluationEnabled
            ? OldWorseEvaluation::Evaluation::EvaluatePos(board)
            : OldEvaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

        evalHistory[ply] = staticEval;
    }

    bool improving = false;

    if (inCheck) {
        improving = false;
    }
    else {
        if (ply >= 2 && evalHistory[ply - 2] != SCORE_NONE) {
            improving = staticEval > evalHistory[ply - 2];
        }
        else if (ply >= 4 && evalHistory[ply - 4] != SCORE_NONE) {
            improving = staticEval > evalHistory[ply - 4];
        }
        else {
            improving = true;
        }
    }

    if (!inCheck && !isPvNode && depth <= 4 && ply > 0 && abs(beta) < MATE_SCORE_BOUND) {

        int evalMargin = 120 * depth;

        if (improving) {
            evalMargin -= 65;
        }

        if (staticEval - evalMargin >= beta) {
            return (staticEval + beta) / 2;
        }
    }

    if (!inCheck && !isPvNode && depth >= 3 && ply > 0 && abs(beta) < MATE_SCORE_BOUND) {
        if (staticEval >= beta - 50 && board.HasNonPawnMaterial(board.getSideToMove())) {
            int R = 3 + (depth / 6);
            nnue_state[ply + 1] = nnue_state[ply];
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
            board.MakeNullMove();
            int score = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1);
            board.UndoNullMove();
            if (stop) return alpha;
            if (score >= beta) return beta;
        }
    }

    if (depth <= 0) return quiescence(alpha, beta, ply);

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

    Move bestMoveThisNode;
    int movesSearched = 0;

    for (const Move& m : moves) {

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        PieceType mPieceType = m.getPieceType();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool quiet = !isCapture && !isPromo;
        bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE) && mPieceType == KING;
        bool isEp = flags == EN_PASSANT;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        if (isCapture) {

            if (isEp) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(PAWN, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = epSquare;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;
            }

            else {
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = isPromo ? 64 : mTo;

                PieceType capPiece = board.getPieceAt(mTo, enemy);
                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;

                if (isPromo) {
                    PieceType promoPiece = Board::GetPromotionPiece(flags);
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 3;

                    nnue_state[ply + 1].dirtyPiece.pc[2] = Evaluation::GetNnuePieceNum(promoPiece, player);
                    nnue_state[ply + 1].dirtyPiece.from[2] = 64;
                    nnue_state[ply + 1].dirtyPiece.to[2] = mTo;
                }

                else {
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                }
            }
        }

        else if (isPromo) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = 64;

            PieceType promoPiece = Board::GetPromotionPiece(flags);
            nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(promoPiece, player);
            nnue_state[ply + 1].dirtyPiece.from[1] = 64;
            nnue_state[ply + 1].dirtyPiece.to[1] = mTo;
        }

        else if (isCastle) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
            if (flags == KINGSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_TO;

            }

            else if (flags == QUEENSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_TO;
            }
        }

        else {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 1;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = mTo;
        }

        if (!board.MakeMove(m, true)) {
            continue;
        }
        movesSearched++;

        if (movesSearched == 1)
            bestMoveThisNode = m;

        bool givesCheck = board.isSquareAttacked(board.getKingSquare(enemy), player);
        bool isKiller = (ply < MAX_KILLER_HISTORY) && (m == killerMoves[ply][0] || m == killerMoves[ply][1]);

        if (depth <= 4 && movesSearched > 1 && !inCheck && !givesCheck && quiet && abs(alpha) < MATE_SCORE_BOUND && abs(beta) < MATE_SCORE_BOUND) {

            int futilityMargin = 150 * depth;

            if (staticEval + futilityMargin <= alpha) {
                board.UndoMove(m, true);
                continue;
            }
        }

        uint64_t hash_after_move = board.getHash();
        tt.Prefetch(hash_after_move);
        bool irreversible = (m.getPieceType() == PAWN) || (isCapture);

        int score = 0;

        if (movesSearched == 1) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        }
        else {
            int reduction = 0;

            if (depth >= 3 && movesSearched > 3 && quiet && !inCheck) {

                if (!givesCheck && !isKiller) {
                    reduction = LMR::LMR::GetReduction(depth, movesSearched);
                    reduction = std::clamp(reduction, 0, depth - 2);
                }
            }

            score = -negamax(depth - 1 - reduction, -alpha - 1, -alpha, ply + 1);

            if (score > alpha) {

                if (reduction > 0) {
                    score = -negamax(depth - 1, -alpha - 1, -alpha, ply + 1);
                }

                if (score > alpha && score < beta) {
                    score = -negamax(depth - 1, -beta, -alpha, ply + 1);
                }
            }
        }

        board.UndoMove(m, true);
        if (stop) return alpha;

        if (score >= beta) {
            if (quiet) {
                if (ply < MAX_KILLER_HISTORY && m.isValid() && m != killerMoves[ply][0]) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = m;
                }
                historyMoves[player][mFrom][mTo] += depth * depth;
            }
            tt.Store(hash, score, ply, depth, TT_BETA, m);
            return score;
        }

        if (score > alpha) {
            alpha = score;
            bestMoveThisNode = m;
        }
    }

    if (movesSearched == 0) {
        return inCheck ? -MATE_SCORE + ply : 0;
    }

    TTFlag flag = (alpha <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    tt.Store(hash, alpha, ply, depth, flag, bestMoveThisNode);

    return alpha;
}

void Searcher::PrepareSearcher() {
    startTime = now_ms();
    stop = false;
    isStoppedManually = false;
    isSearching = true;
    nodes = 0;

    if (rtum == RobotTimeUsageMode::FIXED_TIME) {
        softTimeLimit = fixedTimePerMoveMs;
        hardTimeLimit = fixedTimePerMoveMs;
    }
    else {
        int movesToGo = 40;
        softTimeLimit = timeLeftMs / movesToGo;
        softTimeLimit += (incrementMs * 8) / 10;

        if (softTimeLimit >= timeLeftMs) {
            softTimeLimit = std::max((long long)100, timeLeftMs - 200);
        }
        if (softTimeLimit < 100) {
            softTimeLimit = 100;
        }

        hardTimeLimit = softTimeLimit * 3;
        if (hardTimeLimit >= timeLeftMs - 100) {
            if (timeLeftMs < 1000) {
                hardTimeLimit = timeLeftMs / 2;
            }
            else {
                hardTimeLimit = timeLeftMs - 300;
            }
        }
    }

    AgeHistory();
    ClearKillers();
    tt.NewWrite();

    nnue_state[0].dirtyPiece.dirtyNum = 0;
    nnue_state[0].accumulator.computedAccumulation = 0;
}

void Searcher::ClearKillers() {
    for (int i = 0; i < MAX_KILLER_HISTORY; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }
}

void Searcher::AgeHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < SQUARE_COUNT; f++)
            for (int t = 0; t < SQUARE_COUNT; t++)
                historyMoves[c][f][t] >>= 1;
}

void Searcher::ClearHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < SQUARE_COUNT; f++)
            for (int t = 0; t < SQUARE_COUNT; t++)
                historyMoves[c][f][t] = 0;
}

Move Searcher::IterativeDeepening() {
    PrepareSearcher();

    MoveList rootMoves;
    MoveGenerator::GenerateMoves(board, rootMoves);

    MoveList legalRootMoves;
    for (const Move& m : rootMoves) {
        if (board.MakeMove(m, true)) {
            legalRootMoves.push_back(m);
            board.UndoMove(m, true);
        }
    }

    if (legalRootMoves.count == 0) {
        isSearching = false;
        return Move();
    }

    if (legalRootMoves.count == 1) {
        isSearching = false;
        return legalRootMoves[0];
    }

    Move bestMoveToPlay = legalRootMoves[0];
    int lastScore = 0;
    int stableBestMoveCount = 0;

    int window = 50;

    for (int depth = 1; depth <= currentSettings.maxDepth; depth++) {

        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;

        if (depth >= 4) {
            alpha = std::max(-MATE_SCORE, lastScore - window);
            beta = std::min(MATE_SCORE, lastScore + window);
        }

        int score;

        while (true) {
            score = negamax(depth, alpha, beta, 0);

            if (stop) break;

            if (score <= alpha) {
                alpha = std::max(-MATE_SCORE, alpha - window);
                window += window / 2;
            }
            else if (score >= beta) {
                beta = std::min(MATE_SCORE, beta + window);
                window += window / 2;
            }
            else {
                break;
            }
        }

        if (stop) break;

        int prevScore = lastScore;

        lastScore = score;
        window = 50;

        int ttScore = 0;
        Move ttMove;
        tt.Probe(board.getHash(), 0, depth, alpha, beta, ttScore, ttMove);

        if (ttMove.isValid()) {
            if (ttMove == bestMoveToPlay) {
                stableBestMoveCount++;
            }
            else {
                stableBestMoveCount = 0;
            }
            bestMoveToPlay = ttMove;
        }

        bool inCrisis = (depth > 3 && score < prevScore - 50);
        long long timeSpent = now_ms() - startTime;

        LOG_DEBUG("info depth " << depth << " score "
            << ((abs(score) > MATE_SCORE_BOUND)
                ? "mate " + std::to_string((score > 0) ? (MATE_SCORE + 1 - score) / 2 : -(MATE_SCORE + 1 + score) / 2)
                : "cp " + std::to_string(board.getSideToMove() == WHITE ? score : -score))
            << " time " << timeSpent
            << " nodes " << nodes
            << " pv " << bestMoveToPlay.toAlgebraic());

            if (IsMateScore(score) && score > 0) break;

        if (rtum == RobotTimeUsageMode::FIXED_TIME) {
            if (timeSpent >= fixedTimePerMoveMs) {
                break;
            }
        }
        else {
            if (!inCrisis && stableBestMoveCount >= 3 && timeSpent >= (softTimeLimit * 0.6)) break;
            if (timeSpent >= softTimeLimit && !inCrisis) break;
            if (timeSpent > hardTimeLimit * 0.8) break;
        }
    }

    LOG_DEBUG("Final Score: "
        << ((abs(lastScore) > MATE_SCORE_BOUND)
            ? "mate " + std::to_string((lastScore > 0) ? (MATE_SCORE + 1 - lastScore) / 2 : -(MATE_SCORE + 1 + lastScore) / 2)
                      : "cp " + std::to_string(board.getSideToMove() == WHITE ? lastScore : -lastScore) + " pv " + bestMoveToPlay.toAlgebraic()));

        isSearching = false;

    if (isStoppedManually) return Move();

    return bestMoveToPlay;
}

Move Searcher::GetRobotMove() {
    return IterativeDeepening();
}

void Searcher::ClearSearcher() {
    ClearKillers();
    ClearHistory();
    tt.Clear();
    movesWithoutBlunderOnPropuse = 0;
}
void Searcher::setDifficulty(const Difficulty& diff) {

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

std::string Searcher::getName() const {
    return "Régi robot";
}

std::string Searcher::getNameToSaveInFile() const {
    return "Old_searcher";
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

std::string Searcher::getBotDirectoryPath() const {
    return "bots\\oldSearcher";
};
