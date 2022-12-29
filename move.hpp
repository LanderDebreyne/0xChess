struct Move { int from = 0, to = 0, promo = 0; };
const Move no_move{};

bool operator==(const Move &lhs, const Move &rhs) { return !memcmp(&rhs, &lhs, sizeof(Move));}

string toUci(const Move &move, const int flip) {
    string str;
    str += 'a'+(move.from%8);
    str += '1'+(flip?(7-move.from/8):(move.from/8));
    str += 'a'+(move.to%8);
    str += '1'+(flip?(7-move.to/8):(move.to/8));
    if (move.promo!=None) {str += "\0nbrq\0\0"[move.promo];}
    return str;
}

struct Stack {
    Move moves[256];
    Move move;
    Move killer;
    int score;
};