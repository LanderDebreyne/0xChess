#!/usr/bin/env python3

import csv
from pathlib import Path
import argparse
import sys

import chess


class Puzzle:
    def __init__(self, puzzle_id, fen, moves, rating, rd, popularity,
                 num_plays, tags, game_url):
        self.puzzle_id = puzzle_id
        self.fen = fen
        self.moves = moves
        self.rating = rating
        self.rd = rd
        self.popularity = popularity
        self.num_plays = num_plays
        self.tags = tags
        self.game_url = game_url

    @staticmethod
    def from_csv(row):
        puzzle_id, fen, moves, rating, rd, popularity, \
            num_plays, tags, game_url = row[:9]
        moves = [chess.Move.from_uci(m) for m in moves.split()]
        return Puzzle(puzzle_id, fen, moves, int(rating), int(rd),
                      int(popularity), int(num_plays), tags.split(), game_url)

    def to_csv(self):
        moves = ' '.join(m.uci() for m in self.moves)
        tags = ' '.join(self.tags)

        return (
            self.puzzle_id, self.fen, moves, self.rating, self.rd,
            self.popularity, self.num_plays, tags, self.game_url
        )

    def has_tags(self, tags):
        return all(tag in self.tags for tag in tags)

    def has_not_tags(self, tags):
        return not any(tag in self.tags for tag in tags)

    def _has_move(self, predicate):
        board = chess.Board(self.fen)

        for move in self.moves:
            if predicate(board, move):
                return True

            board.push(move)

        return False

    def has_castling(self):
        return self._has_move(chess.Board.is_castling)

    def has_en_passant(self):
        return self._has_move(chess.Board.is_en_passant)

    def num_plies(self):
        return len(self.moves) - 1


class PuzzleDb:
    def __init__(self, puzzles):
        self._puzzles = puzzles

    @staticmethod
    def from_csv(file):
        def generate_puzzles():
            csv_reader = csv.reader(file)

            for row in csv_reader:
                puzzle = Puzzle.from_csv(row)
                yield puzzle

        return PuzzleDb(generate_puzzles())

    def to_csv(self, file):
        csv_writer = csv.writer(file)

        for puzzle in self._puzzles:
            csv_writer.writerow(puzzle.to_csv())

    def filter(self, predicate):
        return PuzzleDb(filter(predicate, self))

    def collect(self):
        return list(self)

    def sorted(self, key=None):
        return PuzzleDb(sorted(self._puzzles, key=key))

    def __iter__(self):
        yield from self._puzzles


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('puzzle_db', type=Path)
    parser.add_argument('--tag', action='append', dest='tags')
    parser.add_argument('--not-tag', action='append', dest='not_tags')
    parser.add_argument('--castling', choices=['yes', 'no', 'only'],
                        default='yes')
    parser.add_argument('--en-passant', choices=['yes', 'no', 'only'],
                        default='yes')
    parser.add_argument('--min-plies', type=int, default=0)
    parser.add_argument('--max-plies', type=int, default=1000)
    parser.add_argument('--min-rating', type=int, default=0)
    parser.add_argument('--max-rating', type=int, default=4000)
    parser.add_argument('--min-num-plays', type=int, default=0)
    args = parser.parse_args()

    with args.puzzle_db.open(newline='') as f:
        puzzles = PuzzleDb.from_csv(f)

        if args.tags is not None:
            puzzles = puzzles.filter(lambda p: p.has_tags(args.tags))

        if args.not_tags is not None:
            puzzles = puzzles.filter(lambda p: p.has_not_tags(args.not_tags))

        if args.castling == 'no':
            puzzles = puzzles.filter(lambda p: not p.has_castling())
        elif args.castling == 'only':
            puzzles = puzzles.filter(lambda p: p.has_castling())

        if args.en_passant == 'no':
            puzzles = puzzles.filter(lambda p: not p.has_en_passant())
        elif args.en_passant == 'only':
            puzzles = puzzles.filter(lambda p: p.has_en_passant())

        puzzles = puzzles.filter(
            lambda p: args.min_plies <= p.num_plies() <= args.max_plies
        )

        puzzles = puzzles.filter(
            lambda p: args.min_rating <= p.rating <= args.max_rating
        )

        puzzles = puzzles.filter(lambda p: args.min_num_plays <= p.num_plays)

        puzzles.to_csv(sys.stdout)


if __name__ == '__main__':
    main()
