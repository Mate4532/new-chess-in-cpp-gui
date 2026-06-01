#include "Searcher.h"
#include "Evaluation.h"
#include "WorseEvaluation.h"
#include "LMR.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <cstring>
#include <random>

using namespace ImprovedSearcher;
using namespace ImprovedEvaluation;
using namespace ImprovedMoveOrdering;
using namespace ImprovedTT;
using namespace ImprovedLMR;

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Searcher::shouldStop() {
    localNodes++;
    if ((localNodes & 2047) == 0) {
        nodes += 2047;
        localNodes = 0;
        if (now_ms() - startTime >= hardTimeLimit) return true;
    }

    return false;
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
    pvLength[ply] = 0;

    if (shouldStop()) stop = true;
    if (isStopped()) return alpha;

    uint64_t hash = board.getHash();
    if (ply > 0) {
        if (board.getHalfMoveClock() >= 100 || board.getRepetitionTable().Contains(hash) || board.IsInsufficientMaterial()) {
            return 0;
        }
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply);
        if (alpha >= beta) return alpha;
    }

    int originalAlpha = alpha;
    if (ply >= MAXIMUM_DEPTH - 1) return Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

    bool inCheck = board.isSquareAttacked(board.getKingSquare(board.getSideToMove()), (Color)(board.getSideToMove() ^ 1));

    if (inCheck && ply >= 30) {
        return currentSettings.worseEvaluationEnabled
                   ? WorseEvaluation::Evaluation::EvaluatePos(board)
                   : Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);
    }

    Color player = board.getSideToMove();

    if (!inCheck) {
        int standPat = currentSettings.worseEvaluationEnabled
                           ? WorseEvaluation::Evaluation::EvaluatePos(board)
                           : Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

        if (standPat >= beta) {
            return standPat;
        }

        int BIG_DELTA = 975;
        if (board.hasAdvancedPassedPawn(player)) BIG_DELTA += 775;
        if (standPat < alpha - BIG_DELTA) return alpha;

        if (alpha < standPat) alpha = standPat;
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, !inCheck);

    Move dummyKillers[2] = { Move(), Move() };
    int scores[MoveOrdering::SCORE_SIZE];
    MoveOrdering::ScoreMoves(board, moves, Move(), historyMoves, dummyKillers, scores);

    int n = moves.count;
    int movesSearched = 0;
    Move bestMoveThisNode = Move();
    for (int i = 0; i < n; ++i) {

        int bestIndex = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[bestIndex]) bestIndex = j;
        }

        if (bestIndex != i) {
            std::swap(scores[i], scores[bestIndex]);
            std::swap(moves[i], moves[bestIndex]);
        }

        const Move& m = moves[i];
        int currentScore = scores[i];

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        PieceType mPieceType = m.getPieceType();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE) && mPieceType == KING;

        Color enemy = (Color)(player ^ 1);
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        if (isPromo) {
            PieceType promoPiece = Board::GetPromotionPiece(flags);
            if (promoPiece != QUEEN) {
                continue;
            }
        }

        if (!inCheck && isCapture && !isPromo) {
            if (currentScore < 0) {
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

        if (!bestMoveThisNode.isValid()) bestMoveThisNode = m;

        int score = -quiescence(-beta, -alpha, ply + 1);
        board.UndoMove(m, true);

        if (isStopped()) return alpha;
        if (score >= beta) {
            return score;
        }
        if (score > alpha) {
            alpha = score;
            bestMoveThisNode = m;
        }
    }

    if (movesSearched == 0 && inCheck) {
        return -MATE_SCORE + ply;
    }

    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply) {
    pvLength[ply] = 0;

    if (shouldStop()) stop = true;
    if (isStopped()) return alpha;

    if (ply >= MAXIMUM_DEPTH - 1) return Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);
    uint64_t hash = board.getHash();
    if (ply > 0) {
        if (board.getHalfMoveClock() >= 100 || board.getRepetitionTable().Contains(hash) || board.IsInsufficientMaterial()) {
            return 0;
        }
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply);
        if (alpha >= beta) return alpha;
    }

    bool isPvNode = (beta - alpha > 1);
    int originalAlpha = alpha;

    int ttScore = 0;
    Move ttMove = Move();
    int ttDepth = 0;
    TTFlag ttFlag = TT_ALPHA;

    bool foundInTT = tt->Probe(hash, ply, depth, alpha, beta, ttScore, ttMove, ttDepth, ttFlag);

    if (foundInTT && ply > 0 && !isPvNode) {
        if (ttMove.isValid()) {
            pvTable[ply][0] = ttMove;
            pvLength[ply] = 1;
        }
        return ttScore;
    }

    if (!ttMove.isValid() && depth >= 4 && ply > 0) {

        int iidDepth = (isPvNode) ? depth - 2 : depth / 2;

        negamax(iidDepth, alpha, beta, ply);

        tt->Probe(hash, ply, iidDepth, alpha, beta, ttScore, ttMove, ttDepth, ttFlag);
    }

    MoveList moves;
    if (ply == 0) {
        for (const RootMove& rm : rootMoves) {
            moves.push_back(rm.m);
        }
    } else {
        MoveGenerator::GenerateMoves(board, moves);
    }

    bool inCheck = board.isSquareAttacked(board.getKingSquare(board.getSideToMove()), (Color)(board.getSideToMove() ^ 1));

    bool isSingleReply = false;

    if (inCheck) {
        int legalEvasions = 0;
        for (int i = 0; i < moves.count; i++) {
            if (board.MakeMove(moves[i], true)) {
                legalEvasions++;
                board.UndoMove(moves[i], true);

                if (legalEvasions > 1) {
                    break;
                }
            }
        }
        isSingleReply = (legalEvasions == 1);
    }

    int staticEval = SCORE_NONE;
    evalHistory[ply] = SCORE_NONE;

    if (!inCheck) {
        staticEval = currentSettings.worseEvaluationEnabled
                         ? WorseEvaluation::Evaluation::EvaluatePos(board)
                         : Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

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
        if (staticEval >= beta && board.HasNonPawnMaterial(board.getSideToMove())) {
            if (!board.hasAdvancedPassedPawn(board.getSideToMove())) {

                int R = 3 + (depth / 6);

                nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
                nnue_state[ply + 1].accumulator.computedAccumulation = 0;

                board.MakeNullMove();
                int score = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1);
                board.UndoNullMove();

                if (isStopped()) return alpha;
                if (score >= beta) return beta;
            }
        }
    }

    if (depth <= 0) return quiescence(alpha, beta, ply);

    Move currentKillers[2] = { Move(), Move() };

    if (ply < MAX_KILLER_HISTORY) {
        currentKillers[0] = killerMoves[ply][0];
        currentKillers[1] = killerMoves[ply][1];
    }

    int scores[MoveOrdering::SCORE_SIZE];
    if (ply == 0) {
        for (int i = 0; i < moves.count; i++) {
            scores[i] = 1000000 - i;
        }
    } else {
        MoveOrdering::ScoreMoves(board, moves, ttMove, historyMoves, currentKillers, scores);
    }

    Move bestMoveThisNode;
    int n = moves.count;
    int movesSearched = 0;
    for (int i = 0; i < n; ++i) {

        int bestIndex = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[bestIndex]) bestIndex = j;
        }

        if (bestIndex != i) {
            std::swap(scores[i], scores[bestIndex]);
            std::swap(moves[i], moves[bestIndex]);
        }

        const Move& m = moves[i];

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();


        MoveFlag flags = m.getFlags();
        PieceType mPieceType = m.getPieceType();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool quiet = !isCapture && !isPromo;
        bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE) && mPieceType == KING;
        bool isEp = flags == EN_PASSANT;
        bool isAdvancedPassedPawnPush = (mPieceType == PAWN) && board.isAdvancedPassedPawnPush(m);

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
        tt->Prefetch(hash_after_move);

        int score = 0;

        int extension = 0;

        if (isSingleReply) {
            extension = 1;
        }
        else if (isPvNode && givesCheck && movesSearched == 1) {
            extension = 1;
        }

        if (movesSearched == 1) {
            score = -negamax(depth + extension - 1, -beta, -alpha, ply + 1);
        }
        else {
            int reduction = 0;
            int seeScore = MoveOrdering::See(board, m);
            bool badQuiet = quiet && (seeScore < 0);

            if (depth >= 3 && movesSearched > 1 && !inCheck) { // movesSearched > 1 + quiet is not here
                if (quiet) { // quiet
                    reduction = LMR::LMR::GetReduction(depth, movesSearched, isPvNode); // PVNode

                    if (!improving) {
                        reduction += 1;
                    }

                    if (badQuiet) {
                         reduction += 1;
                    }

                    if (isKiller){
                        reduction -= 1;
                    }

                    if (isPvNode) {
                        reduction -= 1;
                    }

                    if (isAdvancedPassedPawnPush) {
                        reduction -= 1;
                    }

                    if (givesCheck) {
                        reduction -= 1;
                    }

                    int histScore = historyMoves[player][mFrom][mTo];

                    int historyModifier = -(histScore / 2048);

                    historyModifier = std::clamp(historyModifier, -1, 2);

                    reduction += historyModifier;

                    reduction = std::clamp(reduction, 0, depth);
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
        if (isStopped()) return alpha;

        if (ply == 0) {
            for (RootMove& rm : rootMoves) {
                if (rm.m == m) {
                    rm.score = score;
                    break;
                }
            }
        }

        if (score >= beta) {
            if (quiet) {
                if (ply < MAX_KILLER_HISTORY && m.isValid() && m != killerMoves[ply][0]) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = m;
                }
                int bonus = depth * depth;
                if (bonus > 1024) bonus = 1024;

                int& goodHist = historyMoves[player][mFrom][mTo];
                goodHist += bonus - (goodHist * std::abs(bonus)) / 8192;

                for (int j = 0; j < i; j++) {
                    const Move& badMove = moves[j];
                    MoveFlag badFlags = badMove.getFlags();

                    if (!(badFlags & CAPTURE_FLAG) && !(badFlags & PROMOTION_FLAG)) {
                        int& badHist = historyMoves[player][badMove.getFrom()][badMove.getTo()];
                        badHist -= bonus + (badHist * std::abs(bonus)) / 8192;
                    }
                }
            }
            tt->Store(hash, score, ply, depth, TT_BETA, m);
            return score;
        }

        if (score > alpha) {
            alpha = score;
            bestMoveThisNode = m;

            pvTable[ply][0] = m;
            for (int j = 0; j < pvLength[ply + 1]; j++) {
                pvTable[ply][j + 1] = pvTable[ply + 1][j];
            }
            pvLength[ply] = pvLength[ply + 1] + 1;

        }
    }

    if (movesSearched == 0) {
        return inCheck ? -MATE_SCORE + ply : 0;
    }

    TTFlag flag = (alpha <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    tt->Store(hash, alpha, ply, depth, flag, bestMoveThisNode);

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

    nnue_state[0].dirtyPiece.dirtyNum = 0;
    nnue_state[0].accumulator.computedAccumulation = 0;
}

void Searcher::ClearKillers() {
    for (int i = 0; i < MAX_KILLER_HISTORY; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }
}

//void Searcher::AgeHistory() {
//    for (int c = 0; c < 2; c++)
//        for (int f = 0; f < SQUARE_COUNT; f++)
//            for (int t = 0; t < SQUARE_COUNT; t++)
//                historyMoves[c][f][t] /= 2;
//}

void Searcher::ClearHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < SQUARE_COUNT; f++)
            for (int t = 0; t < SQUARE_COUNT; t++)
                historyMoves[c][f][t] = 0;
}

