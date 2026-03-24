#include "WorseEvaluation.h"

using namespace WorseEvaluation;

const int kingPawnShieldScores[] = { 5, 8, 5, 3, 6, 3 };

static const int knightWeight = 10;
static const int bishopWeight = 10;
static const int rookWeight = 20;
static const int queenWeight = 45;
static const int endgameStart = 2 * knightWeight + 2 * bishopWeight + 2 * rookWeight + queenWeight;

static const uint64_t whiteTerritoryMask = (RANK_1 | RANK_2 | RANK_3 | RANK_4);
static const uint64_t blackTerritoryMask = (RANK_5 | RANK_6 | RANK_7 | RANK_8);

static const uint64_t CENTRAL_FILES = FILE_C | FILE_D | FILE_E | FILE_F;

static const uint64_t CENTER_4 = (RANK_4 | RANK_5) & (FILE_E | FILE_D);
static const uint64_t CENTER_4_SIDE = (RANK_4 | RANK_5) & (FILE_C  | FILE_F);
static const uint64_t CENTER_16_SIDE = (RANK_3 | RANK_6) & (FILE_C | FILE_D | FILE_E | FILE_F);

int Evaluation::GetPieceValue(PieceType p) {
    switch (p) {
    case PAWN:   return 100;
    case KNIGHT: return 320;
    case BISHOP: return 330;
    case ROOK:   return 500;
    case QUEEN:  return 900;
    case KING:   return 10000;
    default:     return 0;
    }
}

int Evaluation::GetGamePhase(const Board& board) {
    int phase = 0;

    phase += std::popcount(board.getPieceBitboard(WHITE, KNIGHT)) * knightWeight;
    phase += std::popcount(board.getPieceBitboard(WHITE, BISHOP)) * bishopWeight;
    phase += std::popcount(board.getPieceBitboard(WHITE, ROOK))   * rookWeight;
    phase += std::popcount(board.getPieceBitboard(WHITE, QUEEN))  * queenWeight;

    phase += std::popcount(board.getPieceBitboard(BLACK, KNIGHT)) * knightWeight;
    phase += std::popcount(board.getPieceBitboard(BLACK, BISHOP)) * bishopWeight;
    phase += std::popcount(board.getPieceBitboard(BLACK, ROOK))   * rookWeight;
    phase += std::popcount(board.getPieceBitboard(BLACK, QUEEN))  * queenWeight;

    return phase;
}

int Evaluation::EvaluateMobility(const Board& board, Color color) {
    int score = 0;
    uint64_t occ = board.getAllOccupancy();
    uint64_t myPieces = board.getSideOccupancy(color);

    uint64_t knights = board.getPieceBitboard(color, KNIGHT);
    while (knights) {
        Square sq = PopBit(knights);
        uint64_t attacks = board.getKnightAttacks(sq);
        int moves = std::popcount(attacks & ~myPieces);
        score += KnightMobilityBonus[moves];
    }

    uint64_t bishops = board.getPieceBitboard(color, BISHOP);
    while (bishops) {
        Square sq = PopBit(bishops);
        uint64_t attacks = board.getBishopAttacks(sq, occ);
        int moves = std::popcount(attacks & ~myPieces);
        if (moves > 13) moves = 13;
        score += BishopMobilityBonus[moves];
    }

    uint64_t rooks = board.getPieceBitboard(color, ROOK);
    while (rooks) {
        Square sq = PopBit(rooks);
        uint64_t attacks = board.getRookAttacks(sq, occ);
        int moves = std::popcount(attacks & ~myPieces);
        if (moves > 14) moves = 14;
        score += RookMobilityBonus[moves];
    }

    uint64_t queens = board.getPieceBitboard(color, QUEEN);
    while (queens) {
        Square sq = PopBit(queens);
        uint64_t attacks = board.getBishopAttacks(sq, occ) | board.getRookAttacks(sq, occ);
        int moves = std::popcount(attacks & ~myPieces);
        if (moves > 26) moves = 26;
        score += QueenMobilityBonus[moves];
    }

    return score;
}


int Evaluation::EvaluatePawnTerritory(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    int bonus = 0;
    uint64_t enemyTerritoryMask = (color == WHITE) ? blackTerritoryMask : whiteTerritoryMask;

    uint64_t bb = pawns;
    while (bb) {
        Square sq = PopBit(bb);
        int rank = sq >> 3;
        int file = sq & 7;

        uint64_t adjacentFiles = 0;
        if (file > 0) adjacentFiles |= FILE_MASKS[file - 1];
        if (file < 7) adjacentFiles |= FILE_MASKS[file + 1];

        if ((pawns & adjacentFiles & RANK_MASKS[rank]) &&
            (enemyTerritoryMask & RANK_MASKS[rank])) {
            bonus += 7;
        }
        int relativeRank = (color == WHITE) ? rank : 7 - rank;

        bool inEnemyTerritory = relativeRank >= 4;
        if (inEnemyTerritory) {
            bonus += 10;
            if (file >= 2 && file <= 5)
                bonus += 3;
        }
    }
    return bonus;
}

