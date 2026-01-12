#include "RepetitionTable.h"
#include "Board.h"

void RepetitionTable::Init(const Board& board) {
    Clear();
	std::vector<uint64_t> repetition_hashes = board.getRepetitionHash();
    count = repetition_hashes.size();

    for (int i = 0; i < repetition_hashes.size(); i++) {
        hashes[i] = repetition_hashes[i];
        startIndices[i] = 0;
    }
    startIndices[count] = 0;
}

void RepetitionTable::Push(uint64_t hash, bool reset)
{
    if (count >= MAX_REPETITION) {
        return;
    }

    hashes[count] = hash;
    startIndices[count + 1] = reset ? count : startIndices[count];
    ++count;
}

void RepetitionTable::TryPop()
{
    count = std::max(0, count - 1);
}

bool RepetitionTable::Contains(uint64_t hash) const {

    int start = startIndices[count];

    for (int i = start; i < count - 1; i++) {
        if (hashes[i] == hash) {
            return true;
        }
    }
    return false;
}

bool RepetitionTable::IsDraw(uint64_t hash) const
{
    int start = startIndices[count];
	int counter = 0;

    for (int i = start; i < count; i++) {
        if (hashes[i] == hash) {
            counter++;
        }
    }

    return counter >= 3;
}

void RepetitionTable::Clear(){
    std::fill(hashes, hashes + MAX_REPETITION, 0ULL);
    std::fill(startIndices, startIndices + MAX_REPETITION + 1, 0);
}
