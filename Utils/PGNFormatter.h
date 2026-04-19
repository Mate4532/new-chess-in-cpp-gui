#ifndef PGNFORMATTER_H
#define PGNFORMATTER_H

#include "Board.h"

class PGNFormatter
{
public:
    static std::string convertMoveToSAN(const Move& m, const MoveList& currentLegalMoves, bool wasMoveCheck, bool addPieceCharToString = true);
    static std::string createFullPGN(
        const GameResult& gameResult,
        const std::string& whiteName,
        const std::string& blackName,
        const std::vector<std::string>& moveList,
        const std::string& startingFen
        );
};

#endif // PGNFORMATTER_H
