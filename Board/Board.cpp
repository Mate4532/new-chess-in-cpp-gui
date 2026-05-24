#include "Board.h"
#include "Zobrist.h"
#include "PGNFormatter.h"
#include <mutex>

static std::once_flag init_flag;

uint64_t Board::pawn_attacks_table[2][64];
uint64_t Board::knight_attacks_table[64];
uint64_t Board::king_attacks_table[64];

uint64_t Board::raw_rook_magics[] = { 468374916371625120, 18428729537625841661, 2531023729696186408, 6093370314119450896, 13830552789156493815, 16134110446239088507, 12677615322350354425, 5404321144167858432, 2111097758984580, 18428720740584907710, 17293734603602787839, 4938760079889530922, 7699325603589095390, 9078693890218258431, 578149610753690728, 9496543503900033792, 1155209038552629657, 9224076274589515780, 1835781998207181184, 509120063316431138, 16634043024132535807, 18446673631917146111, 9623686630121410312, 4648737361302392899, 738591182849868645, 1732936432546219272, 2400543327507449856, 5188164365601475096, 10414575345181196316, 1162492212166789136, 9396848738060210946, 622413200109881612, 7998357718131801918, 7719627227008073923, 16181433497662382080, 18441958655457754079, 1267153596645440, 18446726464209379263, 1214021438038606600, 4650128814733526084, 9656144899867951104, 18444421868610287615, 3695311799139303489, 10597006226145476632, 18436046904206950398, 18446726472933277663, 3458977943764860944, 39125045590687766, 9227453435446560384, 6476955465732358656, 1270314852531077632, 2882448553461416064, 11547238928203796481, 1856618300822323264, 2573991788166144, 4936544992551831040, 13690941749405253631, 15852669863439351807, 18302628748190527413, 12682135449552027479, 13830554446930287982, 18302628782487371519, 7924083509981736956, 4734295326018586370 };
int Board::raw_rook_shifts[] = { 52, 52, 52, 52, 52, 52, 52, 52, 53, 53, 53, 54, 53, 53, 54, 53, 53, 54, 54, 54, 53, 53, 54, 53, 53, 54, 53, 53, 54, 54, 54, 53, 52, 54, 53, 53, 53, 53, 54, 53, 52, 53, 54, 54, 53, 53, 54, 53, 53, 54, 54, 54, 53, 53, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52 };

uint64_t Board::raw_bishop_magics[] = { 16509839532542417919, 14391803910955204223, 1848771770702627364, 347925068195328958, 5189277761285652493, 3750937732777063343, 18429848470517967340, 17870072066711748607, 16715520087474960373, 2459353627279607168, 7061705824611107232, 8089129053103260512, 7414579821471224013, 9520647030890121554, 17142940634164625405, 9187037984654475102, 4933695867036173873, 3035992416931960321, 15052160563071165696, 5876081268917084809, 1153484746652717320, 6365855841584713735, 2463646859659644933, 1453259901463176960, 9808859429721908488, 2829141021535244552, 576619101540319252, 5804014844877275314, 4774660099383771136, 328785038479458864, 2360590652863023124, 569550314443282, 17563974527758635567, 11698101887533589556, 5764964460729992192, 6953579832080335136, 1318441160687747328, 8090717009753444376, 16751172641200572929, 5558033503209157252, 17100156536247493656, 7899286223048400564, 4845135427956654145, 2368485888099072, 2399033289953272320, 6976678428284034058, 3134241565013966284, 8661609558376259840, 17275805361393991679, 15391050065516657151, 11529206229534274423, 9876416274250600448, 16432792402597134585, 11975705497012863580, 11457135419348969979, 9763749252098620046, 16960553411078512574, 15563877356819111679, 14994736884583272463, 9441297368950544394, 14537646123432199168, 9888547162215157388, 18140215579194907366, 18374682062228545019 };
int Board::raw_bishop_shifts[] = { 58, 60, 59, 59, 59, 59, 60, 58, 60, 59, 59, 59, 59, 59, 59, 60, 59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59, 60, 60, 59, 59, 59, 59, 60, 60, 58, 60, 59, 59, 59, 59, 59, 58 };

uint64_t Board::rook_table[64][4096];
uint64_t Board::bishop_table[64][512];
Magic Board::rook_magics[64];
Magic Board::bishop_magics[64];

uint64_t Board::passed_pawn_mask[2][64];

std::atomic<uint64_t> global_node_count(0);

Board::Board() {
    InitializeBoard();
}

void Board::InitializeBoard() {
    std::call_once(init_flag, [this]() {
        Zobrist::Init();
        InitializeAttackTables();
        InitializeMagicTables();
        InizializePassedPawnTable();
    });

    LoadFEN(newPosFen);
}

void Board::InitializeAttackTables() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t b = (1ULL << sq);

        uint64_t k_attacks = 0;
        k_attacks |= (b << 17) & ~FILE_A;
        k_attacks |= (b << 15) & ~FILE_H;
        k_attacks |= (b << 10) & ~(FILE_A | FILE_B);
        k_attacks |= (b << 6) & ~(FILE_G | FILE_H);
        k_attacks |= (b >> 17) & ~FILE_H;
        k_attacks |= (b >> 15) & ~FILE_A;
        k_attacks |= (b >> 10) & ~(FILE_G | FILE_H);
        k_attacks |= (b >> 6) & ~(FILE_A | FILE_B);
        knight_attacks_table[sq] = k_attacks;

        uint64_t ki_attacks = 0;
        ki_attacks |= (b << 8);
        ki_attacks |= (b >> 8);
        ki_attacks |= (b << 1) & ~FILE_A;
        ki_attacks |= (b >> 1) & ~FILE_H;
        ki_attacks |= (b << 7) & ~FILE_H;
        ki_attacks |= (b << 9) & ~FILE_A;
        ki_attacks |= (b >> 7) & ~FILE_A;
        ki_attacks |= (b >> 9) & ~FILE_H;
        king_attacks_table[sq] = ki_attacks;

        uint64_t w_pawn = 0;
        w_pawn |= (b << 7) & ~FILE_H;
        w_pawn |= (b << 9) & ~FILE_A;
        pawn_attacks_table[WHITE][sq] = w_pawn;

        uint64_t b_pawn = 0;
        b_pawn |= (b >> 7) & ~FILE_A;
        b_pawn |= (b >> 9) & ~FILE_H;
        pawn_attacks_table[BLACK][sq] = b_pawn;
    }
}

