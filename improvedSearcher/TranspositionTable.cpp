#include "TranspositionTable.h"
#include <cstring>
#include "Searcher.h"

using namespace ImprovedTT;
using namespace ImprovedSearcher;

size_t TranspositionTable::NextPowerOf2(size_t n) {
    size_t count = 0;
    if (n && !(n & (n - 1))) return n;
    while (n != 0) { n >>= 1; count += 1; }
    return 1ULL << count;
}

TranspositionTable::TranspositionTable(size_t mb) {
    size_t entryCount = (mb * 1024 * 1024) / sizeof(TTEntry);
    size_t size = NextPowerOf2(entryCount);
    table.resize(size);
    Clear();
}

int TranspositionTable::ScoreToTT(int score, int ply) {
    if (std::abs(score) > Searcher::MATE_SCORE_BOUND) {
        return score > 0 ? score + ply : score - ply;
    }
    return score;
}

int TranspositionTable::ScoreFromTT(int score, int ply) {
    if (std::abs(score) > Searcher::MATE_SCORE_BOUND) {
        return score > 0 ? score - ply : score + ply;
    }
    return score;
}

void TranspositionTable::Store(uint64_t hash, int score, int ply, int depth, TTFlag flag, Move bestMove) {
    size_t index = hash & (table.size() - 1);
    TTEntry& e = table[index];

    int writeScore = ScoreToTT(score, ply);

    if (e.key != hash || e.gen != generation || depth >= e.depth) {

        if (e.key != hash) {
            e.moveData = 0;
            e.movePieceType = 0;
        }

        e.key = hash;
        e.score = (int16_t)writeScore;
        e.depth = (int8_t)depth;
        e.type = (uint8_t)flag;
        e.gen = generation;

        if (bestMove.isValid()) {
            e.moveData = bestMove.getMoveData();
            e.movePieceType = bestMove.getPieceType();
        }
    }
}

bool TranspositionTable::Probe(uint64_t hash, int ply, int depth, int alpha, int beta, int& score, Move& bestMove) {
    size_t index = hash & (table.size() - 1);
    TTEntry& e = table[index];

    if (e.key != hash)
        return false;

    bestMove = Move(e.moveData, e.movePieceType);

    if (e.depth >= depth) {
        int retrievedScore = ScoreFromTT(e.score, ply);

        if (e.type == TT_EXACT) {
            score = retrievedScore;
            return true;
        }

        if (e.type == TT_ALPHA && retrievedScore <= alpha) {
            score = retrievedScore;
            return true;
        }
        if (e.type == TT_BETA && retrievedScore >= beta) {
            score = retrievedScore;
            return true;
        }
    }

    return false;
}

void TranspositionTable::Clear() {
    std::memset(table.data(), 0, table.size() * sizeof(TTEntry));
    generation = 0;
}
