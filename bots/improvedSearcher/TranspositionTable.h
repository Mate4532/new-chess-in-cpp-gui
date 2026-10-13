#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <xmmintrin.h>
#include "Move.h"

namespace ImprovedTT {

   enum TTFlag : uint8_t { TT_NONE, TT_EXACT, TT_ALPHA, TT_BETA };

   // Lockless entry: the payload is packed into one 64-bit word, and the key is stored XOR-ed with it.
   // A torn write (key and data from different stores) fails the key check, so no locks are needed.
   struct TTEntry {
       std::atomic<uint64_t> keyXorData{0};
       std::atomic<uint64_t> data{0};
   };

   constexpr int CLUSTER_SIZE = 4;

   // one cluster = one 64-byte cache line
   struct alignas(64) TTCluster {
       TTEntry entry[CLUSTER_SIZE];
   };
   static_assert(sizeof(TTCluster) == 64, "a TT cluster must fill exactly one cache line");

   class TranspositionTable {
   public:
       TranspositionTable(size_t mb);

       void NewWrite() { generation++; }

       void Prefetch(uint64_t hash);

       void Store(uint64_t hash, int score, int ply, int depth, TTFlag flag, Move bestMove);
       bool Probe(uint64_t hash, int ply, int depth, int alpha, int beta, int& score, Move& bestMove, int& ttDepth, TTFlag& ttFlag);

       Move getPvMove(uint64_t hash);

       void Clear();

   private:
       std::unique_ptr<TTCluster[]> table;
       size_t size;
       uint8_t generation = 0;

       int ScoreToTT(int score, int ply);
       int ScoreFromTT(int score, int ply);
   };

}