void Board::InitializeMagicTables() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t r_mask = maskRook(sq);
        const_cast<Magic&>(rook_magics[sq]).mask = r_mask;
        const_cast<Magic&>(rook_magics[sq]).magic = raw_rook_magics[sq];
        const_cast<Magic&>(rook_magics[sq]).shift = raw_rook_shifts[sq];

        int r_bits = 0;
        uint64_t temp_r = r_mask;
        while (temp_r) { temp_r &= (temp_r - 1); r_bits++; }

        for (int i = 0; i < (1 << r_bits); i++) {
            uint64_t occ = SetOccupancy(i, r_bits, r_mask);
            uint32_t index = (uint32_t)((occ * rook_magics[sq].magic) >> rook_magics[sq].shift);
            rook_table[sq][index] = getRookAttacksSlow((Square)sq, occ);
        }

        uint64_t b_mask = maskBishop(sq);
        const_cast<Magic&>(bishop_magics[sq]).mask = b_mask;
        const_cast<Magic&>(bishop_magics[sq]).magic = raw_bishop_magics[sq];
        const_cast<Magic&>(bishop_magics[sq]).shift = raw_bishop_shifts[sq];

        int b_bits = 0;
        uint64_t temp_b = b_mask;
        while (temp_b) { temp_b &= (temp_b - 1); b_bits++; }

        for (int i = 0; i < (1 << b_bits); i++) {
            uint64_t occ = SetOccupancy(i, b_bits, b_mask);
            uint32_t index = (uint32_t)((occ * bishop_magics[sq].magic) >> bishop_magics[sq].shift);
            bishop_table[sq][index] = getBishopAttacksSlow((Square)sq, occ);
        }
    }
}

void Board::InizializePassedPawnTable() {
    for (int sq = 0; sq < 64; sq++) {
        int file = sq % 8;
        int rank = sq / 8;

        uint64_t white_mask = 0ULL;
        uint64_t black_mask = 0ULL;

        for (int r = 0; r < 8; r++) {
            for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
                int current_sq = r * 8 + f;
                if (r > rank) white_mask |= (1ULL << current_sq);
                if (r < rank) black_mask |= (1ULL << current_sq);
            }
        }
        passed_pawn_mask[WHITE][sq] = white_mask;
        passed_pawn_mask[BLACK][sq] = black_mask;
    }
}

uint64_t Board::GenerateFullHash() const {
    uint64_t h = 0ULL;
    for (int c = 0; c < 2; c++) {
        for (int p = PAWN; p <= KING; p++) {
            uint64_t bb = m_bitboards[c][p];
            while (bb) {
                Square sq = PopBit(bb);
                h ^= Zobrist::pieceKeys[c][p][sq];
            }
        }
    }
    if (m_side_to_move == BLACK) h ^= Zobrist::sideKey;

    h ^= Zobrist::castlingKeys[boardStateHistory[m_ply].castling_rights];

    Square ep = (Square)boardStateHistory[m_ply].en_passant_sq;
    if (ep != SQUARE_NONE) h ^= Zobrist::enPassantKeys[ep % 8];

    return h;
}

uint64_t Board::SetOccupancy(int index, int bits_in_mask, uint64_t mask) {
    uint64_t occupancy = 0ULL;
    for (int i = 0; i < bits_in_mask; i++) {
        int square = PopBit(mask);
        if (index & (1 << i)) occupancy |= (1ULL << (uint64_t)square);
    }
    return occupancy;
}

uint64_t Board::maskRook(int sq) {
    uint64_t mask = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i < 7; i++) mask |= (1ULL << (i * 8 + f));
    for (int i = r - 1; i > 0; i--) mask |= (1ULL << (i * 8 + f));
    for (int i = f + 1; i < 7; i++) mask |= (1ULL << (r * 8 + i));
    for (int i = f - 1; i > 0; i--) mask |= (1ULL << (r * 8 + i));
    return mask;
}

uint64_t Board::maskBishop(int sq) {
    uint64_t mask = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1, j = f + 1; i < 7 && j < 7; i++, j++) mask |= (1ULL << (i * 8 + j));
    for (int i = r + 1, j = f - 1; i < 7 && j > 0; i++, j--) mask |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = f + 1; i > 0 && j < 7; i--, j++) mask |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = f - 1; i > 0 && j > 0; i--, j--) mask |= (1ULL << (i * 8 + j));
    return mask;
}