Move Searcher::IterativeDeepening(bool silent) {

    PrepareSearcher();

    if (!silent) {
        std::vector<Move> moves = board.getMoveHistory();
        std::string movesString = "";
        for (Move m : moves){
            if (!m.isValid()) continue;
            movesString += m.toAlgebraic();
            movesString += " ";
        }
        LOG_DEBUG("\n");
        LOG_DEBUG("Beginner FEN: " << board.getBeginnerFen());
        LOG_DEBUG("Current pos fen: " << board.GetFEN());
        LOG_DEBUG("All moves: " << movesString);
    }

    MoveList rawRootMoves;
    MoveGenerator::GenerateMoves(board, rawRootMoves);

    rootMoves.clear();
    for (const Move& m : rawRootMoves) {
        if (board.MakeMove(m, true)) {
            rootMoves.push_back({m, -MATE_SCORE});
            board.UndoMove(m, true);
        }
    }

    if (rootMoves.empty()) {
        isSearching = false;
        return Move();
    }

    if (rootMoves.size() == 1) {
        isSearching = false;
        return rootMoves[0].m;
    }

    Move bestMoveToPlay = rootMoves[0].m;
    int lastScore = 0;
    int stableBestMoveCount = 0;
    int scoreHistory[MAXIMUM_DEPTH + 1] = {0};

    std::string bestPvStringSoFar = "";

    for (int depth = 1; depth <= currentSettings.maxDepth; depth++) {

        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;
        int delta = 60;

        if (depth >= 4) {
            int expectedScore = scoreHistory[depth - 2];
            alpha = std::max(-MATE_SCORE, expectedScore - delta);
            beta = std::min(MATE_SCORE, expectedScore + delta);
        }

        int score;

        while (true) {
            score = negamax(depth, alpha, beta, 0);

            if (isStopped()) break;

            if (score <= alpha) {
                alpha = -MATE_SCORE;
            }
            else if (score >= beta) {
                beta = MATE_SCORE;
            }
            else {
                break;
            }
        }

        if (isStopped()) break;

        std::stable_sort(rootMoves.begin(), rootMoves.end(), [](const RootMove& a, const RootMove& b) {
            return a.score > b.score;
        });

        int prevScore = lastScore;
        lastScore = score;

        scoreHistory[depth] = score;

        Move currentBest = rootMoves[0].m;
        Move previousBestMove = bestMoveToPlay;

        if (currentBest == bestMoveToPlay) {
            stableBestMoveCount++;
        } else {
            stableBestMoveCount = 0;
        }

        bestMoveToPlay = currentBest;

        bool inCrisis = (depth > 3 && score < prevScore - 50);
        long long timeSpent = now_ms() - startTime;

        std::string currentPvString = "";
        for (int i = 0; i < pvLength[0]; i++) {
            currentPvString += pvTable[0][i].toAlgebraic() + " ";
        }

        if (pvLength[0] >= 2) {
            bestPvStringSoFar = currentPvString;
        }
        else if (pvLength[0] < 2 && currentBest == previousBestMove && bestPvStringSoFar != "") {
            currentPvString = bestPvStringSoFar;
        }

        if (!silent) LOG_DEBUG("info depth " << depth << " score "
                      << ((abs(score) > MATE_SCORE_BOUND)
                              ? "mate " + std::to_string((score > 0) ? (MATE_SCORE + 1 - score) / 2 : -(MATE_SCORE + 1 + score) / 2)
                              : "cp " + std::to_string(board.getSideToMove() == WHITE ? score : -score))
                      << " time " << timeSpent
                      << " nodes " << (nodes + localNodes)
                                    << " | pv " << currentPvString);

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

    if (!silent) {

        std::string currentPvString = "";
        for (int i = 0; i < pvLength[0]; i++) {
            currentPvString += pvTable[0][i].toAlgebraic() + " ";
        }

        if (pvLength[0] == 0 || currentPvString.empty() || isStopped()) {
            currentPvString = bestPvStringSoFar;
        }

        bool isMate = std::abs(lastScore) > MATE_SCORE_BOUND;

        LOG_DEBUG("Final Score: ");

        if (isMate) {
            int mateIn = (lastScore > 0) ? (MATE_SCORE + 1 - lastScore) / 2 : -(MATE_SCORE + 1 + lastScore) / 2;
            LOG_DEBUG("mate " << mateIn << " ");
        } else {
            int cpScore = (board.getSideToMove() == WHITE ? lastScore : -lastScore);
            LOG_DEBUG("cp " << cpScore << " ");
        }

        LOG_DEBUG("| pv " << currentPvString);
    }

    isSearching = false;

    if (isStoppedManually) return Move();

    return bestMoveToPlay;
}

Move Searcher::GetBestAmongTopMoves(const SearcherSettings& settings) {
    static std::mt19937 gen(now_ms());
    std::uniform_int_distribution<> dis(1, 100);

    PrepareSearcher();

    if (settings.minNormalMovesAfterBlunder >= movesWithoutBlunderOnPropuse || dis(gen) > settings.chanceToActivatePossBlunder) {
        movesWithoutBlunderOnPropuse++;
        return IterativeDeepening();
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    Move dummyKillers[2] = { Move(), Move() };
    int scores[MoveOrdering::SCORE_SIZE];
    MoveOrdering::ScoreMoves(board, moves, Move(), historyMoves, dummyKillers, scores);

    int n = (int)moves.size();
    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[best])
                best = j;
        }
        if (best != i) {
            std::swap(scores[i], scores[best]);
            std::swap(moves[i], moves[best]);
        }
    }
    struct ScoredMove { Move m; int score; };
    std::vector<ScoredMove> lastCompletedScores;

    for (int i = 0; i < moves.size(); ++i) {
        if (board.MakeMove(moves[i], true)) {
            lastCompletedScores.push_back({moves[i], 0});
            board.UndoMove(moves[i], true);
        }
    }

    if (lastCompletedScores.empty()) return IterativeDeepening();

    int targetDepth = std::max(settings.maxDepth - 2, 3);

    for (int d = 1; d <= targetDepth; d++) {
        std::vector<ScoredMove> currentDepthScores;
        bool depthFinished = true;
        Color us = board.getSideToMove();

        for (auto& sm : lastCompletedScores) {
            nnue_state[1].dirtyPiece.dirtyNum = 0;
            nnue_state[1].accumulator.computedAccumulation = 0;

            if (!board.MakeMove(sm.m, true)) continue;
            int score = -negamax(d - 1, -MATE_SCORE, MATE_SCORE, 1);
            board.UndoMove(sm.m, true);

            if (stop) {
                depthFinished = false;
                break;
            }

            int realScore = us == WHITE ? score : -score;

            currentDepthScores.push_back({sm.m, realScore});
        }

        if (depthFinished && !currentDepthScores.empty()) {
            std::sort(currentDepthScores.begin(), currentDepthScores.end(),
                      [us](const ScoredMove& a, const ScoredMove& b) { return us == WHITE ? a.score > b.score : a.score < b.score; });
            lastCompletedScores = currentDepthScores;
            if (std::abs(lastCompletedScores[0].score) > MATE_SCORE_BOUND) break;
        } else {
            break;
        }

        if (now_ms() - startTime >= softTimeLimit) {
            break;
        }
    }

    isSearching = false;

    if (board.isDebugMode) {
        LOG_DEBUG("info string --- Top 10 Initial Candidates ---");
        int printLimit = std::min((int)lastCompletedScores.size(), 10);
        for (int i = 0; i < printLimit; ++i) {
            LOG_DEBUG("info string rank " << (i + 1)
            << ": " << lastCompletedScores[i].m.toAlgebraic()
            << " | score: " << lastCompletedScores[i].score);
        }
    }

    int bestScore = lastCompletedScores[0].score;
    int limit = settings.topNMoveOff ? (int)lastCompletedScores.size() : std::min((int)lastCompletedScores.size(), settings.topNmove);

    std::vector<int> validIndices;

    if (board.isDebugMode && !lastCompletedScores.empty()) {
        LOG_DEBUG("info string [FILTER] Removed best move: " << lastCompletedScores[0].m.toAlgebraic());
    }

    if (settings.takeFreePieces) {
        for (int i = 0; i < std::min((int)lastCompletedScores.size(), 3); i++) {
            Move m = lastCompletedScores[i].m;
            bool isCap = (m.getFlags() & CAPTURE_FLAG);
            Color us = board.getSideToMove();
            Color enemy = (Color)(us ^ 1);

            if (isCap) {
                bool isProtected = board.isSquareAttacked(m.getTo(), enemy);

                if (!isProtected) {
                    if (board.isDebugMode) {
                        LOG_DEBUG("info string [FREE PIECE] Found in 1 depth, best move made: " << m.toAlgebraic()
                        << " (rank 1 | score " << lastCompletedScores[0].score << ")");
                    }
                    return lastCompletedScores[0].m;
                }
            }
        }
    }

    for (int i = 1; i < limit; i++) {
        if (std::abs(bestScore - lastCompletedScores[i].score) <= settings.blunderThreshold) {
            bool isEmbarrassingBlunder = false;

            if (settings.preventEmbarrassingBlunders) {
                Move candidateMove = lastCompletedScores[i].m;
                Color us = board.getSideToMove();

                if (!isEmbarrassingBlunder && board.MakeMove(candidateMove, true)) {
                    MoveList enemyCaptures;
                    MoveGenerator::GenerateMoves(board, enemyCaptures, true);

                    for (const auto& enemyMove : enemyCaptures) {
                        if (enemyMove.getFlags() == EN_PASSANT) continue;

                        PieceType myCapturedPiece = board.getPieceAt(enemyMove.getTo(), us);
                        PieceType enemyAttackingPiece = enemyMove.getPieceType();

                        if (myCapturedPiece >= KNIGHT && myCapturedPiece <= QUEEN) {
                            bool isProtectedByUs = board.isSquareAttacked(enemyMove.getTo(), us);
                            int valMyPiece = Evaluation::GetPieceValue(myCapturedPiece);
                            int valEnemyPiece = Evaluation::GetPieceValue(enemyAttackingPiece);

                            if (!isProtectedByUs || enemyAttackingPiece == PAWN || valMyPiece > valEnemyPiece) {
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
                LOG_DEBUG("info string [FILTER] Removed: embarrassing blunder: " << lastCompletedScores[i].m.toAlgebraic());
            }
        }
        else {
            LOG_DEBUG("info string [FILTER] Removed: out of threshold: " << lastCompletedScores[i].m.toAlgebraic());
        }
    }

    if (validIndices.empty()) {
        if (board.isDebugMode) {
            LOG_DEBUG("info string [FALLBACK] No safe suboptimal moves found. Using best move.");
        }
        validIndices.push_back(0);
    }

    if (board.isDebugMode) {
        LOG_DEBUG("info string --- Final Valid Candidates (After Filtering) ---");
        for (int idx : validIndices) {
            LOG_DEBUG("info string rank " << (idx + 1) << ": " << lastCompletedScores[idx].m.toAlgebraic()
            << " | score: " << lastCompletedScores[idx].score << (idx == 0 ? " (FILTERED BEST)" : ""));
        }
    }

    std::uniform_int_distribution<> topDis(0, validIndices.size() - 1);
    int chosenIndex = validIndices[topDis(gen)];

    if (board.isDebugMode) {
        LOG_DEBUG("info string Bot picked move: " << lastCompletedScores[chosenIndex].m.toAlgebraic()
        << " (rank " << (chosenIndex + 1) << " | score "
        << lastCompletedScores[chosenIndex].score << ")");
    }

    movesWithoutBlunderOnPropuse = 0;
    return lastCompletedScores[chosenIndex].m.isValid() ? lastCompletedScores[chosenIndex].m : lastCompletedScores[0].m;
}

Move Searcher::GetRobotMove() {
    Move bestMove = GetMultiThreadedBestMove();

    if (currentSettings.areBlundersOnPurposeEnabled) {
        return GetBestAmongTopMoves(currentSettings);
    }

    return bestMove;
}

Move Searcher::GetMultiThreadedBestMove() {
    int numThreads = this->threads;
    if (numThreads < 1) numThreads = 1;

    tt->NewWrite();
    // AgeHistory();

    std::vector<std::thread> helpers;

    this->stop = false;
    Board board_snapshot = this->board;

    struct HistorySnapshot {
        int moves[2][SQUARE_COUNT][SQUARE_COUNT];
    };
    HistorySnapshot histCopy;
    std::memcpy(histCopy.moves, this->historyMoves, sizeof(this->historyMoves));

    for (int i = 0; i < numThreads - 1; ++i) {
        helpers.emplace_back([this, board_snapshot, histCopy]() mutable {
            auto helper = std::make_unique<Searcher>(this->tt);
            helper->isHelper = true;
            helper->abortPtr = &this->stop;
            helper->board = board_snapshot;
            helper->currentDiff = this->currentDiff;
            helper->currentSettings = this->currentSettings;
            helper->rtum = this->rtum;
            helper->fixedTimePerMoveMs.store(this->fixedTimePerMoveMs.load());
            helper->timeLeftMs.store(this->timeLeftMs.load());
            helper->incrementMs.store(this->incrementMs.load());

            std::memcpy(helper->historyMoves, histCopy.moves, sizeof(histCopy.moves));

            helper->IterativeDeepening(true);
        });
    }

    Move bestMove = IterativeDeepening(false);
    this->stop = true;

    for (auto& t : helpers) {
        if (t.joinable()) t.join();
    }

    return bestMove;
}

void Searcher::ClearSearcher() {
    ClearKillers();
    ClearHistory();
    tt->Clear();
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
    return "Új robot";
}

std::string Searcher::getNameToSaveInFile() const {
    return "Improved_searcher";
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
    return "bots\\improvedSearcher";
};