int Evaluation::EvaluatePawnCenter(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    int score = 0;

    score += 15 * std::popcount(pawns & CENTER_4);
    score += 10 * std::popcount(pawns & CENTER_4_SIDE);
    score += 5 * std::popcount(pawns & CENTER_16_SIDE);

    return score;
}

void Evaluation::EvaluatePawns(const Board& board, Color color, int& mgScore, int& egScore) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    uint64_t enemyPawns = board.getPieceBitboard((Color)(color ^ 1), PAWN);
    int isolated = 0;

    for (int file = 0; file < 8; file++) {
        int count = std::popcount(pawns & FILE_MASKS[file]);
        if (count > 1) {
            egScore -= 15 * (count - 1);
        }
    }

    uint64_t bb = pawns;
    while (bb) {
        Square sq = PopBit(bb);
        int file = sq & 7;
        int rank = sq >> 3;

        uint64_t adjacentFilesMask = 0;
        if (file > 0) adjacentFilesMask |= FILE_MASKS[file - 1];
        if (file < 7) adjacentFilesMask |= FILE_MASKS[file + 1];

        if (!(pawns & adjacentFilesMask)) {
            isolated++;
        }

        uint64_t pathMask = FILE_MASKS[file] | adjacentFilesMask;
        uint64_t forwardMask = 0;
        if (color == WHITE) {
            for (int r = rank + 1; r < 8; r++) forwardMask |= RANK_MASKS[r];
        }
        else {
            for (int r = rank - 1; r >= 0; r--) forwardMask |= RANK_MASKS[r];
        }

        if (!(enemyPawns & pathMask & forwardMask)) {
            int relativeRank = (color == WHITE) ? rank : 7 - rank;
            int idx = std::clamp(relativeRank, 0, 7);

            mgScore += passedPawnBonusesMG[idx];
            egScore += passedPawnBonusesEG[idx];
        }
    }

    int isoIdx = std::clamp(isolated, 0, 8);
    mgScore += isolatedPawnPenaltyMG[isoIdx];
    egScore += isolatedPawnPenaltyEG[isoIdx];
}

int Evaluation::EvaluateKingSafety(const Board& board, Color color) {
    Color enemy = (Color)(color ^ 1);

    if (board.getPieceBitboard(enemy, QUEEN) == 0 && board.getPieceBitboard(enemy, ROOK) == 0)
        return 0;

    Square kingSq = board.getKingSquare(color);
    uint64_t zone = board.getKingAttacks(kingSq) | (1ULL << kingSq);

    int attackUnits = 0;
    int attackingPiecesCount = 0;

    const int pieceWeightValue[] = { 0, 2, 2, 3, 5, 0 };

    uint64_t occ = board.getAllOccupancy();

    for (int pt = KNIGHT; pt <= QUEEN; pt++) {
        uint64_t bb = board.getPieceBitboard(enemy, (PieceType)pt);
        while (bb) {
            Square sq = PopBit(bb);
            uint64_t attacks = 0;

            if (pt == KNIGHT) attacks = board.getKnightAttacks(sq);
            else if (pt == BISHOP) attacks = board.getBishopAttacks(sq, occ);
            else if (pt == ROOK) attacks = board.getRookAttacks(sq, occ);
            else if (pt == QUEEN) attacks = (board.getBishopAttacks(sq, occ) | board.getRookAttacks(sq, occ));

            uint64_t hits = attacks & zone;
            if (hits) {
                attackingPiecesCount++;
                attackUnits += pieceWeightValue[pt] + (std::popcount(hits) * 2);
            }
        }
    }

    if (attackingPiecesCount < 2) return 0;

    return -kingsSafetyTable[std::min(attackUnits, 99)];
}