void Board::LoadFEN(std::string fen) {

    ClearBoard();
    beginnerFen = fen;

    std::cout << fen;

    if (fen.empty()) {
        fen = newPosFen;
    }

    for (int c = 0; c < 2; c++) {
        m_side_occupancy[c] = 0ULL;
        for (int p = PAWN; p <= KING; p++) {
            m_bitboards[c][p] = 0ULL;
            piece_count[c][p] = 0;
        }
    }
    m_all_occupancy = 0ULL;

    std::stringstream ss(fen);
    std::string pieces, side, castling, enPassant, halfMove, fullMove;
    ss >> pieces >> side >> castling >> enPassant >> halfMove >> fullMove;

    int rank = 7;
    int file = 0;
    for (char c : pieces) {
        if (c == '/') {
            rank--;
            file = 0;
        }
        else if (isdigit(c)) {
            file += (c - '0');
        }
        else {
            Color color = isupper(c) ? WHITE : BLACK;
            PieceType type;
            char lowerC = tolower(c);
            if (lowerC == 'p') type = PAWN;
            else if (lowerC == 'n') type = KNIGHT;
            else if (lowerC == 'b') type = BISHOP;
            else if (lowerC == 'r') type = ROOK;
            else if (lowerC == 'q') type = QUEEN;
            else if (lowerC == 'k') type = KING;

            Square sq = (Square)(rank * 8 + file);
            m_bitboards[color][type] |= (1ULL << sq);
            piece_count[color][type]++;
            file++;
        }
    }

    m_side_to_move = (side == "w") ? WHITE : BLACK;

    BoardState state;
    state.castling_rights = 0;
    if (castling != "-") {
        for (char c : castling) {
            if (c == 'K') state.castling_rights |= WHITE_KINGSIDE_CASTLE;
            else if (c == 'Q') state.castling_rights |= WHITE_QUEENSIDE_CASTLE;
            else if (c == 'k') state.castling_rights |= BLACK_KINGSIDE_CASTLE;
            else if (c == 'q') state.castling_rights |= BLACK_QUEENSIDE_CASTLE;
        }
    }

    if (enPassant == "-") {
        state.en_passant_sq = SQUARE_NONE;
    }
    else {
        int f = enPassant[0] - 'a';
        int r = enPassant[1] - '1';
        state.en_passant_sq = (Square)(r * 8 + f);
    }

    state.half_move_clock = halfMove.empty() ? 0 : stoi(halfMove);
    state.full_move_number = fullMove.empty() ? 1 : stoi(fullMove);
    state.captured_piece_type = PIECE_NONE;

    for (int c = 0; c < 2; c++) {
        for (int p = PAWN; p <= KING; p++) {
            m_side_occupancy[c] |= m_bitboards[c][p];
        }
    }
    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];

    boardStateHistory[m_ply] = state;
    boardStateHistory[m_ply].zobrist_hash = GenerateFullHash();

    repetition_history.Push(boardStateHistory[m_ply].zobrist_hash, true);
    pieceHistory.push_back(getBoardMatrix());
    move_history.push_back(Move());

    checkHistory.push_back(false);
    playerToMoveHistory.push_back(m_side_to_move);
    legalMovesHistory.push_back(generateCurrentLegalMoves());
}

std::string Board::GetFEN() const {
    std::stringstream ss;

    for (int rank = 7; rank >= 0; rank--) {
        int empty_squares = 0;

        for (int file = 0; file < 8; file++) {
            Square sq = (Square)GetSquare(rank, file);
            char piece_char = '?';

            PieceType pt_white = getPieceAt(sq, WHITE);
            if (pt_white != PIECE_NONE) {
                if (pt_white == PAWN) piece_char = 'P';
                else if (pt_white == KNIGHT) piece_char = 'N';
                else if (pt_white == BISHOP) piece_char = 'B';
                else if (pt_white == ROOK) piece_char = 'R';
                else if (pt_white == QUEEN) piece_char = 'Q';
                else if (pt_white == KING) piece_char = 'K';
            }
            else {
                // Ha nincs világos, megnézzük a sötétet
                PieceType pt_black = getPieceAt(sq, BLACK);
                if (pt_black != PIECE_NONE) {
                    if (pt_black == PAWN) piece_char = 'p';
                    else if (pt_black == KNIGHT) piece_char = 'n';
                    else if (pt_black == BISHOP) piece_char = 'b';
                    else if (pt_black == ROOK) piece_char = 'r';
                    else if (pt_black == QUEEN) piece_char = 'q';
                    else if (pt_black == KING) piece_char = 'k';
                }
            }

            if (piece_char != '?') {
                if (empty_squares > 0) {
                    ss << empty_squares;
                    empty_squares = 0;
                }
                ss << piece_char;
            } else {
                empty_squares++;
            }
        }

        if (empty_squares > 0) {
            ss << empty_squares;
        }
        if (rank > 0) {
            ss << '/';
        }
    }

    ss << (m_side_to_move == WHITE ? " w " : " b ");

    uint8_t castling = boardStateHistory[m_ply].castling_rights;
    bool has_castling = false;
    if (castling & WHITE_KINGSIDE_CASTLE)  { ss << "K"; has_castling = true; }
    if (castling & WHITE_QUEENSIDE_CASTLE) { ss << "Q"; has_castling = true; }
    if (castling & BLACK_KINGSIDE_CASTLE)  { ss << "k"; has_castling = true; }
    if (castling & BLACK_QUEENSIDE_CASTLE) { ss << "q"; has_castling = true; }
    if (!has_castling) ss << "-";

    Square ep_sq = (Square)boardStateHistory[m_ply].en_passant_sq;
    if (ep_sq == SQUARE_NONE) {
        ss << " - ";
    } else {
        char file_char = 'a' + (ep_sq % 8);
        char rank_char = '1' + (ep_sq / 8);
        ss << " " << file_char << rank_char << " ";
    }

    ss << (int)boardStateHistory[m_ply].half_move_clock << " "
       << (int)boardStateHistory[m_ply].full_move_number;

    return ss.str();
}

void Board::loadNewGame() {
    LoadFEN(newPosFen);
}

PieceType Board::getPieceAt(Square sq, Color color) const {
    for (int piece = PAWN; piece <= KING; piece++) {
        if (m_bitboards[color][piece] & (1ULL << sq)) {
            return (PieceType)piece;
        }
    }
    return PIECE_NONE;
}

uint64_t Board::getRookAttacksSlow(Square sq, uint64_t occupied) const {
    uint64_t attacks = 0;
    int dr[] = { 1, -1, 0, 0 };
    int df[] = { 0, 0, 1, -1 };

    int r = sq / 8;
    int f = sq % 8;

    for (int i = 0; i < 4; i++) {
        for (int step = 1; step < 8; step++) {
            int nr = r + dr[i] * step;
            int nf = f + df[i] * step;
            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;

            Square target = (Square)(nr * 8 + nf);
            attacks |= (1ULL << target);
            if (occupied & (1ULL << target)) break;
        }
    }
    return attacks;
}

uint64_t Board::getRookAttacks(Square sq, uint64_t occupied) const {
    uint64_t occ = occupied & rook_magics[sq].mask;
    uint32_t index = static_cast<uint32_t>((occ * rook_magics[sq].magic) >> rook_magics[sq].shift);
    return rook_table[sq][index];
}

