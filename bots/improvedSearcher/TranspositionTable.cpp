#include "TranspositionTable.h"
#include <cstring>
#include <bit>
#include <algorithm>
#include "Searcher.h"

using namespace ImprovedTT;
using namespace ImprovedSearcher;

TranspositionTable::TranspositionTable(size_t mb) {
    size_t sizeInBytes = mb * 1024 * 1024;
    size_t clusterCount = sizeInBytes / sizeof(TTCluster);

    size = std::bit_ceil(clusterCount);

    table.resize(size);
    Clear();
}

void TranspositionTable::Prefetch(uint64_t hash) {
    size_t index = hash & (size - 1);
    _mm_prefetch(reinterpret_cast<const char*>(&table[index]), _MM_HINT_T0);
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
    depth = std::max(depth, 0);

    size_t index = hash & (size - 1);
    std::lock_guard<SpinLock> lock(ttLocks[index % NUM_LOCKS]);
    TTCluster& cluster = table[index];

    int replaceIndex = -1;
    int worstScore = INT_MIN;
    int writeScore = ScoreToTT(score, ply);

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry& e = cluster.entry[i];

        if (e.key == hash) {
            replaceIndex = i;
            break;
        }

        if (e.key == 0) {
            replaceIndex = i;
            break;
        }

        int badness = ((e.gen != generation) * 1000) - e.depth;

        if (badness > worstScore) {
            worstScore = badness;
            replaceIndex = i;
        }
    }

    TTEntry& e = cluster.entry[replaceIndex];

    if (e.key == hash) {
        e.gen = generation;

        if (bestMove.isValid()) {
            e.moveData = bestMove.getMoveData();
            e.movePieceType = bestMove.getPieceType();
        }

        if (depth >= e.depth) {
            e.score = (int16_t)writeScore;
            e.depth = (int8_t)depth;
            e.type = (uint8_t)flag;
        }

        return;
    }

    e.key = hash;
    e.score = (int16_t)writeScore;
    e.depth = (int8_t)depth;
    e.type = (uint8_t)flag;
    e.gen = generation;

    if (bestMove.isValid()) {
        e.moveData = bestMove.getMoveData();
        e.movePieceType = bestMove.getPieceType();
    } else {
        e.moveData = 0;
        e.movePieceType = 0;
    }
}

bool TranspositionTable::Probe(uint64_t hash, int ply, int depth, int alpha, int beta, int& score, Move& bestMove) {
    size_t index = hash & (size - 1);

    std::lock_guard<SpinLock> lock(ttLocks[index % NUM_LOCKS]);

    TTCluster& cluster = table[index];

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry& e = cluster.entry[i];

        if (e.key == hash) {

            if (e.moveData != 0) {
                bestMove = Move(e.moveData, e.movePieceType);
            }
            else {
                bestMove = Move();
            }

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
    }

    return false;
}

void TranspositionTable::Clear() {
    std::memset(table.data(), 0, table.size() * sizeof(TTCluster));
    generation = 0;
}
