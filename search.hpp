#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include "position.hpp"
#include "move.hpp"
#include "hash.hpp"
#include "movegen.hpp"
#include "eval.hpp"

#define MATE_SCORE (1 << 15)
#define INF (1 << 16)

int64_t now() {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000+t.tv_nsec/1000000;
}

struct TTE {u64 key; Move move; int score; int depth; uint16_t flag;};
vector<TTE> tt;

// options
u64 tt_size = 1024ULL << 15;  // size of tt in megabytes
int thread_count = 8; // thread count

int search(Position &pos, int alpha, const int beta, int depth, const int ply, int64_t &nodes, const int64_t stop_time, int &stop, Stack *const stack, int64_t (&hh_table)[2][64][64], vector<u64> &hash_history, const int null = true) {
    const int static_eval = eval(pos);
    if (ply > 127) { return static_eval; }
    const bool qsearch = depth <= 0;
    const bool improving = ply>1 && static_eval>stack[ply-2].score;
    const u64 in_check = attacked(pos, lsb(pos.colour[0] & pos.pieces[King]));
    stack[ply].score = static_eval;
    depth = in_check ? max(1, depth+1):depth;
    if (qsearch&&static_eval>alpha) {
        if (static_eval >= beta) { return beta;}
        alpha = static_eval;
    }
    const u64 tt_key = get_hash(pos);
    // early pruning
    if (ply > 0 && !qsearch) {
        // 3-fold
        for (const u64 old_hash : hash_history) { if (old_hash == tt_key) { return 0;}}
        // Can't this be loosened up?
        if (!in_check && alpha==beta-1) {
            // Reverse futility
            if (depth<5) {
                const int margins[] = {0, 50, 100, 200, 300};
                if (static_eval - margins[depth-improving] >= beta) { return beta;}
            }
            // TODO: double null, etc...
            // Null move
            if (null && depth>2 && static_eval>=beta) {
                Position npos = pos;
                flipPos(npos);
                npos.ep = 0;
                if (-search(npos, -beta, -beta+1, depth-4-depth/6, ply+1, nodes, stop_time, stop, stack, hh_table, hash_history, false) >= beta) { return beta; }
            }
        }
    }
    // TODO: other tt replacement strat?
    TTE &tte = tt[tt_key%tt_size];
    Move tt_move{};
    // tt hit
    if (tte.key == tt_key) {
        tt_move = tte.move;
        if (ply>0 && tte.depth >= depth) {
            if (tte.flag == 0) {return tte.score; }
            if (tte.flag == 1 && tte.score <= alpha) {return tte.score; }
            if (tte.flag == 2 && tte.score >= beta) {return tte.score; }
        }
    } else if (depth > 3) { depth--;}
    // time
    if (stop || now() >= stop_time) { return 0;}
    auto &moves = stack[ply].moves;
    const int num_moves = movegen(pos, moves, qsearch);
    // Score moves
    int64_t move_scores[256];
    for (int j = 0; j < num_moves; ++j) {
        const int capture = piece_on(pos, moves[j].to);
        if (moves[j] == tt_move) { move_scores[j] = 1LL << 62;} 
        else if (capture != None) { move_scores[j] = ((capture + 1) * (1LL << 54)) - piece_on(pos, moves[j].from);} 
        else if (moves[j] == stack[ply].killer) { move_scores[j] = 1LL << 50;} 
        else move_scores[j] = hh_table[pos.flipped][moves[j].from][moves[j].to];
    }
    int quiet_moves_evaluated = 0;
    int moves_evaluated = 0;
    int best_score = -INF;
    Move best_move{};
    uint16_t tt_flag = 1;
    hash_history.emplace_back(tt_key);
    for (int i = 0; i < num_moves; ++i) {
        int best_move_index = i;
        for (int j = i; j < num_moves; ++j) { if (move_scores[j] > move_scores[best_move_index]) best_move_index = j;}
        const auto move = moves[best_move_index];
        const auto best_move_score = move_scores[best_move_index];
        moves[best_move_index] = moves[i];
        move_scores[best_move_index] = move_scores[i];
        // Delta pruning
        if (qsearch && !in_check && static_eval + 50 + max_material[piece_on(pos, move.to)] < alpha) {
            best_score = alpha;
            break;
        }
        // Forward futility pruning
        if (!qsearch && !in_check && !(move==tt_move) && static_eval+150*depth+max_material[piece_on(pos, move.to)]<alpha) {
            best_score = alpha;
            break;
        }
        auto npos = pos;
        if (!makemove(npos, move)) continue;
        nodes++;
        int score;
        if (qsearch || !moves_evaluated) { full_window: score = -search(npos, -beta, -alpha, depth-1, ply+1, nodes, stop_time, stop, stack, hh_table, hash_history);} 
        else {
        // LMR
        int reduction = depth>3 && moves_evaluated>3 && piece_on(pos, move.to) == None ? 1+moves_evaluated/16+depth/10+(alpha==beta-1)-improving : 0;
        zero_window:
            score = -search(npos, -alpha-1, -alpha, depth-reduction-1, ply+1, nodes, stop_time, stop, stack, hh_table, hash_history);
            if (reduction>0 && score>alpha) {
                reduction = 0;
                goto zero_window;
            }
            if (score>alpha && score<beta) goto full_window;
        }
        // time
        if (stop || now() >= stop_time) {
            hash_history.pop_back();
            return 0;
        }
        moves_evaluated++;
        if (piece_on(pos, move.to) == None) quiet_moves_evaluated++;
        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score > alpha) {
                tt_flag = 0;
                alpha = score;
                stack[ply].move = move;
            }
        } else if (!qsearch && !in_check && alpha==beta-1 && depth <= 3 && moves_evaluated >= (depth*3)+2 && static_eval<alpha-(50*depth) && best_move_score<(1LL<<50)) {
            best_score = alpha;
            break;
        }
        if (alpha>=beta) {
            tt_flag = 2; 
            const int capture = piece_on(pos, move.to);
            if (capture == None) {
                hh_table[pos.flipped][move.from][move.to] += depth * depth;
                stack[ply].killer = move;
            }
            break;
        }
        // Late move pruning based on quiet move count
        if (!in_check && alpha == beta - 1 && quiet_moves_evaluated > 3 + 2 * depth * depth) {
            break;
        }
    }
    hash_history.pop_back();
    // mate or draw
    if (best_score == -INF) return qsearch ? alpha : in_check ? ply - MATE_SCORE : 0;
    // TT
    if (tte.key != tt_key || depth >= tte.depth || tt_flag == 0) tte = TTE{tt_key, best_move == no_move ? tt_move : best_move, best_score, qsearch ? 0 : depth, tt_flag};
    return alpha;
}

