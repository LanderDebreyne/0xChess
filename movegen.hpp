//TODO: magic bitboards

template <typename F>
u64 ray(const int sq, const u64 blockers, F f) {
    u64 mask = f(1ULL << sq);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    return mask;
}

static constexpr u64 knights[64] {
        132096, 329728, 659712, 1319424, 2638848, 5277696, 10489856, 4202496, 33816580,
        84410376, 168886289, 337772578, 675545156, 1351090312, 2685403152, 1075839008, 8657044482,
        21609056261, 43234889994, 86469779988, 172939559976, 345879119952, 687463207072, 275414786112, 2216203387392,
        5531918402816, 11068131838464, 22136263676928, 44272527353856,
        88545054707712, 175990581010432, 70506185244672, 567348067172352,
        1416171111120896, 2833441750646784, 5666883501293568, 11333767002587136,
        22667534005174272, 45053588738670592, 18049583422636032, 145241105196122112,
        362539804446949376, 725361088165576704, 1450722176331153408,
        2901444352662306816, 5802888705324613632, 11533718717099671552ULL, 4620693356194824192ULL, 288234782788157440ULL,
        576469569871282176, 1224997833292120064, 2449995666584240128,
        4899991333168480256, 9799982666336960512ULL, 1152939783987658752ULL, 2305878468463689728ULL, 1128098930098176ULL,
        2257297371824128, 4796069720358912, 9592139440717824, 19184278881435648,
        38368557762871296, 4679521487814656, 9077567998918656
    };

u64 knight(const int sq, const u64) { return knights[sq];}
u64 bishop(const int sq, const u64 blockers) { return ray(sq, blockers, nw) | ray(sq, blockers, ne) | ray(sq, blockers, sw) | ray(sq, blockers, se);}
u64 rook(const int sq, const u64 blockers) { return ray(sq, blockers, north) | ray(sq, blockers, east) | ray(sq, blockers, south) | ray(sq, blockers, west);}
u64 king(const int sq, const u64) {
    const u64 k=1ULL<<sq;
    return (k<<8) | (k>>8) | (((k>>1) | (k>>9) | (k<<7)) & 0x7F7F7F7F7F7F7F7FULL) | (((k<<1) | (k<<9) | (k>>7)) & 0xFEFEFEFEFEFEFEFEULL);
}

u64 attacked(const Position &pos, const int sq, const int them = true) {
    const u64 pc = 1ULL << sq;
    const u64 kt = pos.colour[them] & pos.pieces[Knight];
    const u64 BQ = pos.pieces[Bishop] | pos.pieces[Queen];
    const u64 RQ = pos.pieces[Rook] | pos.pieces[Queen];
    const u64 pawns = pos.colour[them] & pos.pieces[Pawn];
    const u64 pawn_attacks = them ? sw(pawns) | se(pawns) : nw(pawns) | ne(pawns);
    return (pawn_attacks & pc) | (kt & knight(sq, 0)) |
           (bishop(sq, pos.colour[0] | pos.colour[1]) & pos.colour[them] & BQ) |
           (rook(sq, pos.colour[0] | pos.colour[1]) & pos.colour[them] & RQ) |
           (king(sq, 0) & pos.colour[them] & pos.pieces[King]);
}

inline constexpr u64 square(const Position &pos, u64 sq){
    return pos.flipped ? flip(sq) : sq;
}

u64 makemove(Position &pos, const Move &move) {
    const int piece = piece_on(pos, move.from);
    const int captured = piece_on(pos, move.to);
    const u64 to = 1ULL<<move.to;
    const u64 from = 1ULL<<move.from;
    const int ally = pos.flipped ? 1 : 0;
    const int enemy = pos.flipped ? 0 : 1;
    const int hf = lsb(square(pos, from));
    const int ht = lsb(square(pos, to));
    pos.hash ^= get_key(piece, ally, hf) ^ get_key(piece, ally , ht);
    pos.colour[0] ^= from | to;
    pos.pieces[piece] ^= from | to;
    if (piece == Pawn && to == pos.ep) {
        pos.hash ^= get_key(Pawn, enemy, hf) ^ get_key(Pawn, enemy, ht);
        pos.colour[1] ^= to >> 8;
        pos.pieces[Pawn] ^= to >> 8;
    }
    // TODO update hash for ep
    pos.hash ^= keys[768+lsb(pos.flipped ? flip(pos.ep) : pos.ep)];
    pos.ep = (piece == Pawn && move.to - move.from == 16) ? to>>8 :  0x0ULL;
    pos.hash ^= keys[768+lsb(pos.flipped ? flip(pos.ep) : pos.ep)];
    if (captured != None) {
        pos.hash ^= get_key(captured, enemy, ht);
        pos.colour[1] ^= to;
        pos.pieces[captured] ^= to;
    }
    if (piece == King) {
        const u64 rook = move.to-move.from==2 ? 0xa0ULL : move.to-move.from==-2 ? 0x9ULL : 0x0ULL;
        pos.hash ^= get_key(Rook, ally, lsb(pos.flipped ? flip(rook) : rook));
        pos.colour[0] ^= rook;
        pos.pieces[Rook] ^= rook;
    }
    if (piece == Pawn && move.to >= 56) {
        pos.hash ^= get_key(Pawn, ally, ht) ^ get_key(move.promo, ally, ht);
        pos.pieces[Pawn] ^= to;
        pos.pieces[move.promo] ^= to;
    }
    pos.hash ^= keys[832 + (pos.castling[0]|pos.castling[1]<<1|pos.castling[2]<<2|pos.castling[3]<<3)];
    pos.castling[0] &= !((from | to) & 0x90ULL);
    pos.castling[1] &= !((from | to) & 0x11ULL);
    pos.castling[2] &= !((from | to) & 0x9000000000000000ULL);
    pos.castling[3] &= !((from | to) & 0x1100000000000000ULL);
    pos.hash ^= keys[832 + (pos.castling[0]|pos.castling[1]<<1|pos.castling[2]<<2|pos.castling[3]<<3)];
    flipPos(pos);
    return !attacked(pos, lsb(pos.colour[1] & pos.pieces[King]), false);
}