uint64_t Board::getKnightAttacks(Square s) const {
    uint64_t b = 1ULL << s;
    uint64_t attacks = 0;
    attacks |= (b << 17) & ~FILE_A;
    attacks |= (b << 15) & ~FILE_H;
    attacks |= (b << 10) & ~(FILE_A | FILE_B);
    attacks |= (b << 6) & ~(FILE_G | FILE_H);

    attacks |= (b >> 17) & ~FILE_H;
    attacks |= (b >> 15) & ~FILE_A;
    attacks |= (b >> 10) & ~(FILE_G | FILE_H);
    attacks |= (b >> 6) & ~(FILE_A | FILE_B);

    return attacks;
}

uint64_t Board::getBishopAttacksSlow(Square sq, uint64_t occupied) const {
    uint64_t attacks = 0;

    int dr[] = { 1, 1, -1, -1 };
    int df[] = { 1, -1, 1, -1 };

    int r = sq / 8;
    int f = sq % 8;

    for (int i = 0; i < 4; i++) {
        for (int step = 1; step < 8; step++) {
            int nr = r + dr[i] * step;
            int nf = f + df[i] * step;

            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;

            Square target = (Square)(nr * 8 + nf);
            attacks |= (1ULL << target);

            if (occupied & (1ULL << target)) break;
        }
    }
    return attacks;
}

uint64_t Board::getBishopAttacks(Square sq, uint64_t occupied) const {
    uint64_t occ = occupied & bishop_magics[sq].mask;
    uint32_t index = static_cast<uint32_t>((occ * bishop_magics[sq].magic) >> bishop_magics[sq].shift);
    return bishop_table[sq][index];
}

uint64_t Board::getKingAttacks(Square sq) const {
    uint64_t bit = (1ULL << sq);
    uint64_t attacks = 0;

    attacks |= (bit << 8);
    attacks |= (bit >> 8);
    attacks |= (bit << 1) & ~FILE_A;
    attacks |= (bit >> 1) & ~FILE_H;

    attacks |= (bit << 7) & ~FILE_H;
    attacks |= (bit << 9) & ~FILE_A;
    attacks |= (bit >> 7) & ~FILE_A;
    attacks |= (bit >> 9) & ~FILE_H;

    return attacks;

}

uint64_t Board::getPawnAttacks(Square sq, Color attackerColor) const {

    return pawn_attacks_table[attackerColor][sq];
}

uint64_t Board::getInvertedPawnAttacks(Square sq, Color attackerColor) const {

    return pawn_attacks_table[attackerColor ^ 1][sq];
}

bool Board::IsCheckMate() {
    Color us = m_side_to_move;
    Color enemy = (Color)(us ^ 1);
    bool inCheck = isSquareAttacked(getKingSquare(us), enemy);

    if (!inCheck) return false;

    MoveList moves;
    MoveGenerator::GenerateMoves(*this, moves);

    for (int i = 0; i < moves.size(); i++) {
        if (MakeMove(moves[i], true)) {
            UndoMove(moves[i], true);
            return false;
        }
    }

    return true;
}

bool Board::HasNonPawnMaterial(Color color) const {
if (getPieceBitboard(color, KNIGHT)) return true;
    if (getPieceBitboard(color, BISHOP)) return true;
    if (getPieceBitboard(color, ROOK))   return true;
    if (getPieceBitboard(color, QUEEN))  return true;
    return false;
}

bool Board::hasPromotingPawn() const {
    if (m_side_to_move == WHITE) {
        return (m_bitboards[WHITE][PAWN] & RANK_7) != 0;
    }
    else {
        return (m_bitboards[BLACK][PAWN] & RANK_2) != 0;
    }
}

bool Board::hasAdvancedPawn() const {
    if (m_side_to_move == WHITE) {
        return (m_bitboards[WHITE][PAWN] & (RANK_7 | RANK_6)) != 0;
    }
    else {
        return (m_bitboards[BLACK][PAWN] & (RANK_2 | RANK_3)) != 0;
    }
}

bool Board::hasAdvancedPassedPawn(Color color) const {
    uint64_t advancedPawns;

    if (color == WHITE) {
        advancedPawns = m_bitboards[WHITE][PAWN] & (RANK_6 | RANK_7);
    } else {
        advancedPawns = m_bitboards[BLACK][PAWN] & (RANK_3 | RANK_2);
    }

    if (!advancedPawns) return false;

    uint64_t tempPawns = advancedPawns;
    while (tempPawns) {
        Square sq = PopBit(tempPawns);

        if (isPassedPawn(color, sq)) {
            return true;
        }
    }

    return false;
}

uint64_t Board::getAttacksTo(Square sq, uint64_t occupied) const {
    return (getPawnAttacks(sq, BLACK) & getPieceBitboard(WHITE, PAWN)) |
        (getPawnAttacks(sq, WHITE) & getPieceBitboard(BLACK, PAWN)) |
        (getKnightAttacks(sq) & (getPieceBitboard(WHITE, KNIGHT) | getPieceBitboard(BLACK, KNIGHT))) |
        (getBishopAttacks(sq, occupied) & (getPieceBitboard(WHITE, BISHOP) | getPieceBitboard(BLACK, BISHOP) | getPieceBitboard(WHITE, QUEEN) | getPieceBitboard(BLACK, QUEEN))) |
        (getRookAttacks(sq, occupied) & (getPieceBitboard(WHITE, ROOK) | getPieceBitboard(BLACK, ROOK) | getPieceBitboard(WHITE, QUEEN) | getPieceBitboard(BLACK, QUEEN))) |
        (getKingAttacks(sq) & (getPieceBitboard(WHITE, KING) | getPieceBitboard(BLACK, KING)));
}

Square Board::getSmallestAttacker(uint64_t attackers, Color side, PieceType& attackerType) const {
    for (int pt = PAWN; pt <= KING; pt++) {
        uint64_t subset = attackers & getPieceBitboard(side, (PieceType)pt);
        if (subset) {
            attackerType = (PieceType)pt;
            return PopBit(subset);
        }
    }
    attackerType = PIECE_NONE;
    return SQUARE_NONE;
}

