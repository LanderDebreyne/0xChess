int S(const int mg, const int eg) {return (eg << 16) + mg;}

const int phases[] = {0, 1, 1, 2, 4, 0};
const int max_material[] = {133, 418, 401, 603, 1262, 0, 0};
const int material[] = {S(80, 133), S(418, 292), S(401, 328), S(546, 603), S(1262, 1065), 0};
const int psts[][4] = {
    {S(-19, 0), S(-1, -2), S(6, 0), S(6, 11)},
    {S(-25, 2), S(-10, -2), S(18, 2), S(23, 1)},
    {S(-1, 1), S(-6, 1), S(-2, 2), S(3, 6)},
    {S(-21, 3), S(0, -15), S(-7, 15), S(21, 9)},
    {S(-4, -32), S(1, -15), S(-28, 19), S(22, 20)},
    {S(-49, 5), S(-4, -5), S(43, 0), S(4, 4)},
};
const int centralities[] = {S(9, -13), S(20, 16), S(26, 7), S(-2, 2), S(1, 27), S(-20, 17)};
const int outside_files[] = {S(3, -5), S(-2, -5), S(8, 0), S(-4, 1), S(-3, -5), S(-5, 2)};
const int pawn_protection[] = {S(15, 20), S(14, 17), S(-4, 18), S(1, 8), S(-5, 19), S(-43, 15)};
const int passers[] = {S(13, 8), S(23, -2), S(29, 12), S(26, 35), S(69, 103), S(154, 201)};
const int pawn_doubled = S(-23, -27);
const int pawn_passed_blocked = S(-5, -34);
const int pawn_passed_king_distance[] = {S(0, -3), S(-2, 5)};
const int bishop_pair = S(36, 57);
const int rook_open = S(74, 1);
const int rook_semi_open = S(35, 11);
const int rook_rank78 = S(34, 1);
const int king_shield[] = {S(36, -13), S(16, -15), S(-89, 30)};
const int pawn_attacked[] = {S(-64, -14), S(-55, -42)};

int eval(Position &pos) {
    int score = S(16, 8);
    int phase = 0;

    for (int c = 0; c < 2; ++c) {
        // our pawns, their pawns
        const u64 pawns[] = {pos.colour[0] & pos.pieces[Pawn], pos.colour[1] & pos.pieces[Pawn]};
        const u64 protected_by_pawns = nw(pawns[0]) | ne(pawns[0]);
        const u64 attacked_by_pawns = se(pawns[1]) | sw(pawns[1]);
        const int kings[] = {(int) lsb(pos.colour[0] & pos.pieces[King]), (int) lsb(pos.colour[1] & pos.pieces[King])};

        // Bishop pair
        if (count(pos.colour[0] & pos.pieces[Bishop]) == 2) {
            score += bishop_pair;
        }

        // For each piece type
        for (int p = 0; p < 6; ++p) {
            auto copy = pos.colour[0] & pos.pieces[p];
            while (copy) {
                phase += phases[p];

                const int sq = lsb(copy);
                copy &= copy - 1;
                const int rank = sq / 8;
                const int file = sq % 8;
                const int centrality = (7 - abs(7 - rank - file) - abs(rank - file)) / 2;

                // Material
                score += material[p];

                // Centrality
                score += centrality * centralities[p];

                // Closeness to outside files
                score += abs(file - 3) * outside_files[p];

                // Quadrant PSTs
                score += psts[p][(rank / 4) * 2 + file / 4];

                // Pawn protection
                const u64 piece_bb = 1ULL << sq;
                if (piece_bb & protected_by_pawns) {
                    score += pawn_protection[p];
                }
                if (~pawns[0] & piece_bb & attacked_by_pawns) {
                    // If we're to move, we'll just lose some options and our tempo.
                    // If we're not to move, we lose a piece?
                    score += pawn_attacked[c];
                }

                if (p == Pawn) {
                    // Passed pawns
                    u64 blockers = 0x101010101010101ULL << sq;
                    blockers = nw(blockers) | ne(blockers);
                    if (!(blockers & pawns[1])) {
                        score += passers[rank - 1];

                        // Blocked passed pawns
                        if (north(piece_bb) & pos.colour[1]) {
                            score += pawn_passed_blocked;
                        }

                        // King defense/attack
                        // king distance to square in front of passer
                        for (int i = 0; i < 2; ++i) {
                            score += pawn_passed_king_distance[i] * (rank - 1) *
                                     max(abs((kings[i] / 8) - (rank + 1)), abs((kings[i] % 8) - file));
                        }
                    }

                    // Doubled pawns
                    if ((north(piece_bb) | north(north(piece_bb))) & pawns[0]) {
                        score += pawn_doubled;
                    }
                } else if (p == Rook) {
                    // Rook on open or semi-open files
                    const u64 file_bb = 0x101010101010101ULL << file;
                    if (!(file_bb & pawns[0])) {
                        if (!(file_bb & pawns[1])) {
                            score += rook_open;
                        } else {
                            score += rook_semi_open;
                        }
                    }

                    // Rook on 7th or 8th rank
                    if (rank >= 6) {
                        score += rook_rank78;
                    }
                } else if (p == King && piece_bb & 0xE7) {
                    const u64 shield = file < 3 ? 0x700 : 0xE000;
                    score += count(shield & pawns[0]) * king_shield[0];
                    score += count(north(shield) & pawns[0]) * king_shield[1];

                    // C3D7 = Reasonable king squares
                    score += !(piece_bb & 0xC3D7) * king_shield[2];
                }
            }
        }

        flipPos(pos);

        score = -score;
    }

    // Tapered eval
    return ((short)score * phase + ((score + 0x8000) >> 16) * (24 - phase)) / 24;
}