int Evaluation::KingPawnShield(const Board& board, Color color) {
    Square kingSq = board.getKingSquare(color);
    int kFile = kingSq & 7;
    int kRank = kingSq >> 3;

    if (color == WHITE && kRank > 2) return 0;
    if (color == BLACK && kRank < 5) return 0;

    uint64_t myPawns = board.getPieceBitboard(color, PAWN);
    int shieldBonus = 0;

    for (int file = std::max(0, kFile - 1); file <= std::min(7, kFile + 1); file++) {
        uint64_t fileMask = FILE_A << file;

        int idx = file - (kFile - 1);
        if (idx < 0) idx = 0;
        if (idx > 2) idx = 2;

        int r1 = (color == WHITE) ? 1 : 6;
        if (myPawns & fileMask & RANK_MASKS[r1]) {
            shieldBonus += kingPawnShieldScores[idx] * 2;
        }
        else {
            int r2 = (color == WHITE) ? 2 : 5;
            if (myPawns & fileMask & RANK_MASKS[r2]) {
                shieldBonus += kingPawnShieldScores[idx + 3] * 2;
            }
        }
    }
    return shieldBonus;
}

int Evaluation::EvaluateInvasion(const Board& board, Color color) {
    int bonus = 0;

    uint64_t enemyTerritory = (color == WHITE) ? blackTerritoryMask : whiteTerritoryMask;
    uint64_t enemyBackyard = (color == WHITE) ? (RANK_7 | RANK_8) : (RANK_1 | RANK_2);

    for (int pt = KNIGHT; pt <= QUEEN; pt++) {
        uint64_t myPieces = board.getPieceBitboard(color, (PieceType)pt);
        uint64_t invaders = myPieces & enemyTerritory;

        while (invaders) {
            Square sq = PopBit(invaders);
            bonus += 15;
            if ((1ULL << sq) & enemyBackyard) {
                bonus += 20;
            }
        }
    }
    return bonus;
}


int Evaluation::MopUpEval(const Board& board, Color winner) {

    Square k1 = board.getKingSquare(winner);
    Square k2 = board.getKingSquare((Color)(winner ^ 1));

    int dist = abs((k1 >> 3) - (k2 >> 3)) + abs((k1 & 7) - (k2 & 7));
    return (14 - dist) * 10;
}

void Evaluation::CalculateImbalancePenalty(const Board& board, Color c, int pieceCounts[2][6], int& midGameScore, int& endGameScore) {
    int opp = c ^ 1;

    if (pieceCounts[c][BISHOP] >= 2) {
        midGameScore += 30;
        endGameScore += 50;
    }

    int myPawns = pieceCounts[c][PAWN];
    int oppPawns = pieceCounts[opp][PAWN];
    int myMinors = pieceCounts[c][KNIGHT] + pieceCounts[c][BISHOP];
    int oppMinors = pieceCounts[opp][KNIGHT] + pieceCounts[opp][BISHOP];
    int myRooks = pieceCounts[c][ROOK];
    int oppRooks = pieceCounts[opp][ROOK];
    int myQueens = pieceCounts[c][QUEEN];
    int oppQueens = pieceCounts[opp][QUEEN];

    if (myRooks > oppRooks && oppMinors >= myMinors + 2) {
        midGameScore -= 40;
        endGameScore -= 80;
    }

    if (myQueens > oppQueens && oppRooks >= myRooks + 2) {
        midGameScore -= 30;
        endGameScore -= 60;
    }

    if (myQueens > oppQueens && oppMinors >= myMinors + 3) {
        midGameScore -= 40;
        endGameScore -= 80;
    }

    if (oppMinors > myMinors && myPawns - oppPawns >= 3) {
        midGameScore -= 60;
        endGameScore -= 80;
    }
}

int Evaluation::RookBlockPenalty(const Board& board, Color color) {
    Square kingSq = board.getKingSquare(color);
    int kFile = kingSq & 7;
    int penalty = 0;

    if (kFile <= 2 || kFile >= 5) {
        uint64_t rooks = board.getPieceBitboard(color, ROOK);
        while (rooks) {
            Square rSq = PopBit(rooks);
            int rFile = rSq & 7;

            if ((rFile == 0 && kFile < 3 && kFile > 0) || (rFile == 7 && kFile > 4 && kFile < 7)) {

                uint64_t attacks = board.getRookAttacks(rSq, board.getAllOccupancy());
                if (std::popcount(attacks) <= 3) {
                    penalty += 40;
                }
            }
        }
    }
    return -penalty;
}

