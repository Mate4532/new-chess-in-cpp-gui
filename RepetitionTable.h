#pragma once
#include <cstdint>
#include <algorithm>
#include <vector>

class Board;

class RepetitionTable {
private:
    static constexpr int MAX_REPETITION = 256;

    uint64_t hashes[MAX_REPETITION];
    int startIndices[MAX_REPETITION + 1];
    int count;

public:
    RepetitionTable() : count(0) {
        Clear();
    }

    void Init(const Board& board);
    void Push(uint64_t hash, bool reset);
    void TryPop();
    bool Contains(uint64_t hash) const;
    bool IsDraw(uint64_t hash) const;
    inline std::vector<uint64_t> getHashes() const {
        return std::vector<uint64_t>(hashes, hashes + count);
    }
    void Clear();
};
