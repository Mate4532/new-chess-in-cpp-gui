#include "Searcher.h"
#include "Evaluation.h"
#include "WorseEvaluation.h"
#include "LMR.h"
#include "UCIParsing.h"

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
    if ((++localNodes & 1023) == 0) {
        nodes += localNodes;
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

void Searcher::SetupNnueDirtyPieces(const Move& m, int ply) {
    NNUEdata& next = nnue_state[ply + 1];

    next.dirtyPiece.dirtyNum = 0;
    next.accumulator.computedAccumulation = 0;

    const Color player = board.getSideToMove();
    const Color enemy = (Color)(player ^ 1);

    const MoveFlag flags = m.getFlags();
    const PieceType mPieceType = m.getPieceType();
    const Square mFrom = m.getFrom();
    const Square mTo = m.getTo();

    const bool isCapture = flags & CAPTURE_FLAG;
    const bool isPromo = flags & PROMOTION_FLAG;
    const bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE);
    const bool isEp = (flags == EN_PASSANT);

    if (isCapture) {
        if (isEp) {
            next.dirtyPiece.dirtyNum = 2;

            next.dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
            next.dirtyPiece.from[0] = mFrom;
            next.dirtyPiece.to[0] = mTo;

            int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
            next.dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(PAWN, enemy);
            next.dirtyPiece.from[1] = epSquare;
            next.dirtyPiece.to[1] = 64;
        }
        else {
            next.dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
            next.dirtyPiece.from[0] = mFrom;
            next.dirtyPiece.to[0] = isPromo ? 64 : mTo;

            PieceType capPiece = board.getPieceAt(mTo, enemy);
            next.dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(capPiece, enemy);
            next.dirtyPiece.from[1] = mTo;
            next.dirtyPiece.to[1] = 64;

            if (isPromo) {
                PieceType promoPiece = Board::GetPromotionPiece(flags);
                next.dirtyPiece.dirtyNum = 3;

                next.dirtyPiece.pc[2] = Evaluation::GetNnuePieceNum(promoPiece, player);
                next.dirtyPiece.from[2] = 64;
                next.dirtyPiece.to[2] = mTo;
            }
            else {
                next.dirtyPiece.dirtyNum = 2;
            }
        }
    }

    else if (isPromo) {
        next.dirtyPiece.dirtyNum = 2;

        next.dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
        next.dirtyPiece.from[0] = mFrom;
        next.dirtyPiece.to[0] = 64;

        PieceType promoPiece = Board::GetPromotionPiece(flags);
        next.dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(promoPiece, player);
        next.dirtyPiece.from[1] = 64;
        next.dirtyPiece.to[1] = mTo;
    }

    else if (isCastle) {
        next.dirtyPiece.dirtyNum = 2;

        next.dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
        next.dirtyPiece.from[0] = mFrom;
        next.dirtyPiece.to[0] = mTo;

        next.dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);

        if (flags == KINGSIDE_CASTLE) {
            next.dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_FROM;
            next.dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_TO;
        }
        else {
            next.dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_FROM;
            next.dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_TO;
        }
    }

    else {
        next.dirtyPiece.dirtyNum = 1;

        next.dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
        next.dirtyPiece.from[0] = mFrom;
        next.dirtyPiece.to[0] = mTo;
    }
}