//TODO: update hash
u64 unmakemove(Position &pos, const Move &move) {
    const int piece = piece_on(pos, move.from);
    const int captured = piece_on(pos, move.to);
    const u64 to = 1ULL<<move.to;
    const u64 from = 1ULL<<move.from;
    pos.colour[0] ^= from | to;
    pos.pieces[piece] ^= from | to;
    if (piece == Pawn && to == pos.ep) {
        pos.colour[1] ^= to >> 8;
        pos.pieces[Pawn] ^= to >> 8;
    }
    pos.ep = (piece == Pawn && move.to - move.from == 16) ? to>>8 :  0x0ULL;
    if (captured != None) {
        pos.colour[1] ^= to;
        pos.pieces[captured] ^= to;
    }
    if (piece == King) {
        const u64 rook = move.to-move.from==2 ? 0xa0ULL : move.to-move.from==-2 ? 0x9ULL : 0x0ULL;
        pos.colour[0] ^= rook;
        pos.pieces[Rook] ^= rook;
    }
    if (piece == Pawn && move.to >= 56) {
        pos.pieces[Pawn] ^= to;
        pos.pieces[move.promo] ^= to;
    }
    pos.castling[0] &= !((from | to) & 0x90ULL);
    pos.castling[1] &= !((from | to) & 0x11ULL);
    pos.castling[2] &= !((from | to) & 0x9000000000000000ULL);
    pos.castling[3] &= !((from | to) & 0x1100000000000000ULL);
    flipPos(pos);
    return !attacked(pos, lsb(pos.colour[1] & pos.pieces[King]), false);
}

//TODO: 
// makenull()
// unmakenull()

void add_move(Move *const movelist, int &num_moves, const int from, const int to, const int promo = None) {movelist[num_moves++] = Move{from, to, promo}; }

void generate_pawn_moves(Move *const movelist, int &num_moves, u64 to_mask, const int offset) {
    while (to_mask) {
        const int to = lsb(to_mask);
        to_mask &= to_mask-1;
        if (to >= 56) {
            add_move(movelist, num_moves, to + offset, to, Queen);
            add_move(movelist, num_moves, to + offset, to, Rook);
            add_move(movelist, num_moves, to + offset, to, Bishop);
            add_move(movelist, num_moves, to + offset, to, Knight);
        } else {
            add_move(movelist, num_moves, to + offset, to);
        }
    }
}

void generate_piece_moves(Move *const movelist, int &num_moves, const Position &pos, const int piece, const u64 to_mask, u64 (*func)(int, u64)) {
    u64 copy = pos.colour[0] & pos.pieces[piece];
    while (copy) {
        const int fr = lsb(copy);
        copy &= copy - 1;
        u64 moves = func(fr, pos.colour[0]|pos.colour[1]) & to_mask;
        while (moves) {
            const int to = lsb(moves);
            moves &= moves - 1;
            add_move(movelist, num_moves, fr, to);
        }
    }
}

int movegen(const Position &pos, Move *const movelist, const bool only_captures) {
    int num_moves = 0;
    const u64 all = pos.colour[0]|pos.colour[1];
    const u64 to_mask = only_captures?pos.colour[1]:~pos.colour[0];
    const u64 pawns = pos.colour[0] & pos.pieces[Pawn];
    generate_pawn_moves(movelist, num_moves, north(pawns)&~all&(only_captures?0xFF00000000000000ULL:~0ULL), -8);
    if (!only_captures) { generate_pawn_moves(movelist, num_moves, north(north(pawns & 0xFF00ULL) & ~all) & ~all, -16);}
    generate_pawn_moves(movelist, num_moves, nw(pawns) & (pos.colour[1] | pos.ep), -7);
    generate_pawn_moves(movelist, num_moves, ne(pawns) & (pos.colour[1] | pos.ep), -9);
    generate_piece_moves(movelist, num_moves, pos, Knight, to_mask, knight);
    generate_piece_moves(movelist, num_moves, pos, Bishop, to_mask, bishop);
    generate_piece_moves(movelist, num_moves, pos, Queen, to_mask, bishop);
    generate_piece_moves(movelist, num_moves, pos, Rook, to_mask, rook);
    generate_piece_moves(movelist, num_moves, pos, Queen, to_mask, rook);
    generate_piece_moves(movelist, num_moves, pos, King, to_mask, king);
    if (!only_captures && pos.castling[0] && !(all & 0x60ULL) && !attacked(pos, 4) && !attacked(pos, 5)) { add_move(movelist, num_moves, 4, 6);}
    if (!only_captures && pos.castling[1] && !(all & 0xEULL) && !attacked(pos, 4) && !attacked(pos, 3)) { add_move(movelist, num_moves, 4, 2);}
    return num_moves;
}