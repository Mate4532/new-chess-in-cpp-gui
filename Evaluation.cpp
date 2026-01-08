#include "Evaluation.h"
#include "PrecomputedEvaluationData.h"
#include "Searcher.h"

const int passedPawnBonuses[] = { 0, 10, 20, 40, 70, 120, 200 };
const int isolatedPawnPenalty[] = { 0, -10, -25, -50, -75, -75, -75, -75, -75 };
const int kingPawnShieldScores[] = { 4, 7, 4, 3, 6, 3 };

static const int knightWeight = 10;
static const int bishopWeight = 10;
static const int rookWeight = 20;
static const int queenWeight = 45;
static const int endgameStart = 2 * knightWeight + 2 * bishopWeight + 2 * rookWeight + queenWeight;

static const uint64_t whiteTerritoryMask = (RANK_1 | RANK_2 | RANK_3 | RANK_4);
static const uint64_t blackTerritoryMask = (RANK_5 | RANK_6 | RANK_7 | RANK_8);

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

int Evaluation::EvaluateMobility(const Board& board, Color color) {
    int score = 0;
    uint64_t occ = board.getAllOccupancy();
    uint64_t myPieces = board.getSideOccupancy(color);
    uint64_t enemyTerritory =
        (color == WHITE) ? blackTerritoryMask : whiteTerritoryMask;

    uint64_t knights = board.getPieceBitboard(color, KNIGHT);
    while (knights) {
        Square sq = PopBit(knights);
        uint64_t a = board.getKnightAttacks(sq);
        int m = std::popcount(a & ~myPieces);
        int t = std::popcount(a & enemyTerritory & ~myPieces);

        score += m * 4;
        score += t * 1;
    }

    uint64_t bishops = board.getPieceBitboard(color, BISHOP);
    while (bishops) {
        Square sq = PopBit(bishops);
        uint64_t a = board.getBishopAttacks(sq, occ);
        int m = std::popcount(a & ~myPieces);
        int t = std::popcount(a & enemyTerritory & ~myPieces);

        score += m * 5;
        score += t * 2;
    }

    uint64_t rooks = board.getPieceBitboard(color, ROOK);
    while (rooks) {
        Square sq = PopBit(rooks);
        uint64_t a = board.getRookAttacks(sq, occ);
        int m = std::popcount(a & ~myPieces);
        int t = std::popcount(a & enemyTerritory & ~myPieces);

        score += m * 3;
        score += t * 1;
    }

    uint64_t queens = board.getPieceBitboard(color, QUEEN);
    while (queens) {
        Square sq = PopBit(queens);
        uint64_t a =
            board.getBishopAttacks(sq, occ) |
            board.getRookAttacks(sq, occ);

        score += std::popcount(a & ~myPieces);
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

        uint64_t diagonalDefenders = board.getPawnAttacks(sq, (Color)(color ^ 1));
        if (diagonalDefenders & pawns) {
            bonus += 4;
        }

        uint64_t adjacentFiles = 0;
        if (file > 0) adjacentFiles |= FILE_MASKS[file - 1];
        if (file < 7) adjacentFiles |= FILE_MASKS[file + 1];

        if ((pawns & adjacentFiles & RANK_MASKS[rank]) &&
            (enemyTerritoryMask & RANK_MASKS[rank])) {
            bonus += 5;
        }
        int relativeRank = (color == WHITE) ? rank : 7 - rank;

        bool inEnemyTerritory = relativeRank >= 4;
        if (inEnemyTerritory) {
            bonus += 6;
            if (file >= 2 && file <= 5) 
                bonus += 3;

            uint64_t attacks = board.getPawnAttacks(sq, color);
            int controlledCount = std::popcount(attacks & enemyTerritoryMask);
            bonus += std::min(controlledCount * 2, 3);
        }
    }
    return bonus;
}
int Evaluation::EvaluatePawnCenter(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    int score = 0;

    score += 12 * std::popcount(pawns & CENTER_4);
	score += 8 * std::popcount(pawns & CENTER_4_SIDE);
    score += 4 * std::popcount(pawns & CENTER_16_SIDE);

    return score;
}

