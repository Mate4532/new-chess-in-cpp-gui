#pragma once
#include "Board.h"

class UCIParsing {
public:
	static Move Parse(const std::string& uci, const Board& board);
	static std::string MoveToUCI(const Move& m);
};