int Searcher::quiescence(int alpha, int beta, int ply) {
    pvLength[ply] = 0;

    if (shouldStop()) stop = true;
    if (isStopped()) return alpha;

    if (ply > 0) {
        if (board.isDrawWithoutStealmate(true)) { // 2-fold inside the search tree
            return 0;
        }
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply);
        if (alpha >= beta) return alpha;
    }

    if (ply >= MAXIMUM_DEPTH - 1) return Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

    // TT in quiescence: cut at non-PV nodes, and use the stored move for ordering
    const uint64_t hash = board.getHash();
    const int originalAlpha = alpha;
    Move ttMove;
    {
        int ttScore = 0, ttDepth = 0;
        TTFlag ttFlag = TT_ALPHA;
        if (tt->Probe(hash, ply, 0, alpha, beta, ttScore, ttMove, ttDepth, ttFlag) && beta - alpha == 1) {
            return ttScore;
        }
    }

    bool inCheck = board.isSquareAttacked(board.getKingSquare(board.getSideToMove()), (Color)(board.getSideToMove() ^ 1));

    Color player = board.getSideToMove();

    // fail-soft: return the best score found, not just alpha, so TT bounds stay tight
    int bestScore = -MATE_SCORE + ply;

    if (!inCheck) {
        int standPat = currentSettings.worseEvaluationEnabled
                           ? WorseEvaluation::Evaluation::EvaluatePos(board)
                           : Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

        if (standPat >= beta) {
            return standPat;
        }

        int BIG_DELTA = 975;
        if (board.hasAdvancedPassedPawn(player)) BIG_DELTA += 775;
        if (standPat < alpha - BIG_DELTA) return standPat + BIG_DELTA;

        if (alpha < standPat) alpha = standPat;
        bestScore = standPat;
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, !inCheck);

    Move dummyKillers[2] = { Move(), Move() };
    int scores[MoveOrdering::SCORE_SIZE];
    MoveOrdering::ScoreMoves(board, moves, ttMove, historyMoves, dummyKillers, Move(), scores);

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

        MoveFlag flags = m.getFlags();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;

        if (isPromo) {
            PieceType promoPiece = Board::GetPromotionPiece(m);
            if (promoPiece != QUEEN && promoPiece != KNIGHT) {
                continue;
            }
        }

        if (!inCheck && isCapture) {
            if (currentScore < 0) {
                continue;
            }
        }

        SetupNnueDirtyPieces(m, ply);

        if (!board.MakeMove(m, true)) continue;

        movesSearched++;
        int score = 0;

        if (!bestMoveThisNode.isValid()) bestMoveThisNode = m;
        score = -quiescence(-beta, -alpha, ply + 1);

        board.UndoMove(m, true);

        if (isStopped()) return alpha;
        if (score >= beta) {
            tt->Store(hash, score, ply, 0, TT_BETA, m);
            return score;
        }
        if (score > bestScore) bestScore = score;
        if (score > alpha) {
            alpha = score;
            bestMoveThisNode = m;
        }
    }

    if (movesSearched == 0 && inCheck) {
        return -MATE_SCORE + ply;
    }

    // only bounds are stored from quiescence, so a depth-0 EXACT never overwrites a deeper entry's score
    tt->Store(hash, bestScore, ply, 0, bestScore > originalAlpha ? TT_BETA : TT_ALPHA, bestMoveThisNode);
    return bestScore;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply, Move excludedMove, Move prevMove) {
    pvLength[ply] = 0;

    if (shouldStop()) stop = true;
    if (isStopped()) return alpha;

    if (ply >= MAXIMUM_DEPTH - 1) return Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);
    uint64_t hash = board.getHash();

    const bool isExclusionSearch = excludedMove.isValid();
    if (isExclusionSearch) hash ^= 0x9E3779B97F4A7C15ULL;

    if (ply > 0) {
        if (board.isDrawWithoutStealmate(true)) { // 2-fold inside the search tree
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

    // near the 50-move limit a stored score may not know the position is about to be drawn
    if (board.getHalfMoveClock() >= 90) foundInTT = false;

    if (foundInTT && ply > 0) {
        if (!isPvNode) {
            return ttScore;
        }
    }

    if (depth <= 0) return quiescence(alpha, beta, ply);

    // IIR: without a TT move, search one ply shallower instead of running a separate IID search
    if (!ttMove.isValid() && depth >= 4 && ply > 0 && !isExclusionSearch) {
        depth--;
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

    // if (inCheck) {
    //     int legalEvasions = 0;
    //     for (int i = 0; i < moves.count; i++) {
    //         if (board.MakeMove(moves[i], true)) {
    //             legalEvasions++;
    //             board.UndoMove(moves[i], true);

    //             if (legalEvasions > 1) {
    //                 break;
    //             }
    //         }
    //     }
    //     isSingleReply = (legalEvasions == 1);
    // }

    int staticEval = SCORE_NONE;
    evalHistory[ply] = SCORE_NONE;

    if (!inCheck) {
        staticEval = currentSettings.worseEvaluationEnabled
                         ? WorseEvaluation::Evaluation::EvaluatePos(board)
                         : Evaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

        if (ttDepth > 0) {
            if (ttFlag == TT_EXACT) {
                staticEval = ttScore;
            } else if (ttFlag == TT_BETA && ttScore > staticEval) {
                staticEval = ttScore;
            } else if (ttFlag == TT_ALPHA && ttScore < staticEval) {
                staticEval = ttScore;
            }
        }

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

    if (!inCheck && !isPvNode && depth <= 3 && ply > 0 && abs(alpha) < MATE_SCORE_BOUND) {
        int razorMargin = 200 + 150 * depth;
        if (staticEval + razorMargin < alpha) {
            if (depth == 1) {
                return quiescence(alpha, beta, ply);
            }
            int razorScore = quiescence(alpha - 1, alpha, ply);
            if (razorScore < alpha) {
                return razorScore;
            }
        }
    }

    if (!inCheck && !isPvNode && depth <= 6 && ply > 0 && abs(beta) < MATE_SCORE_BOUND && abs(staticEval) < MATE_SCORE_BOUND) {

        int evalMargin = 100 * depth;

        if (improving) {
            evalMargin -= 75;
        }

        if (staticEval - evalMargin >= beta) {
            return (staticEval + beta) / 2;
        }
    }

    if (!inCheck && !isPvNode && depth >= 2 && ply > 0 && abs(beta) < MATE_SCORE_BOUND) {
        if (staticEval >= beta && board.HasNonPawnMaterial(board.getSideToMove())) {
            int R = (823 + 67 * depth) / 256 + std::min((staticEval - beta) / 200, 3);

            nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
            nnue_state[ply + 1].accumulator.computedAccumulation = 0;

            board.MakeNullMove();
            int score = -negamax(std::max(depth - 1 - R, 0), -beta, -beta + 1, ply + 1, Move(), Move());
            board.UndoNullMove();

            if (isStopped()) return alpha;
            if (score >= beta) return beta;
        }
    }

    Move currentKillers[2] = { Move(), Move() };

    if (ply < MAX_KILLER_HISTORY) {
        currentKillers[0] = killerMoves[ply][0];
        currentKillers[1] = killerMoves[ply][1];
    }

    Move counterMoveHint = Move();
    if (prevMove.isValid()) {
        counterMoveHint = counterMoves[prevMove.getPieceType()][prevMove.getTo()];
    }

    int scores[MoveOrdering::SCORE_SIZE];
    if (ply == 0) {
        for (int i = 0; i < moves.count; i++) {
            scores[i] = 1000000 - i;
        }
    } else {
        MoveOrdering::ScoreMoves(board, moves, ttMove, historyMoves, currentKillers, counterMoveHint, scores);
    }

    Move bestMoveThisNode;
    int bestScore = -MATE_SCORE - 1; // fail-soft best; below any real score
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

        if (isExclusionSearch && m == excludedMove) continue;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        MoveFlag flags = m.getFlags();
        PieceType mPieceType = m.getPieceType();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool quiet = !isCapture && !isPromo;
        bool isAdvancedPassedPawnPush = (mPieceType == PAWN) && board.isAdvancedPassedPawnPush(m);
        bool isKiller = (ply < MAX_KILLER_HISTORY) && (m == killerMoves[ply][0] || m == killerMoves[ply][1]);
        const bool isTTMove = ttMove.isValid() && (m == ttMove);

        // SEE must be computed before MakeMove: it assumes the move has not been played yet
        const bool needSee = isCapture || (!inCheck && !isPvNode && !isKiller && !isTTMove && depth <= 7);
        const int seeScore = needSee ? MoveOrdering::See(board, m) : 0;

        SetupNnueDirtyPieces(m, ply);

        if (!board.MakeMove(m, true)) {
            continue;
        }

        movesSearched++;
        int score = 0;

        if (movesSearched == 1)
            bestMoveThisNode = m;

        bool givesCheck = board.isSquareAttacked(board.getKingSquare(enemy), player);

        if (!inCheck && !isPvNode && movesSearched > 1 && !givesCheck && !isTTMove) {

            if (quiet && !isKiller && depth <= 8) {
                int lmpThreshold = (improving ? 4 : 2) + depth * depth;
                if (movesSearched > lmpThreshold) {
                    board.UndoMove(m, true);
                    continue;
                }
            }

            if (abs(alpha) < MATE_SCORE_BOUND && abs(beta) < MATE_SCORE_BOUND) {

                if (quiet && depth <= 6) {
                    int futilityMargin = 120 * depth;

                    if (staticEval + futilityMargin <= alpha) {
                        board.UndoMove(m, true);
                        continue;
                    }
                }

                if (!isKiller && depth <= 7) {
                    int seeMargin = quiet ? -50 * depth : -90 * depth;
                    if (seeScore < seeMargin) {
                        board.UndoMove(m, true);
                        continue;
                    }
                }
            }
        }

        uint64_t hash_after_move = board.getHash();
        tt->Prefetch(hash_after_move);

        int extension = 0;

        const bool canSingular =
            isTTMove
            && !isExclusionSearch
            && ply > 0
            && ply < rootDepth // caps how far singular extensions can push a line
            && depth >= 8
            && !inCheck
            && ttDepth >= depth - 3
            && ttFlag == TT_BETA
            && std::abs(ttScore) < MATE_SCORE_BOUND;

        if (canSingular) {
            const int singularMargin = 3 + depth / 2;
            const int singularBeta   = ttScore - singularMargin;
            const int singularDepth  = (depth - 1) / 2;

            const int savedPvLength = pvLength[ply];
            Move savedPvTable[MAXIMUM_DEPTH];
            for (int k = 0; k < savedPvLength; k++) savedPvTable[k] = pvTable[ply][k];

            board.UndoMove(m, true);

            int singularScore = negamax(singularDepth,
                                        singularBeta - 1, singularBeta,
                                        ply, m, prevMove);

            pvLength[ply] = savedPvLength;
            for (int k = 0; k < savedPvLength; k++) pvTable[ply][k] = savedPvTable[k];

            SetupNnueDirtyPieces(m, ply);
            if (!board.MakeMove(m, true)) {
                movesSearched--;
                continue;
            }

            if (isStopped()) {
                board.UndoMove(m, true);
                return alpha;
            }

            if (singularScore < singularBeta) {
                extension = 1;
            }
            else if (singularBeta >= beta) {
                // not stored in the TT: singularBeta comes from a reduced-depth search
                board.UndoMove(m, true);
                return singularBeta;
            }
            else if (ttScore >= beta) {
                extension = -1;
            }
        }

        // if (isSingleReply) {
        //     extension = std::max(extension, 1);
        // }
        // else if (givesCheck && depth <= 10 && (isPvNode || movesSearched <= 2)) {
        //     extension = std::max(extension, 1);
        // }

        if (movesSearched == 1) {
            score = -negamax(depth + extension - 1, -beta, -alpha, ply + 1, Move(), m);
        }
        else {
            int reduction = 0;

            if (depth >= 3) {
                reduction = LMR::LMR::GetReduction(depth, movesSearched, isPvNode);

                if (quiet) {
                    if (!improving) reduction += 1;

                    if (isKiller) reduction -= 1;
                    if (isAdvancedPassedPawnPush) reduction -= 1;

                    int histScore = historyMoves[player][mFrom][mTo];
                    int historyModifier = -(histScore / 2048);
                    historyModifier = std::clamp(historyModifier, -2, 2);
                    reduction += historyModifier;
                }
                else if (isCapture) {
                    if (seeScore >= 0) {
                        reduction = 0;
                    } else {
                        reduction += 1;

                        if (seeScore < -300) {
                            reduction += 1;
                        }
                    }
                }

                if (reduction > 0) {
                    if (givesCheck) reduction -= 1;
                    if (inCheck) reduction -= 1;
                    if (isPvNode) reduction -= 1;
                    if (isPromo) reduction = 0;
                }

                reduction = std::clamp(reduction, 0, depth - 2);
            }

            const int fullDepth = depth + extension - 1;

            score = -negamax(fullDepth - reduction, -alpha - 1, -alpha, ply + 1, Move(), m);

            if (score > alpha) {

                if (reduction > 0) {
                    score = -negamax(fullDepth, -alpha - 1, -alpha, ply + 1, Move(), m);
                }

                if (score > alpha && score < beta) {
                    score = -negamax(fullDepth, -beta, -alpha, ply + 1, Move(), m);
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

                if (prevMove.isValid()) {
                    counterMoves[prevMove.getPieceType()][prevMove.getTo()] = m;
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
            // singular verification searches use a modified key that is never probed again
            if (!isExclusionSearch) tt->Store(hash, score, ply, depth, TT_BETA, m);
            return score;
        }

        if (score > bestScore) bestScore = score;
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
        if (isExclusionSearch) {
            return alpha;
        }
        return inCheck ? -MATE_SCORE + ply : 0;
    }

    // every searched move was pruned: fall back to alpha as the bound
    if (bestScore < -MATE_SCORE) bestScore = alpha;

    TTFlag flag = (bestScore <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    if (!isExclusionSearch) tt->Store(hash, bestScore, ply, depth, flag, bestMoveThisNode);

    return bestScore;
}

void Searcher::PrepareSearcher() {
    startTime = now_ms();
    stop = false;
    isStoppedManually = false;
    isSearching = true;
    nodes = 0;
    localNodes = 0;

    for (int i = 0; i < MAXIMUM_DEPTH; i++) {
        pvLength[i] = 0;
    }

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

void Searcher::ClearCounterMoves() {
    for (int p = 0; p < PIECE_TYPE_COUNT; p++) {
        for (int sq = 0; sq < SQUARE_COUNT; sq++) {
            counterMoves[p][sq] = Move();
        }
    }
}

// void Searcher::AgeHistory() {
//    for (int c = 0; c < 2; c++)
//        for (int f = 0; f < SQUARE_COUNT; f++)
//            for (int t = 0; t < SQUARE_COUNT; t++)
//                historyMoves[c][f][t] /= 2;
// }

void Searcher::ClearHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < SQUARE_COUNT; f++)
            for (int t = 0; t < SQUARE_COUNT; t++)
                historyMoves[c][f][t] = 0;
}

std::string Searcher::BuildPvString() {
    constexpr int MAX_PV_PRINT = 64;

    std::vector<Move> played;
    std::vector<uint64_t> seen{ board.getHash() };
    std::string pv;

    auto tryPlay = [&](const Move& candidate) {
        MoveList legal;
        MoveGenerator::GenerateMoves(board, legal);
        for (const Move& m : legal) {
            if (!(m == candidate)) continue;
            if (!board.MakeMove(m, true)) return false;
            played.push_back(m);
            pv += UCIParsing::MoveToUCI(m) + " ";
            return true;
        }
        return false;
    };

    bool pvComplete = true;
    for (int i = 0; i < pvLength[0]; i++) {
        if (!tryPlay(pvTable[0][i])) { pvComplete = false; break; }
        seen.push_back(board.getHash());
    }

    while (pvComplete && (int)played.size() < MAX_PV_PRINT) {
        Move ttMove = tt->getPvMove(board.getHash());
        if (!ttMove.isValid() || !tryPlay(ttMove)) break;
        const uint64_t h = board.getHash();
        if (std::find(seen.begin(), seen.end(), h) != seen.end()) break;
        seen.push_back(h);
    }

    for (int i = (int)played.size() - 1; i >= 0; i--) {
        board.UndoMove(played[i], true);
    }
    return pv;
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
            rootMoves.push_back({m, SCORE_UNSEARCHED, SCORE_UNSEARCHED});
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
        rootDepth = depth;

        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;
        int delta = 25;

        if (depth >= 4 && std::abs(scoreHistory[depth - 1]) < MATE_SCORE_BOUND) {
            int expectedScore = scoreHistory[depth - 1];
            delta = 25 + std::abs(expectedScore) / 8; // large scores swing more between iterations
            alpha = std::max(-MATE_SCORE, expectedScore - delta);
            beta = std::min(MATE_SCORE, expectedScore + delta);
        }

        int score = 0;

        for (int failCount = 0; ; ++failCount) {

            for (RootMove& rm : rootMoves) {
                rm.score = SCORE_UNSEARCHED;
            }

            score = negamax(depth, alpha, beta, 0);

            if (isStopped()) break;
            if (score > alpha && score < beta) break;

            if (failCount >= 8) {
                alpha = -MATE_SCORE;
                beta = MATE_SCORE;
                for (RootMove& rm : rootMoves) {
                    rm.score = SCORE_UNSEARCHED;
                }
                score = negamax(depth, alpha, beta, 0);
                break;
            }

            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(-MATE_SCORE, score - delta);
            }
            else {
                beta = std::min(MATE_SCORE, score + delta);
            }

            delta += delta;
        }

        if (isStopped()) break;

        std::stable_sort(rootMoves.begin(), rootMoves.end(), [](const RootMove& a, const RootMove& b) {
            if (a.score != b.score) return a.score > b.score;
            return a.prevScore > b.prevScore;
        });

        for (RootMove& rm : rootMoves) {
            rm.prevScore = rm.score;
        }

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

        // a drop only matters while the position is still undecided: going from +2000 to +1900
        // (or from -900 to -1000) changes nothing, and must not burn extra time
        bool inCrisis = (depth > 3 && score < prevScore - 50 && std::abs(prevScore) < CRISIS_SCORE_LIMIT);
        long long timeSpent = now_ms() - startTime;

        std::string currentPvString = BuildPvString();

        if (pvLength[0] >= 2) {
            bestPvStringSoFar = currentPvString;
        }
        else if (pvLength[0] < 2 && currentBest == previousBestMove && bestPvStringSoFar != "") {
            currentPvString = bestPvStringSoFar;
        }

        if (!silent) LOG_DEBUG("info depth " << depth << " score "
                      << ((abs(score) > MATE_SCORE_BOUND)
                              ? "mate " + std::to_string((score > 0) ? (MATE_SCORE + 1 - score) / 2 : (MATE_SCORE + 1 + score) / 2)
                              : "cp " + std::to_string(board.getSideToMove() == WHITE ? score : -score))
                      << " time " << timeSpent
                      << " nodes " << (nodes + localNodes)
                                    << " | pv " << currentPvString);

        if (score >= MATE_SCORE_BOUND) break;

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

        std::string currentPvString = BuildPvString();

        if (pvLength[0] == 0 || currentPvString.empty() || isStopped()) {
            currentPvString = bestPvStringSoFar;
        }

        bool isMate = std::abs(lastScore) > MATE_SCORE_BOUND;

        LOG_DEBUG("Final Score: ");

        if (isMate) {
            int mateIn = (lastScore > 0) ? (MATE_SCORE + 1 - lastScore) / 2 : (MATE_SCORE + 1 + lastScore) / 2;
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
    MoveOrdering::ScoreMoves(board, moves, Move(), historyMoves, dummyKillers, Move(), scores);

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
    bool anyDepthCompleted = false;

    for (int d = 1; d <= targetDepth; d++) {
        rootDepth = d;
        std::vector<ScoredMove> currentDepthScores;
        bool depthFinished = true;
        Color us = board.getSideToMove();

        for (auto& sm : lastCompletedScores) {
            SetupNnueDirtyPieces(sm.m, 0);

            if (!board.MakeMove(sm.m, true)) continue;
            int score = -negamax(d - 1, -MATE_SCORE, MATE_SCORE, 1, Move(), sm.m);
            board.UndoMove(sm.m, true);

            if (isStopped()) {
                depthFinished = false;
                break;
            }

            int realScore = us == WHITE ? score : -score;

            currentDepthScores.push_back({sm.m, realScore});
        }

        if (depthFinished && currentDepthScores.size() == lastCompletedScores.size()) {
            std::sort(currentDepthScores.begin(), currentDepthScores.end(),
                      [us](const ScoredMove& a, const ScoredMove& b) { return us == WHITE ? a.score > b.score : a.score < b.score; });
            lastCompletedScores = currentDepthScores;
            anyDepthCompleted = true;
            if (std::abs(lastCompletedScores[0].score) > MATE_SCORE_BOUND) break;
        } else {
            break;
        }

        if (now_ms() - startTime >= softTimeLimit) {
            break;
        }
    }

    if (isStoppedManually) {
        isSearching = false;
        return Move();
    }

    if (!anyDepthCompleted) {
        if (board.isDebugMode) {
            LOG_DEBUG("info string [FALLBACK] No depth completed, using full search.");
        }
        movesWithoutBlunderOnPropuse++;
        return IterativeDeepening();
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
                        LOG_DEBUG("info string [FREE PIECE] Taking undefended piece: " << m.toAlgebraic()
                        << " (rank " << (i + 1) << " | score " << lastCompletedScores[i].score << ")");
                    }
                    movesWithoutBlunderOnPropuse = 0;
                    return m;
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

                if (board.MakeMove(candidateMove, true)) {
                    MoveList enemyCaptures;
                    MoveGenerator::GenerateMoves(board, enemyCaptures, true);

                    for (const auto& enemyMove : enemyCaptures) {
                        if (enemyMove.getFlags() == EN_PASSANT) continue;

                        PieceType myCapturedPiece = board.getPieceAt(enemyMove.getTo(), us);
                        PieceType enemyAttackingPiece = enemyMove.getPieceType();

                        if (myCapturedPiece >= KNIGHT && myCapturedPiece <= QUEEN) {
                            if (!board.MakeMove(enemyMove, true)) continue;
                            board.UndoMove(enemyMove, true);

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
    if (currentSettings.areBlundersOnPurposeEnabled) {
        return GetBestAmongTopMoves(currentSettings);
    }

    return GetMultiThreadedBestMove();
}

Move Searcher::GetMultiThreadedBestMove() {
    int numThreads = this->threads;
    if (numThreads < 1) numThreads = 1;

    tt->NewWrite();
    // AgeHistory();

    std::vector<std::thread> helpers;

    this->stop = false;
    Board board_snapshot = this->board;

    std::vector<std::unique_ptr<Searcher>> tmpHelpers;

    for (int i = 0; i < numThreads - 1; ++i) {
        auto helper = std::make_unique<Searcher>(this->tt);
        helper->isHelper = true;
        helper->abortPtr = &this->stop;
        helper->board = board_snapshot;

        helper->currentSettings  = this->currentSettings;
        helper->currentDiff      = this->currentDiff;
        helper->rtum             = this->rtum;
        helper->fixedTimePerMoveMs = this->fixedTimePerMoveMs.load();

        helper->setTournamentTime(this->timeLeftMs, this->incrementMs);

        tmpHelpers.push_back(std::move(helper));
    }

    for (int i = 0; i < numThreads - 1; ++i) {
        helpers.emplace_back([i, &tmpHelpers]() {
            tmpHelpers[i]->IterativeDeepening(true);
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
    ClearCounterMoves();
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

