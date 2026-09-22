#include "RepetitionTable.h"
#include "Board.h"

void RepetitionTable::Init(const Board& board) {
    Clear();
    std::vector<uint64_t> repetition_hashes = board.getRepetitionHash();

    // getRepetitionHash already strips entries before the last irreversible move
    // (it returns hashes from startIndices[count] onward). Treat them all as one
    // continuous window with start index 0, matching what Push(hash, false) would do.
    // The old code set startIndices[i]=0 for all entries but then wrote
    // startIndices[count]=0 again AFTER the loop, which was harmless but only
    // correct by accident; be explicit.
    for (int i = 0; i < (int)repetition_hashes.size(); i++) {
        hashes[count] = repetition_hashes[i];
        startIndices[count + 1] = startIndices[count]; // no reset: all share the same window
        ++count;
    }
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

bool RepetitionTable::IsDraw(uint64_t hash, bool inSearch) const
{
    int start = startIndices[count];
	int counter = 0;

    for (int i = start; i < count; i++) {
        if (hashes[i] == hash) {
            counter++;
        }
    }

    return inSearch ? counter >= 2 : counter >= 3;
}

void RepetitionTable::Clear(){
    count = 0;
    std::fill(hashes, hashes + MAX_REPETITION, 0ULL);
    std::fill(startIndices, startIndices + MAX_REPETITION + 1, 0);
}