bool Evaluation::OppositeColorBishopEndgame(const Board& board, const int pieceCounts[2][6]) {
    if (pieceCounts[WHITE][BISHOP] == 1 && pieceCounts[BLACK][BISHOP] == 1 &&
        pieceCounts[WHITE][KNIGHT] == 0 && pieceCounts[BLACK][KNIGHT] == 0) {

        uint64_t wB = board.getPieceBitboard(WHITE, BISHOP);
        uint64_t bB = board.getPieceBitboard(BLACK, BISHOP);

        if (wB && bB) {
            Square wSq = PopBit(wB);
            Square bSq = PopBit(bB);

            bool wLight = (LIGHT_SQUARES & (1ULL << wSq));
            bool bLight = (LIGHT_SQUARES & (1ULL << bSq));

            if (wLight != bLight) {
                return true;
            }
        }
    }

    return false;
}

bool Evaluation::IsDrawKnightEndgame(const int pieceCounts[2][6]){
    if (pieceCounts[WHITE][PAWN] == 0 && pieceCounts[BLACK][PAWN] == 0) {
        if (pieceCounts[WHITE][ROOK] == 0 && pieceCounts[BLACK][ROOK] == 0 &&
            pieceCounts[WHITE][QUEEN] == 0 && pieceCounts[BLACK][QUEEN] == 0) {

            int wN = pieceCounts[WHITE][KNIGHT];
            int wB = pieceCounts[WHITE][BISHOP];
            int bN = pieceCounts[BLACK][KNIGHT];
            int bB = pieceCounts[BLACK][BISHOP];

            if (wN == 2 && wB == 0 && bN == 0 && bB == 0) return true;
            if (bN == 2 && bB == 0 && wN == 0 && wB == 0) return true;
        }
    }

    return false;
}

bool Evaluation::WrongColoredBishopDrawEndgame(const Board& board, const Color us, const int pieceCounts[2][6]) {

    Color them = (Color)(us ^ 1);

    if (pieceCounts[us][PAWN] >= 1 && pieceCounts[us][BISHOP] == 1 &&
        pieceCounts[them][PAWN] == 0 &&
        pieceCounts[us][ROOK] == 0 && pieceCounts[us][QUEEN] == 0 &&
        pieceCounts[us][KNIGHT] == 0) {

        uint64_t pawns = board.getPieceBitboard(us, PAWN);

        bool allOnFileA = (pawns & ~FILE_MASKS[0]) == 0;

        bool allOnFileH = (pawns & ~FILE_MASKS[7]) == 0;

        if (!allOnFileA && !allOnFileH) {
            return false;
        }

        int file = allOnFileA ? 0 : 7;

        int promRank = (us == WHITE) ? 7 : 0;
        Square promSq = (Square)(promRank * 8 + file);

        bool promIsLight = (LIGHT_SQUARES & (1ULL << promSq));

        uint64_t bishopBB = board.getPieceBitboard(us, BISHOP);
        Square bSq = (Square)std::countr_zero(bishopBB);
        bool bishopIsLight = (LIGHT_SQUARES & (1ULL << bSq));

        if (promIsLight != bishopIsLight) {

            Square enemyKing = board.getKingSquare(them);

            int dist = std::max(abs((enemyKing & 7) - file), abs((enemyKing >> 3) - promRank));

            if (dist <= 2) {
                return true;
            }
        }
    }

    return false;
}

int Evaluation::RookAgainstMinorsEndgame(const int pieceCounts[2][6]) {

    int scaleFactor = 16;

    if (pieceCounts[WHITE][PAWN] == 0 && pieceCounts[BLACK][PAWN] == 0) {

        bool whiteMajor = (pieceCounts[WHITE][ROOK] == 1 && pieceCounts[WHITE][QUEEN] == 0);
        bool blackMajor = (pieceCounts[BLACK][ROOK] == 1 && pieceCounts[BLACK][QUEEN] == 0);

        int wMinors = pieceCounts[WHITE][KNIGHT] + pieceCounts[WHITE][BISHOP];
        int bMinors = pieceCounts[BLACK][KNIGHT] + pieceCounts[BLACK][BISHOP];

        if ((whiteMajor && wMinors == 0 && bMinors == 1 && !blackMajor) ||
            (blackMajor && bMinors == 0 && wMinors == 1 && !whiteMajor)) {

            scaleFactor = 2;
        }
    }

    return scaleFactor;
}