uint64_t Board::getNewXRayAttacks(Square to, uint64_t occupied) const {
    uint64_t attackers = 0;

    uint64_t diagonalPieces = getPieceBitboard(WHITE, BISHOP) | getPieceBitboard(BLACK, BISHOP) |
        getPieceBitboard(WHITE, QUEEN) | getPieceBitboard(BLACK, QUEEN);

    uint64_t straightPieces = getPieceBitboard(WHITE, ROOK) | getPieceBitboard(BLACK, ROOK) |
        getPieceBitboard(WHITE, QUEEN) | getPieceBitboard(BLACK, QUEEN);

    attackers |= getBishopAttacks(to, occupied) & diagonalPieces;

    attackers |= getRookAttacks(to, occupied) & straightPieces;

    return attackers;
}

bool Board::IsInsufficientMaterial() const {
    if (getPieceBitboard(WHITE, PAWN) || getPieceBitboard(BLACK, PAWN)) return false;
    if (getPieceBitboard(WHITE, ROOK) || getPieceBitboard(BLACK, ROOK)) return false;
    if (getPieceBitboard(WHITE, QUEEN) || getPieceBitboard(BLACK, QUEEN)) return false;

    uint64_t wN = getPieceBitboard(WHITE, KNIGHT);
    uint64_t wB = getPieceBitboard(WHITE, BISHOP);
    uint64_t bN = getPieceBitboard(BLACK, KNIGHT);
    uint64_t bB = getPieceBitboard(BLACK, BISHOP);

    int numWN = std::popcount(wN);
    int numWB = std::popcount(wB);
    int numBN = std::popcount(bN);
    int numBB = std::popcount(bB);

    int whiteTotal = numWN + numWB;
    int blackTotal = numBN + numBB;

    if (whiteTotal == 0 && blackTotal == 0) return true;

    if ((whiteTotal == 1 && blackTotal == 0) || (whiteTotal == 0 && blackTotal == 1)) return true;

    if (numWN == 1 && numWB == 0 && numBN == 1 && numBB == 0) return true;

    if (numWB == 1 && numWN == 0 && numBB == 1 && numBN == 0) {

        Square wBishopSq = (Square)std::countr_zero(wB);
        Square bBishopSq = (Square)std::countr_zero(bB);

        bool wBishopIsLight = (LIGHT_SQUARES & (1ULL << wBishopSq));
        bool bBishopIsLight = (DARK_SQUARES & (1ULL << bBishopSq));

        if (wBishopIsLight == bBishopIsLight) {
            return true;
        }
    }

    return false;
}

bool Board::IsStalemate() {
    Color us = m_side_to_move;
    Color enemy = (Color)(us ^ 1);

    bool inCheck = isSquareAttacked(getKingSquare(us), enemy);
    if (inCheck) return false;

    MoveList moves;
    MoveGenerator::GenerateMoves(*this, moves);

    for (int i = 0; i < moves.size(); i++) {
        if (MakeMove(moves[i], true)) {
            UndoMove(moves[i], true);
            return false;
        }
    }

    return true;
}

bool Board::IsDraw() {

    if (boardStateHistory[m_ply].half_move_clock >= 100)
        return true;

    if (repetition_history.IsDraw(boardStateHistory[m_ply].zobrist_hash))
        return true;

	if (IsStalemate())
        return true;

    if (IsInsufficientMaterial()) return true;

    return false;
}


bool Board::isSquareAttacked(Square sq, Color attackerColor) const {
    if (pawn_attacks_table[attackerColor ^ 1][sq] & m_bitboards[attackerColor][PAWN]) return true;

    if (knight_attacks_table[sq] & m_bitboards[attackerColor][KNIGHT]) return true;
    if (king_attacks_table[sq] & m_bitboards[attackerColor][KING]) return true;

    uint64_t occ = m_all_occupancy;
    if (getBishopAttacks(sq, occ) & (m_bitboards[attackerColor][BISHOP] | m_bitboards[attackerColor][QUEEN])) return true;
    if (getRookAttacks(sq, occ) & (m_bitboards[attackerColor][ROOK] | m_bitboards[attackerColor][QUEEN])) return true;

    return false;
}

bool Board::isPassedPawn(Color color, Square sq) const {
    Color enemy = (Color)(color ^ 1);
    return !(passed_pawn_mask[color][sq] & m_bitboards[enemy][PAWN]);
}

bool Board::isAdvancedPassedPawnPush(Move move) const {
    if (move.getPieceType() != PAWN) return false;

    Square to_sq = move.getTo();
    Color player = m_side_to_move;

    if (!isPassedPawn(player, to_sq)) return false;

    int rank = to_sq / 8;
    if (player == WHITE) {
        return rank >= 5;
    } else {
        return rank <= 2;
    }
}

