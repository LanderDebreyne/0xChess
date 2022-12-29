#include <array>
#include "bit.hpp"

enum { Pawn, Knight, Bishop, Rook, Queen, King, None};

using namespace std;

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
};

int piece_on(const Position &pos, const int sq) {
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
    pos.flipped = !pos.flipped;
}