void Evaluation::DrawnEndgamePenalty(const Color us, const int pieceCounts[2][6], int& midGameScore, int& endGameScore) {

    const Color enemy = (Color)(us ^ 1);
    if (pieceCounts[WHITE][PAWN] == 0 && pieceCounts[BLACK][PAWN] == 0) {
        if (pieceCounts[WHITE][QUEEN] == 0 && pieceCounts[BLACK][QUEEN] == 0 &&
        pieceCounts[WHITE][BISHOP] == 0 && pieceCounts[BLACK][BISHOP] == 0 && pieceCounts[us][ROOK] == 1 && pieceCounts[enemy][ROOK] == 1 &&
            pieceCounts[us][KNIGHT] == 1 && pieceCounts[enemy][KNIGHT] == 0){
            midGameScore -= 400;
            endGameScore -= 400;
        }

        if (pieceCounts[us][QUEEN] == 1 && pieceCounts[enemy][QUEEN] == 0 &&
            pieceCounts[WHITE][BISHOP] == 0 && pieceCounts[BLACK][BISHOP] == 0 && pieceCounts[us][ROOK] == 0 && pieceCounts[enemy][ROOK] == 1 &&
            pieceCounts[us][KNIGHT] == 0 && pieceCounts[enemy][KNIGHT] == 0){
            midGameScore -= 400;
            endGameScore -= 400;
        }
    }
}

int Evaluation::EvaluatePos(const Board& board) {
    int mg[2] = { 0, 0 };
    int eg[2] = { 0, 0 };
    int phase = 0;
    int pieceCounts[2][6] = { {0} };

    for (int c = WHITE; c <= BLACK; c++) {
        for (int pt = PAWN; pt <= KING; pt++) {
            uint64_t bb = board.getPieceBitboard((Color)c, (PieceType)pt);
            while (bb) {
                Square sq = PopBit(bb);
                int s = (c == WHITE) ? sq : sq ^ 56;
                pieceCounts[c][pt]++;

                int val = GetPieceValue((PieceType)pt);
                mg[c] += val;
                eg[c] += val;

                if (pt == KNIGHT) phase += knightWeight;
                else if (pt == BISHOP) phase += bishopWeight;
                else if (pt == ROOK) phase += rookWeight;
                else if (pt == QUEEN) phase += queenWeight;

                int mgpst = 0, egpst = 0;
                if (pt == PAWN) { mgpst = pawn_pst[s]; egpst = pawn_pst_eg[s]; }
                else if (pt == KNIGHT) mgpst = egpst = knight_pst[s];
                else if (pt == BISHOP) mgpst = egpst = bishop_pst[s];
                else if (pt == ROOK) mgpst = egpst = rook_pst[s];
                else if (pt == QUEEN) mgpst = egpst = queen_pst[s];
                else { mgpst = king_pst[s]; egpst = king_pst_eg[s]; }

                mg[c] += mgpst;
                eg[c] += egpst;
            }
        }
    }

    if (IsDrawKnightEndgame(pieceCounts)) return 0;
    if (WrongColoredBishopDrawEndgame(board, WHITE, pieceCounts)) return 0;
    if (WrongColoredBishopDrawEndgame(board, BLACK, pieceCounts)) return 0;

    bool oppositeBishopEndgame = OppositeColorBishopEndgame(board, pieceCounts);
    int scaleFactor = RookAgainstMinorsEndgame(pieceCounts);

    float egT = 1.0f - std::min(1.0f, (float)phase / 240.0f);

    for (int c = WHITE; c <= BLACK; c++) {
        Color us = (Color)c;
        Color opp = (Color)(c ^ 1);

        CalculateImbalancePenalty(board, us, pieceCounts, mg[c], eg[c]);
        EvaluatePawns(board, us, mg[c], eg[c]);

        int mobility = EvaluateMobility(board, us);
        mg[c] += mobility;
        eg[c] += (int)(mobility * 1.2f);

        mg[c] += EvaluateInvasion(board, us);
        mg[c] += RookBlockPenalty(board, us);
        mg[c] += (int)(EvaluatePawnTerritory(board, us) * 0.5f);
        mg[c] += EvaluateKingSafety(board, us);
        mg[c] += KingPawnShield(board, us);
        mg[c] += EvaluatePawnCenter(board, us);

        uint64_t myRookBits = board.getPieceBitboard(us, ROOK);
        uint64_t seventhRank = (us == WHITE) ? RANK_7 : RANK_2;
        if (myRookBits & seventhRank) {
            mg[c] += 20;
            eg[c] += 40;
        }

        if (eg[c] > eg[opp] + 250 && pieceCounts[opp][PAWN] <= 2)
            eg[c] += MopUpEval(board, us);

        DrawnEndgamePenalty(us, pieceCounts, mg[c], eg[c]);
    }

    int score = (int)(mg[WHITE] * (1.0f - egT) + eg[WHITE] * egT)
                - (int)(mg[BLACK] * (1.0f - egT) + eg[BLACK] * egT);

    score = (score * scaleFactor) / 16;
    if (oppositeBishopEndgame) score /= 2;

    return board.getSideToMove() == WHITE ? score : -score;
}
