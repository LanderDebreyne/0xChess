#include <cstdint>
#include "search.hpp"

int main( const int argc, const char **argv) {
    (void) argc;
    (void) argv;
    setbuf(stdout, 0);
    Position pos;
    vector<u64> hash_history;
    Move moves[256];
    string command;

    // UCI info
    cin >> command;
    cout << "id name 0xChess" << endl << "id author Lander Debreyne" << endl << "uciok" << endl;

    tt.resize(tt_size);

    while (true) {
        cin >> command;
        if (command=="quit" || !cin.good()) { break;} 
        else if (command == "ucinewgame") { tt.clear(); tt.resize(tt_size);}
        else if (command == "isready") { cout << "readyok" << endl; }
        else if (command == "setoption") { cin >> command >> command; }
        else if (command == "go") {
            int wtime;
            int btime;
            cin >> command >> wtime >> command >> btime;
            if (command == "wtime") { swap(wtime, btime);}
            const int64_t start = now();
            // TODO: smarter timecontrol
            const int64_t allocated_time = (pos.flipped ? btime : wtime) / 3;
            vector<thread> threads;
            vector<int> stops(thread_count, false);
            // TODO: check pos copy for each thread
            for (int i=1; i<thread_count; ++i) { 
                Position t_pos = pos;
                threads.emplace_back([=, &stops]() mutable { id(t_pos, hash_history, i, start, 1<<30, stops[i]);});
            }
            const Move best_move = id(pos, hash_history, 0, start, allocated_time, stops[0]);
            // stop other threads
            for (int i=1; i<thread_count; ++i) { stops[i] = true;}
            for (int i=1; i<thread_count; ++i) { threads[i-1].join();}
            cout << "bestmove " << toUci(best_move, pos.flipped) << endl;
        } else if (command == "position") {
            // Set to startpos
            pos = Position();
            pos.hash = get_hash(pos);
            hash_history.clear();

            string fen;
            int fen_size = 0;

            // FEN
            while (fen_size < 6 && cin >> command) {
                if (command == "moves" || command == "startpos") { break;} 
                else if (command != "fen") {
                    if (fen.empty()) { fen = command;} 
                    else { fen += " " + command;}
                    fen_size++;
                }
            }
            if (!fen.empty()) { set_fen(pos, fen); }
        } else {
            const int num_moves = movegen(pos, moves, false);
            for (int i = 0; i < num_moves; ++i) {
                if (command == toUci(moves[i], pos.flipped)) {
                    if (piece_on(pos, moves[i].to) != None || piece_on(pos, moves[i].from) == Pawn) { hash_history.clear();} 
                    else { hash_history.emplace_back(get_hash(pos));}
                    makemove(pos, moves[i]);
                    break;
                }
            }
        }
    }
}