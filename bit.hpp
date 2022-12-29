using u64 = uint64_t;

u64 flip(const u64 bb) { return __builtin_bswap64(bb);}
u64 lsb(const u64 bb) { return __builtin_ctzll(bb);}
u64 count(const u64 bb) { return __builtin_popcountll(bb);}
u64 east(const u64 bb) { return (bb << 1) & ~0x0101010101010101ULL;}
u64 west(const u64 bb) { return (bb >> 1) & ~0x8080808080808080ULL;}
u64 north(const u64 bb) { return bb << 8;}
u64 south(const u64 bb) { return bb >> 8;}
u64 nw(const u64 bb) { return north(west(bb));}
u64 ne(const u64 bb) { return north(east(bb));}
u64 sw(const u64 bb) { return south(west(bb));}
u64 se(const u64 bb) { return south(east(bb));}