#pragma once
#include "Board.h"

namespace OldEvaluation {

    static const int kingsSafetyTable[] = {
        0,  0,   5,  10,  15,
        25, 40,  60,  90, 130,
        180, 250, 330, 450, 600
    };

    static const int KnightMobilityBonus[] = {
        -20, -10, -5, 0, 5, 10, 15, 20, 25
    };

    static const int BishopMobilityBonus[] = {
        -25, -15, -5,
        0,   5, 10,
        15,  20, 25, 30, 35, 40, 45, 50
    };

    static const int RookMobilityBonus[] = {
        -20, -15, -10, -5,
        0,   5,  10, 15,
        20,  25,  30, 35, 40, 45, 50
    };

    static const int QueenMobilityBonus[] = {
        -10, -5,
        0, 0, 0, 5, 5, 10, 10,
        15, 15, 20, 20, 25, 25, 30, 30, 35, 35,
        40, 40, 40, 40, 40, 40, 40, 40
    };

    static const int pawn_pst[64] = {
         0,0,0,0,0,0,0,0,
         5,10,10,-20,-20,10,10,5,
         5,-5,-10,0,0,-10,-5,5,
         0,0,0,20,20,0,0,0,
         5,5,10,25,25,10,5,5,
         10,10,20,30,30,20,10,10,
         50,50,50,50,50,50,50,50,
         0,0,0,0,0,0,0,0
    };

    static const int pawn_pst_eg[64] = {
         0,0,0,0,0,0,0,0,
         10,10,10,10,10,10,10,10,
         20,20,20,20,20,20,20,20,
         30,30,30,30,30,30,30,30,
         40,40,40,40,40,40,40,40,
         60,60,60,60,60,60,60,60,
         80,80,80,80,80,80,80,80,
         0,0,0,0,0,0,0,0
    };

    static const int knight_pst[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,0,5,5,0,-20,-40,
        -30,5,10,15,15,10,5,-30,
        -30,0,15,20,20,15,0,-30,
        -30,5,15,20,20,15,5,-30,
        -30,0,10,15,15,10,0,-30,
        -40,-20,0,0,0,0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };

    static const int bishop_pst[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,5,0,0,0,0,5,-10,
        -10,10,10,10,10,10,10,-10,
        -10,0,10,10,10,10,0,-10,
        -10,5,5,10,10,5,5,-10,
        -10,0,5,10,10,5,0,-10,
        -10,0,0,0,0,0,0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };

    static const int rook_pst[64] = {
         0,0,0,5,5,0,0,0,
        -5,0,0,0,0,0,0,-5,
        -5,0,0,0,0,0,0,-5,
        -5,0,0,0,0,0,0,-5,
        -5,0,0,0,0,0,0,-5,
        -5,0,0,0,0,0,0,-5,
         5,10,10,10,10,10,10,5,
         0,0,0,0,0,0,0,0
    };

    static const int queen_pst[64] = {
        -20,-10,-10,-5,-5,-10,-10,-20,
        -10,0,5,0,0,0,0,-10,
        -10,5,5,5,5,5,0,-10,
         0,0,5,5,5,5,0,-5,
        -5,0,5,5,5,5,0,-5,
        -10,0,5,5,5,5,0,-10,
        -10,0,0,0,0,0,0,-10,
        -20,-10,-10,-5,-5,-10,-10,-20
    };

    static const int king_pst[64] = {
         20,30,10,0,0,10,30,20,
         20,20,0,0,0,0,20,20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30
    };

    static const int king_pst_eg[64] = {
        -50,-40,-30,-20,-20,-30,-40,-50,
        -30,-20,-10,0,0,-10,-20,-30,
        -30,-10,20,30,30,20,-10,-30,
        -30,-10,30,40,40,30,-10,-30,
        -30,-10,30,40,40,30,-10,-30,
        -30,-10,20,30,30,20,-10,-30,
        -30,-30,0,0,0,0,-30,-30,
        -50,-30,-30,-30,-30,-30,-30,-50
    };

    class Evaluation {
    public:
        static int GetPieceValue(PieceType p);
        static int EvaluateMobility(const Board& board, Color color);
        static int EvaluatePos(const Board& board);
        static int EvaluatePawns(const Board& board, Color color);
        static int EvaluatePawnCenter(const Board& board, Color color);
        static int KingPawnShield(const Board& board, Color color);
        static int EvaluateInvasion(const Board& board, Color color);
        static int RookBlockPenalty(const Board& board, Color color);
        static int MopUpEval(const Board& board, Color winner);
        static int EvaluateKingSafety(const Board& board, Color color);
        static int EvaluatePawnTerritory(const Board& board, Color color);
        static void CalculateImbalancePenalty(const Board& board, Color c, int pieceCounts[2][6], int& midGameScore, int& endGameScore);
    };
}