bool Board::MakeMove(Move move, bool in_search) {

    Square from_sq = move.getFrom();
    Square to_sq = move.getTo();
    MoveFlag flags = move.getFlags();
    Color player = m_side_to_move;
    Color enemy = (player == WHITE) ? BLACK : WHITE;
    PieceType piece = move.getPieceType();

    BoardState newBoardState;
    newBoardState.captured_piece_type = PIECE_NONE;
    newBoardState.en_passant_sq = SQUARE_NONE;
    newBoardState.castling_rights = boardStateHistory[m_ply].castling_rights;
    newBoardState.half_move_clock = boardStateHistory[m_ply].half_move_clock + 1;
    newBoardState.full_move_number = boardStateHistory[m_ply].full_move_number + (player == BLACK ? 1 : 0);

    uint64_t newHash = boardStateHistory[m_ply].zobrist_hash;
    newHash ^= Zobrist::pieceKeys[player][piece][from_sq];

    if (flags & CAPTURE_FLAG) {
        newBoardState.half_move_clock = 0;
        if (flags == EN_PASSANT) {
            Square cap_sq = (player == WHITE) ? (Square)(to_sq - 8) : (Square)(to_sq + 8);

            newHash ^= Zobrist::pieceKeys[enemy][PAWN][cap_sq];

            newBoardState.captured_piece_type = PAWN;
            m_bitboards[enemy][PAWN] ^= (1ULL << cap_sq);
            m_side_occupancy[enemy] ^= (1ULL << cap_sq);
            piece_count[enemy][PAWN]--;
        }
        else {
            PieceType captured = getPieceAt(to_sq, enemy);

            newHash ^= Zobrist::pieceKeys[enemy][captured][to_sq];

            newBoardState.captured_piece_type = captured;
            m_bitboards[enemy][captured] ^= (1ULL << to_sq);
            m_side_occupancy[enemy] ^= (1ULL << to_sq);
            piece_count[enemy][captured]--;
        }
    }

    if (piece == PAWN || (move.getFlags() & CAPTURE_FLAG)) {
        newBoardState.half_move_clock = 0;
    }

    if (flags & PROMOTION_FLAG) {
        m_bitboards[player][PAWN] ^= (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq);
        piece_count[player][PAWN]--;

        PieceType prom_piece;

        uint8_t promType = flags & 0b0011;
        if (promType == 0b0011) prom_piece = QUEEN;
        else if (promType == 0b0010) prom_piece = ROOK;
        else if (promType == 0b0001) prom_piece = BISHOP;
        else prom_piece = KNIGHT;

        m_bitboards[player][prom_piece] ^= (1ULL << to_sq);
        m_side_occupancy[player] ^= (1ULL << to_sq);
        piece_count[player][prom_piece]++;

        newHash ^= Zobrist::pieceKeys[player][prom_piece][to_sq];
    }
    else {
        m_bitboards[player][piece] ^= (1ULL << from_sq) | (1ULL << to_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);

        newHash ^= Zobrist::pieceKeys[player][piece][to_sq];

        if (flags == DOUBLE_PAWN_PUSH) {
            newBoardState.en_passant_sq = (player == WHITE) ? (Square)(from_sq + 8) : (Square)(from_sq - 8);
            newHash ^= Zobrist::enPassantKeys[newBoardState.en_passant_sq % 8];
        }

        else if (flags == KINGSIDE_CASTLE) {
            Square r_from = (player == WHITE) ? H1 : H8;
            Square r_to = (player == WHITE) ? F1 : F8;
            m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
            m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);

            newHash ^= Zobrist::pieceKeys[player][ROOK][r_from];
            newHash ^= Zobrist::pieceKeys[player][ROOK][r_to];
        }
        else if (flags == QUEENSIDE_CASTLE) {
            Square r_from = (player == WHITE) ? A1 : A8;
            Square r_to = (player == WHITE) ? D1 : D8;
            m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
            m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);

            newHash ^= Zobrist::pieceKeys[player][ROOK][r_from];
            newHash ^= Zobrist::pieceKeys[player][ROOK][r_to];
        }
    }

    if (boardStateHistory[m_ply].en_passant_sq != SQUARE_NONE)
        newHash ^= Zobrist::enPassantKeys[boardStateHistory[m_ply].en_passant_sq % 8];

    newHash ^= Zobrist::castlingKeys[boardStateHistory[m_ply].castling_rights];

    if (piece == KING) {
        newBoardState.castling_rights &= (player == WHITE ? ~WHITE_ALL_CASTLE_RIGHTS : ~BLACK_ALL_CASTLE_RIGHTS);
    }

    newBoardState.castling_rights &= castling_mask[from_sq];
    newBoardState.castling_rights &= castling_mask[to_sq];

    newHash ^= Zobrist::castlingKeys[newBoardState.castling_rights];

    newHash ^= Zobrist::sideKey;

    newBoardState.zobrist_hash = newHash;
    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];
    m_ply++;
    boardStateHistory[m_ply] = newBoardState;
    m_side_to_move = enemy;

    bool reset = (piece == PAWN) || (flags & CAPTURE_FLAG);
    repetition_history.Push(newHash, reset);

    if (!in_search) {
        Square enemyKingSq = getKingSquare(enemy);
        bool gaveCheck = isSquareAttacked(enemyKingSq, player);

        committedPly++;
        move_history.push_back(move);
        pieceHistory.push_back(getBoardMatrix());
        checkHistory.push_back(gaveCheck);
        playerToMoveHistory.push_back(m_side_to_move);
        legalMovesHistory.push_back(generateCurrentLegalMoves());
    }

    Square kingSq = getKingSquare(player);
    if (isSquareAttacked(kingSq, enemy)) {
        UndoMove(move, in_search);
        return false;
    }

    return true;
}

void Board::UndoMove(Move move, bool in_search) {
    Square from_sq = move.getFrom();
    Square to_sq = move.getTo();
    MoveFlag flags = move.getFlags();
    PieceType piece_type = move.getPieceType();

    BoardState& state_to_undo = boardStateHistory[m_ply];
    PieceType captured = (PieceType)state_to_undo.captured_piece_type;

    m_side_to_move = (m_side_to_move == WHITE) ? BLACK : WHITE;
    Color player = m_side_to_move;
    Color enemy = (player == WHITE) ? BLACK : WHITE;

    if (flags & PROMOTION_FLAG) {
        PieceType promoted;
        uint8_t promType = flags & 0b0011;
        if (promType == 0b0011) promoted = QUEEN;
        else if (promType == 0b0010) promoted = ROOK;
        else if (promType == 0b0001) promoted = BISHOP;
        else promoted = KNIGHT;

        m_bitboards[player][promoted] ^= (1ULL << to_sq);
        m_bitboards[player][PAWN] ^= (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);

        piece_count[player][promoted]--;
        piece_count[player][PAWN]++;
    }

    else {
        m_bitboards[player][piece_type] ^= (1ULL << to_sq) | (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << to_sq) | (1ULL << from_sq);

        if (flags == KINGSIDE_CASTLE) {
            Square r_from = (player == WHITE) ? H1 : H8;
            Square r_to = (player == WHITE) ? F1 : F8;
            m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
            m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);
        }
        else if (flags == QUEENSIDE_CASTLE) {
            Square r_from = (player == WHITE) ? A1 : A8;
            Square r_to = (player == WHITE) ? D1 : D8;
            m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
            m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);
        }
    }

    if (flags & CAPTURE_FLAG) {
        if (flags == EN_PASSANT) {
            Square cap_sq = (player == WHITE) ? (Square)(to_sq - 8) : (Square)(to_sq + 8);
            m_bitboards[enemy][PAWN] ^= (1ULL << cap_sq);
            m_side_occupancy[enemy] ^= (1ULL << cap_sq);
            piece_count[enemy][PAWN]++;
        }
        else {
            m_bitboards[enemy][captured] ^= (1ULL << to_sq);
            m_side_occupancy[enemy] ^= (1ULL << to_sq);
            piece_count[enemy][captured]++;
        }
    }

    repetition_history.TryPop();

    if (!in_search) {
        committedPly--;
        move_history.pop_back();
        pieceHistory.pop_back();
        checkHistory.pop_back();
        playerToMoveHistory.pop_back();
        legalMovesHistory.pop_back();
    }

    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];
    m_ply--;
}

