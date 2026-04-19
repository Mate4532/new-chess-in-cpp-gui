#include "PGNFormatter.h"
#include "versioncontrol.h"

std::string PGNFormatter::convertMoveToSAN(const Move& m, const MoveList& currentLegalMoves, bool wasMoveCheck, bool addPieceCharToString) {
    MoveFlag flags = m.getFlags();
    std::string result = "";

    if (flags == KINGSIDE_CASTLE) {
        result = "O-O";
    } else if (flags == QUEENSIDE_CASTLE) {
        result = "O-O-O";
    } else {
        std::string fromStr = square_to_coordinates[m.getFrom()];
        std::string toStr = square_to_coordinates[m.getTo()];
        PieceType pt = m.getPieceType();
        bool isCapture = (flags & CAPTURE_FLAG) || (flags == EN_PASSANT);

        char pieceChar = '\0';
        switch (pt) {
        case KNIGHT: pieceChar = 'N'; break;
        case BISHOP: pieceChar = 'B'; break;
        case ROOK:   pieceChar = 'R'; break;
        case QUEEN:  pieceChar = 'Q'; break;
        case KING:   pieceChar = 'K'; break;
        default: break;
        }

        std::string ambiguity = "";
        if (pt != KING && pt != PAWN) {
            bool conflict = false;
            bool sameFile = false;
            bool sameRank = false;

            for (int i = 0; i < currentLegalMoves.size(); ++i) {
                Move legalMove = currentLegalMoves[i];
                if (m != legalMove &&
                    legalMove.getPieceType() == pt &&
                    m.getTo() == legalMove.getTo()) {

                    conflict = true;
                    std::string otherFrom = square_to_coordinates[legalMove.getFrom()];
                    if (fromStr[0] == otherFrom[0]) sameFile = true;
                    if (fromStr[1] == otherFrom[1]) sameRank = true;
                }
            }

            if (conflict) {
                if (!sameFile) ambiguity = fromStr[0];
                else if (!sameRank) ambiguity = fromStr[1];
                else ambiguity = fromStr;
            }
        }

        if (pt == PAWN) {
            if (isCapture) {
                result += fromStr[0];
                result += "x";
            }
        } else {
            if (addPieceCharToString) result += pieceChar;
            result += ambiguity;
            if (isCapture) result += "x";
        }

        result += toStr;

        if (flags & PROMOTION_FLAG) {
            PieceType promoPt = m.getPromotionPieceType();
            result += "=";
            switch (promoPt) {
            case KNIGHT: result += "N"; break;
            case BISHOP: result += "B"; break;
            case ROOK:   result += "R"; break;
            case QUEEN:  result += "Q"; break;
            default: break;
            }
        }
    }

    if (wasMoveCheck) {
        result += "+";
    }

    return result;
}

std::string PGNFormatter::createFullPGN(
    const GameResult& gameResult,
    const std::string& whiteName,
    const std::string& blackName,
    const std::vector<std::string>& moveList,
    const std::string& startingFen
    ) {
    std::ostringstream pgn;

    pgn << "[Event \"Bot vs Bot Match\"]\n";
    pgn << "[Site \"Local Simulation\"]\n";

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    pgn << "[Date \"" << std::put_time(&tm, "%Y.%m.%d") << "\"]\n";

    pgn << "[White \"" << VersionControl::getLatestVersionName(whiteName) << "\"]\n";
    pgn << "[Black \"" << VersionControl::getLatestVersionName(blackName) << "\"]\n";

    std::string resStr = "*";
    if (gameResult == DRAW) resStr = "1/2-1/2";
    else if ((gameResult & WHITE_WON) != 0) resStr = "1-0";
    else if ((gameResult & BLACK_WON) != 0) resStr = "0-1";
    pgn << "[Result \"" << resStr << "\"]\n";

    if (!startingFen.empty() && startingFen != newPosFen) {
        pgn << "[SetUp \"1\"]\n";
        pgn << "[FEN \"" << startingFen << "\"]\n";
    }
    pgn << "\n";

    bool whiteToMove = true;
    if (!startingFen.empty()) {
        size_t firstSpace = startingFen.find(' ');
        if (firstSpace != std::string::npos && startingFen.size() > firstSpace + 1) {
            if (startingFen[firstSpace + 1] == 'b') whiteToMove = false;
        }
    }

    int fullMoveCount = 1;

    bool firstMoveInFile = true;

    for (size_t i = 0; i < moveList.size(); ++i) {
        const std::string& san = moveList[i];

        if (san.empty()) continue;

        if (whiteToMove) {
            pgn << fullMoveCount << ". " << san << " ";
        } else {
            if (firstMoveInFile) {
                pgn << fullMoveCount << "... " << san << " ";
            } else {
                pgn << san << " ";
                fullMoveCount++;
            }
        }

        if ((i + 1) % 12 == 0) pgn << "\n";

        whiteToMove = !whiteToMove;
        firstMoveInFile = false;
    }

    pgn << " " << resStr << "\n";

    return pgn.str();
}
