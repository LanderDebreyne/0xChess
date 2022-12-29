const array<u64, 848> keys = []() {
    mt19937_64 r;
    array<u64, 848> values;
    for (u64 &val : values) { val = r();}
    return values;
}();

u64 get_hash(const Position &pos) {
    u64 hash = pos.flipped;
    u64 copy = pos.colour[0]|pos.colour[1];
    while(copy) {
        const int sq = lsb(copy);
        copy &= copy-1;
        hash ^= keys[(piece_on(pos, sq)+6*((pos.colour[pos.flipped]>>sq)&1))*64+sq];
    }
    // En passant square
    if (pos.ep) {hash ^= keys[768+lsb(pos.ep)];}
    // Castling permissions
    hash ^= keys[832 + (pos.castling[0]|pos.castling[1]<<1|pos.castling[2]<<2|pos.castling[3]<<3)];
    return hash;
}