void Board::MakeNullMove() {

    m_ply++;
    boardStateHistory[m_ply] = boardStateHistory[m_ply - 1];

    if (boardStateHistory[m_ply].en_passant_sq != SQUARE_NONE) {
        boardStateHistory[m_ply].zobrist_hash ^= Zobrist::enPassantKeys[boardStateHistory[m_ply].en_passant_sq & 7];
        boardStateHistory[m_ply].en_passant_sq = SQUARE_NONE;
    }

    m_side_to_move = (m_side_to_move == WHITE) ? BLACK : WHITE;
    boardStateHistory[m_ply].zobrist_hash ^= Zobrist::sideKey;
}

void Board::UndoNullMove() {
    m_side_to_move = (m_side_to_move == WHITE) ? BLACK : WHITE;
    m_ply--;
}

uint64_t Board::PerftDivide(int depth) {
    MoveList moves;
    MoveGenerator::GenerateMoves(*this, moves);
    uint64_t total_nodes = 0;
    for (const Move& move : moves) {
        if (!MakeMove(move)) {
            continue;
        }
        uint64_t nodes = Perft(depth - 1);
        std::cout << square_to_coordinates[(int)move.getFrom()] << " -> " << square_to_coordinates[(int)move.getTo()] << ": " << nodes << std::endl;
        total_nodes += nodes;

        UndoMove(move);
    }
    return total_nodes;
}

uint64_t Board::Perft(int depth) {
    if (depth == 0) return 1ULL;

    MoveList moves;
    MoveGenerator::GenerateMoves(*this, moves);

    uint64_t nodes = 0;

    for (const Move& move : moves) {
        if (!MakeMove(move)) {
            continue;
        }

        nodes += Perft(depth - 1);

        UndoMove(move);
    }

    return nodes;
}