int Evaluation::EvaluatePawns(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    uint64_t enemyPawns = board.getPieceBitboard((Color)(color ^ 1), PAWN);
    int score = 0;
    int isolated = 0;

    for (int file = 0; file < 8; file++) {
        int count = std::popcount(pawns & FILE_MASKS[file]);
        if (count > 1) {
            score -= 15 * (count - 1);
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
            score += passedPawnBonuses[std::clamp(relativeRank, 0, 6)];

            uint64_t defenders = board.getPawnAttacks(sq, (Color)(color ^ 1));
            if (defenders & pawns)
                score += 10;
        }
    }

    score += isolatedPawnPenalty[std::clamp(isolated, 0, 8)];
    return score;
}

int Evaluation::EvaluateKingSafety(const Board& board, Color color) {
    Color enemy = (Color)(color ^ 1);

    if (board.getPieceBitboard(enemy, QUEEN) == 0 && board.getPieceBitboard(enemy, ROOK) == 0) return 0;

    Square kingSq = board.getKingSquare(color);
    uint64_t zone = board.getKingAttacks(kingSq) | (1ULL << kingSq);
    int weight = 0;
    const int w[] = { 0, 1, 1, 2, 3, 0 };

    for (int pt = KNIGHT; pt <= QUEEN; pt++) {
        uint64_t bb = board.getPieceBitboard(enemy, (PieceType)pt);
        while (bb) {
            Square sq = PopBit(bb);
            uint64_t attacks = (pt == KNIGHT) ? board.getKnightAttacks(sq) :
                (pt == BISHOP) ? board.getBishopAttacks(sq, board.getAllOccupancy()) :
                (pt == ROOK) ? board.getRookAttacks(sq, board.getAllOccupancy()) :
                (board.getBishopAttacks(sq, board.getAllOccupancy()) | board.getRookAttacks(sq, board.getAllOccupancy()));

            if (attacks & zone) weight += w[pt];
        }
    }

    static const int table[] = { 0, 0, 10, 20, 40, 80, 130, 190, 260, 350, 460, 580, 720, 900, 1100 };
    return -table[std::min(weight, 14)];
}

int Evaluation::KingPawnShield(const Board& board, Color color) {  

    Square kingSq = board.getKingSquare(color);
    int kFile = kingSq & 7;
    uint64_t myPawns = board.getPieceBitboard(color, PAWN);
    int shieldBonus = 0;

    for (int file = std::max(0, kFile - 1); file <= std::min(7, kFile + 1); file++) {
        uint64_t fileMask = FILE_A << file;
        uint64_t shieldRanks = (color == WHITE) ? (RANK_2 | RANK_3) : (RANK_7 | RANK_6);

        if (myPawns & fileMask & shieldRanks) {
            shieldBonus += 15;
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

    if (oppMinors - myMinors > 0 && myPawns - oppPawns >= 3) {

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
            int rRank = rSq >> 3;

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

int Evaluation::EvaluatePos(const Board& board) {
    int mg[2] = { 0,0 };
    int eg[2] = { 0,0 };
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

    float egT = 1.0f - std::min(1.0f, (float)phase / endgameStart);

    for (int c = WHITE; c <= BLACK; c++) {
        int opp = c ^ 1;

        CalculateImbalancePenalty(board, (Color)c, pieceCounts, mg[c], eg[c]);

        int pScore = EvaluatePawns(board, (Color)c);
        mg[c] += pScore;
        eg[c] += pScore;

		int mobility = EvaluateMobility(board, (Color)c);
        mg[c] += (int)(mobility);
        eg[c] += (int)(mobility * 1.2);

        int invasion = EvaluateInvasion(board, (Color)c);
        mg[c] += invasion;

        mg[c] += RookBlockPenalty(board, (Color)c);

        mg[c] += EvaluatePawnTerritory(board, (Color)c);

        mg[c] += EvaluateKingSafety(board, (Color)c);

        mg[c] += KingPawnShield(board, (Color)c);

        mg[c] += EvaluatePawnCenter(board, (Color)c);

        uint64_t myRookBits = board.getPieceBitboard((Color)c, ROOK);
        uint64_t seventhRank = (c == WHITE) ? RANK_7 : RANK_2;
        if (myRookBits & seventhRank) {
            mg[c] += 20;
            eg[c] += 40;
        }

        if (eg[c] > eg[opp] + 150)
            eg[c] += MopUpEval(board, (Color)c);
    }

    int score = (int)(mg[WHITE] * (1.0f - egT) + eg[WHITE] * egT)
        - (int)(mg[BLACK] * (1.0f - egT) + eg[BLACK] * egT);

    return board.getSideToMove() == WHITE ? score : -score;
}