bool is_pseudolegal_move(const Position &pos, const Move &move) {
    Move moves[256];
    const int num_moves = movegen(pos, moves, false);
    for (int i = 0; i < num_moves; ++i) { if (moves[i] == move) { return true;}}
    return false;
}

void print_pv(const Position &pos, const Move move, vector<u64> &hash_history) {
    // Check move pseudolegality
    if (!is_pseudolegal_move(pos, move)) {return;}
    // Check move legality
    auto npos = pos;
    if (!makemove(npos, move)) { return;}
    // Print current move
    cout << " " << toUci(move, pos.flipped);
    // Probe the TT in the resulting position
    const u64 tt_key = get_hash(npos);
    const TTE &tte = tt[tt_key%tt_size];
    // Only continue if the move was valid and comes from a PV search
    if (tte.key!=tt_key || tte.move==Move{} || tte.flag!=0) { return; }
    // Avoid infinite recursion on a repetition
    for (const u64 old_hash : hash_history) {if (old_hash == tt_key) { return;}}
    hash_history.emplace_back(tt_key);
    print_pv(npos, tte.move, hash_history);
    hash_history.pop_back();
}

Move id(Position &pos, vector<u64> &hash_history, int thread_id, const int64_t start_time, const int allocated_time, int &stop) {
    Stack stack[128] = {};
    int64_t hh_table[2][64][64] = {};
    int64_t nodes = 0;
    int score = 0;
    for (int i = 1; i < 128; ++i) {
        auto window = 40;
        auto research = 0;
    research:
        const auto newscore = search(pos, score-window, score+window, i, 0, nodes, start_time + allocated_time, stop, stack, hh_table, hash_history);
        // time
        if (now()>=start_time+allocated_time || stop) { break;}
        if (thread_id == 0) {
            const int64_t elapsed = now() - start_time;
            cout << "info";
            cout << " depth " << i;
            cout << " score cp " << newscore;
            if (newscore >= score + window) cout << " lowerbound"; 
            else if (newscore <= score - window) cout << " upperbound";
            cout << " time " << elapsed;
            cout << " nodes " << nodes;
            if (elapsed > 0) cout << " nps " << nodes*1000/elapsed;
            if (newscore>score-window) { 
                cout << " pv";
                print_pv(pos, stack[0].move, hash_history);
            }
            cout << endl;
        }
        if (newscore>=score+window || newscore<=score-window) {
            window <<= ++research;
            score = newscore;
            goto research;
        }
        score = newscore;
        if (!research && now()>=start_time+allocated_time/10) { break;}
    }
    return stack[0].move;
}

void set_fen(Position &pos, const string &fen) {
    if (fen == "startpos") {
        pos = Position();
        return;
    }
    pos.colour = {};
    pos.pieces = {};
    pos.castling = {};
    stringstream ss{fen};
    string in;
    ss >> in;
    int i = 56;
    for (const char c : in) {
        if (c >= '1' && c <= '8') i += c - '1' + 1;
        else if (c == '/') i -= 16;
        else {
            const int side = c=='p' || c=='n' || c=='b' || c=='r' || c=='q' || c=='k';
            const int piece = (c=='p' || c=='P') ? Pawn : (c=='n' || c=='N') ? Knight : (c=='b' || c=='B') ? Bishop : (c=='r' || c=='R') ? Rook : (c=='q' || c=='Q') ? Queen : King;
            pos.colour.at(side) ^= 1ULL << i;
            pos.pieces.at(piece) ^= 1ULL << i;
            i++;
        }
    }
    ss >> in;
    const bool black = in == "b";
    ss >> in;
    for (const auto c : in) {
        pos.castling[0] |= c == 'K';
        pos.castling[1] |= c == 'Q';
        pos.castling[2] |= c == 'k';
        pos.castling[3] |= c == 'q';
    }
    ss >> in;
    if (in != "-") {
        const int sq = in[0]-'a' + 8*(in[1]-'1');
        pos.ep = 1ULL << sq;
    }
    if (black) flipPos(pos);
}