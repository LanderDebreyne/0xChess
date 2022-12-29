#!/usr/bin/env python3

from puzzledb import PuzzleDb
from puzzlerunner import run_puzzle

import argparse
from pathlib import Path
from random import Random

import glicko2


def select_puzzles(player, puzzle_db, num_puzzles, played_puzzles, random):
    rd = player.rd
    candidates = []

    while len(candidates) < num_puzzles:
        candidates = puzzle_db \
            .filter(lambda p: p.rating >= player.rating - 2 * rd) \
            .filter(lambda p: p.rating <= player.rating + 2 * rd) \
            .filter(lambda p: p not in played_puzzles) \
            .collect()

        rd += 50

    return sorted(random.sample(candidates, num_puzzles),
                  key=lambda p: p.rating)


def play_round(player, puzzle_db, num_puzzles, engine,
               timeout, played_puzzles, random):
    puzzles = select_puzzles(player, puzzle_db, num_puzzles,
                             played_puzzles, random)
    played_puzzles.update(puzzles)
    outcomes = []

    for puzzle in puzzles:
        print(f'Running puzzle {puzzle.puzzle_id} '
              f'with rating {puzzle.rating}... ',
              end='', flush=True)

        result = run_puzzle(puzzle, engine, timeout)
        success = result.is_success()
        outcomes.append(success)

        if success:
            print('OK')
        else:
            print('FAIL')

    ratings = [p.rating for p in puzzles]
    rds = [p.rd for p in puzzles]
    player.update_player(ratings, rds, outcomes)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=60)
    parser.add_argument('--min-plays', type=int, default=500)
    parser.add_argument('--min-popularity', type=int, default=50)
    parser.add_argument('--rounds', type=int, default=20)
    parser.add_argument('--puzzles-per-round', type=int, default=5)
    parser.add_argument('--random-seed', type=int, default=0)
    parser.add_argument('--puzzle-db', type=Path, required=True)
    args = parser.parse_args()

    with args.puzzle_db.open(newline='') as f:
        puzzle_db = PuzzleDb.from_csv(f) \
            .filter(lambda p: p.num_plays >= args.min_plays) \
            .filter(lambda p: p.popularity >= args.min_popularity) \
            .sorted(key=lambda p: p.rating)

    player = glicko2.Player()
    played_puzzles = set()
    random = Random(args.random_seed)

    for current_round in range(1, args.rounds + 1):
        print(f'=== Round {current_round}/{args.rounds}, '
              f'current rating: {round(player.rating)} '
              f'(rd: {round(player.rd)}) ===')
        play_round(player, puzzle_db, args.puzzles_per_round,
                   args.engine, args.timeout, played_puzzles, random)

    print(f'Final rating: {round(player.rating)} (rd: {round(player.rd)})')


if __name__ == '__main__':
    main()