void Board::PrintBoard(bool is_white_player, bool is_black_player) const {
    std::cout << "\n    +-----------------+" << std::endl;

    for (int rank = 7; rank >= 0; rank--)
    {
        std::cout << "  " << (rank + 1) << " |";

        for (int file = 0; file < 8; file++)
        {
            int sq = is_black_player && !is_white_player ? GetSquare(7 - rank, 7 - file) : GetSquare(rank, file);

            char piece_to_print = '.';
            bool found = false;

            for (int color = 0; color < 2; color++)
            {
                for (int piece_type = PAWN; piece_type <= KING; piece_type++)
                {
                    uint64_t current_bb = m_bitboards[color][piece_type];

                    if (current_bb & (1ULL << sq))
                    {
                        piece_to_print = piece_chars[color][piece_type];
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }

            std::cout << " " << piece_to_print;
        }
        std::cout << " |" << std::endl;
    }

    std::cout << "    +-----------------+" << std::endl;
    std::cout << "      A B C D E F G H\n" << std::endl;

    if (isDebugMode) {

        std::cout << "Side to move: " << ((m_side_to_move == WHITE) ? "White" : "Black") << std::endl;
        std::cout << "White pawns count: " << (int)piece_count[WHITE][PAWN] << std::endl;
        std::cout << "White rooks count: " << (int)piece_count[WHITE][ROOK] << std::endl;
        std::cout << "White knights count: " << (int)piece_count[WHITE][KNIGHT] << std::endl;
        std::cout << "White bishops count: " << (int)piece_count[WHITE][BISHOP] << std::endl;
        std::cout << "White queens count: " << (int)piece_count[WHITE][QUEEN] << std::endl;
        std::cout << "White king count: " << (int)piece_count[WHITE][KING] << std::endl;
        std::cout << "Black pawns count: " << (int)piece_count[BLACK][PAWN] << std::endl;
        std::cout << "Black rooks count: " << (int)piece_count[BLACK][ROOK] << std::endl;
        std::cout << "Black knights count: " << (int)piece_count[BLACK][KNIGHT] << std::endl;
        std::cout << "Black bishops count: " << (int)piece_count[BLACK][BISHOP] << std::endl;
        std::cout << "Black queens count: " << (int)piece_count[BLACK][QUEEN] << std::endl;
        std::cout << "Black king count: " << (int)piece_count[BLACK][KING] << std::endl << std::endl;

    }
}

std::vector<std::vector<std::pair<PieceType, Color>>> Board::getBoardMatrix(int ply) const {

    if (ply >= 0 && ply < pieceHistory.size()) {
        return pieceHistory[ply];
    }

    std::vector<std::vector<std::pair<PieceType, Color>>> boardMatrix;

    for (int rank = 7; rank >= 0; rank--)
    {
        std::vector<std::pair<PieceType, Color>> currentRow;

        for (int file = 0; file < 8; file++)
        {
            int sq = GetSquare(rank, file);

            PieceType piece_to_store = PieceType::PIECE_NONE;
            Color color_to_store = Color::WHITE;

            bool found = false;

            for (int color = WHITE; color <= BLACK; color++)
            {
                for (int piece_type = PAWN; piece_type <= KING; piece_type++)
                {
                    uint64_t current_bb = m_bitboards[color][piece_type];

                    if (current_bb & (1ULL << sq))
                    {
                        piece_to_store = (PieceType)piece_type;

                        color_to_store = (Color)color;

                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }

            currentRow.push_back({piece_to_store, color_to_store});
        }

        boardMatrix.push_back(currentRow);
    }

    return boardMatrix;
}

void Board::getPieceCounts(int piecesOut[2][6], int ply) {
    for(int i = 0; i < 2; ++i)
        for(int  j= 0; j < 6; ++j) piecesOut[i][j] = 0;

    auto pieces = getBoardMatrix(ply);
    for (const auto& rowPieces : pieces) {
        for (const auto& piece : rowPieces) {
            if (piece.first == PieceType::PIECE_NONE) continue;
            piecesOut[static_cast<int>(piece.second)][static_cast<int>(piece.first)]++;
        }
    }
}

MoveFlag Board::getMoveFlagBasedOnPromotionPiece(PieceType promotionPiece) {

    switch(promotionPiece){
    case PieceType::KNIGHT:
        return MoveFlag::PROMOTION_TYPE_KNIGHT;

    case PieceType::BISHOP:
        return MoveFlag::PROMOTION_TYPE_BISHOP;

    case PieceType::ROOK:
        return MoveFlag::PROMOTION_TYPE_ROOK;

    case PieceType::QUEEN:
        return MoveFlag::PROMOTION_TYPE_QUEEN;

    default:
        return MoveFlag::NORMAL_MOVE;
    }
}

std::string Board::convertMoveToSAN(int ply, bool addPieceCharToString) {
    int currentPly = (ply == -1) ? committedPly : ply;

    if (currentPly <= 0 || currentPly >= move_history.size()) {
        return "";
    }

    Move m = move_history[currentPly];

    MoveList movesBefore = legalMovesHistory[currentPly - 1];

    bool isCheck = wasMoveCheck(currentPly);

    return PGNFormatter::convertMoveToSAN(m, movesBefore, isCheck, addPieceCharToString);
}

std::vector<std::string> Board::getMoveHistroyInSAN() {

    std::vector<std::string> SANs;

    for (int movePly = 1; movePly <= committedPly; ++movePly) {
        SANs.push_back(convertMoveToSAN(movePly));
    }

    return SANs;
}

MoveList Board::generateCurrentLegalMoves() {
    MoveList moves;
    MoveList validMoves;
    MoveGenerator::GenerateMoves(*this, moves);

    for (Move m : moves) {
        if (MakeMove(m, true)) {
            UndoMove(m, true);
            validMoves.push_back(m);
        }
    }

    return validMoves;
}

void Board::currentPlayerGaveUp() {
    if (gr != GameResult::GAME_DID_NOT_END)
        return;

    Color lastPlayerMoved = (Color)(m_side_to_move ^ 1);
    gr = lastPlayerMoved == WHITE ? GameResult::BLACK_GAVE_UP : GameResult::WHITE_GAVE_UP;
}

GameResult Board::getGameResult() {
    if (gr != GameResult::GAME_DID_NOT_END)
        return gr;

    if (IsDraw() || IsInsufficientMaterial())
        return GameResult::DRAW;

    if (IsCheckMate()) {
        Color lastPlayerMoved = (Color)(m_side_to_move ^ 1);
        return (lastPlayerMoved == WHITE ? GameResult::WHITE_WON_WITH_CHECKMATE : GameResult::BLACK_WON_WITH_CHECKMATE);
    }

    return GameResult::GAME_DID_NOT_END;

}

void Board::ClearBoard() {
    committedPly = m_ply = 0;

    gr = GameResult::GAME_DID_NOT_END;

    beginnerFen = newPosFen;
    repetition_history.Clear();
    move_history.clear();
    pieceHistory.clear();
    checkHistory.clear();
    playerToMoveHistory.clear();
    legalMovesHistory.clear();
}

MoveInfo Board::getMoveInfo(int ply) {
    if (move_history.empty())
        return MoveInfo();

    int index;
    if (ply == -1) {
        index = static_cast<int>(move_history.size()) - 1;
    } else {
        index = ply;
    }

    if (index < 0 || index >= (int)move_history.size() || !move_history[index].isValid())
        return MoveInfo();

    Move m = move_history[index];
    MoveInfo mi;

    Square fromSquare = m.getFrom();
    Square toSquare = m.getTo();

    mi.fromFile = getFileFromSquare(fromSquare);
    mi.fromRank = getRankFromSquare(fromSquare);
    mi.toFile = getFileFromSquare(toSquare);
    mi.toRank = getRankFromSquare(toSquare);

    return mi;
}

void perft_thread_worker(Board board_copy, std::vector<Move> moves_to_test, int depth) {
    uint64_t local_nodes = 0;

    for (const auto& move : moves_to_test) {
        if (board_copy.MakeMove(move)) {
            local_nodes += board_copy.Perft(depth - 1);
            board_copy.UndoMove(move);
        }
    }

    global_node_count += local_nodes;
}

uint64_t Board::MultiThreadedPerft(int depth) {
    auto start_time = std::chrono::high_resolution_clock::now();

    MoveList root_moves;
    MoveGenerator::GenerateMoves(*this, root_moves);

    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    std::vector<std::thread> threads;
    std::vector<std::vector<Move>> move_chunks(num_threads);

    for (size_t i = 0; i < root_moves.size(); ++i) {
        move_chunks[i % num_threads].push_back(root_moves[i]);
    }

    global_node_count = 0;
    std::cout << "Inditas " << num_threads << " szalon..." << std::endl;

    for (size_t i = 0; i < num_threads; ++i) {
        if (move_chunks[i].empty()) continue;

        threads.emplace_back(perft_thread_worker, *this, move_chunks[i], depth);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    uint64_t nodes = global_node_count;
    double nps = nodes / elapsed.count();

    return global_node_count;
}

PieceType Board::GetPromotionPiece(Move m) {
    MoveFlag mf = m.getFlags();

    MoveFlag cleanPromotionFlag = static_cast<MoveFlag>(mf & ~CAPTURE_FLAG);

    return GetPromotionPiece(cleanPromotionFlag);
}

PieceType Board::GetPromotionPiece(MoveFlag promotion_piece) {
    switch (promotion_piece) {
    case PROMOTION_TYPE_KNIGHT:
        return KNIGHT;
        break;

    case PROMOTION_TYPE_BISHOP:
        return BISHOP;
        break;

    case PROMOTION_TYPE_ROOK:
        return ROOK;
        break;

    case PROMOTION_TYPE_QUEEN:
        return QUEEN;

    default:
        return PIECE_NONE;
        break;
    }
}
