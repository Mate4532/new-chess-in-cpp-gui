#include "TranspositionTable.h"
#include <cstring>

using namespace ImprovedTT;

size_t TranspositionTable::NextPowerOf2(size_t n) {
    size_t count = 0;
    if (n && !(n & (n - 1))) return n;
    while (n != 0) { n >>= 1; count += 1; }
    return 1ULL << count;
}

TranspositionTable::TranspositionTable(size_t mb) {
    size_t sizeInBytes = mb * 1024 * 1024;
    size_t clusterCount = sizeInBytes / sizeof(TTCluster);

    size = NextPowerOf2(clusterCount);

    table.resize(size);
    Clear();
}

void TranspositionTable::Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove) {
    size_t index = (hash ^ (hash >> 32)) & (size - 1);
    TTCluster& cluster = table[index];

    int replaceIndex = -1;
    int worstScore = INT_MIN;

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry& e = cluster.entry[i];

        if (e.key == hash) {
            replaceIndex = i;
            break;
        }

        int badness = 0;
        if (e.gen != generation) badness += 1000;
        badness -= e.depth;

        if (badness > worstScore) {
            worstScore = badness;
            replaceIndex = i;
        }
    }

    TTEntry& e = cluster.entry[replaceIndex];

    if (e.key == hash && depth < e.depth) {
        return;
    }

    if (e.key != hash) {
        e.moveData = 0;
        e.pieceType = 0;
    }

    e.key = hash;
    e.score = (int16_t)score;
    e.depth = (int8_t)depth;
    e.type = (uint8_t)flag;
    e.gen = generation;

    if (bestMove.isValid()) {
        e.moveData = bestMove.getMoveData();
        e.pieceType = (uint8_t)bestMove.getPieceType();
    }
}

bool TranspositionTable::Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove) {
    size_t index = (hash ^ (hash >> 32)) & (size - 1);
    TTCluster& cluster = table[index];

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry& e = cluster.entry[i];

        if (e.key == hash) {

            bestMove = Move(e.moveData, e.pieceType);

            if (e.depth >= depth) {
                if (e.type == TT_EXACT) {
                    score = e.score;
                    return true;
                }
                if (e.type == TT_ALPHA && e.score <= alpha) {
                    score = e.score;
                    return true;
                }
                if (e.type == TT_BETA && e.score >= beta) {
                    score = e.score;
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
