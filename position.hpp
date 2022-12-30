#include <array>
#include <random>
#include "bit.hpp"

enum { Pawn, Knight, Bishop, Rook, Queen, King, None};

using namespace std;

// TODO: split keys into more readable smaller arrays
const array<u64, 849> keys = []() {
    mt19937_64 r;
    array<u64, 849> values;
    for (u64 &val : values) { val = r();}
    return values;
}();

//TODO: store hash
struct Position {
    array<int, 4> castling = {true, true, true, true};
    array<u64, 2> colour = {0xFFFFULL, 0xFFFF000000000000ULL};
    array<u64, 6> pieces = {0xFF00000000FF00ULL,
                            0x4200000000000042ULL,
                            0x2400000000000024ULL,
                            0x8100000000000081ULL,
                            0x800000000000008ULL,
                            0x1000000000000010ULL};
    u64 ep = 0x0ULL;
    int flipped = false;
    u64 hash = 0x0ULL;
};

constexpr int piece_on(const Position &pos, const int sq) {
    const u64 pc = 1ULL<<sq;
    for (int i=0; i<6; ++i) {if(pos.pieces[i] & pc){ return i;}}
    return None;
}

void flipPos(Position &pos) {
    pos.colour[0] = flip(pos.colour[0]);
    pos.colour[1] = flip(pos.colour[1]);
    for (int i=0; i<6; ++i) { pos.pieces[i] = flip(pos.pieces[i]);}
    pos.ep = flip(pos.ep);
    swap(pos.colour[0], pos.colour[1]);
    swap(pos.castling[0], pos.castling[2]);
    swap(pos.castling[1], pos.castling[3]);
    pos.hash = keys[848];
    pos.flipped = !pos.flipped;
}

// TODO: reduce using this
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

inline constexpr u64 get_key(const int pc, const int ally, const int sq){ return keys[(pc+6*ally)*64+sq];}