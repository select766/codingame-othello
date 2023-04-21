# CodinGame提出用の実行ファイル同士を対戦させる

import argparse
import sys
import subprocess
import os
from tqdm import tqdm
import json
from multiprocessing import Pool
import othello_train.othello_train_cpp as otc


class Engine:
    def __init__(self, path: str) -> None:
        cwd = os.path.dirname(path)
        name = os.path.abspath(path)
        self.proc = subprocess.Popen(
            name, stdin=subprocess.PIPE, stdout=subprocess.PIPE, cwd=cwd)

    def writeline(self, line: str) -> None:
        self.proc.stdin.write((line + "\n").encode("ascii"))
        self.proc.stdin.flush()

    def readline(self) -> str:
        return self.proc.stdout.readline().decode("ascii")

    def close(self) -> None:
        if self.proc is not None:
            self.proc.stdin.close()
            try:
                self.proc.wait(timeout=0.01)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None

    def __del__(self) -> None:
        self.close()

def run_one_game(args):
    # エンジンに新規ゲームの開始を伝える手段がないので、ゲームごとにプロセスを起動する
    # 同じプロセスで複数のゲームを行うと、置換表が満杯になるなどの不具合が起きる
    black_engine = args["black_engine"]
    engines = [Engine(args["engine1"]), Engine(args["engine2"])]
    board = otc.Board()
    for i, engine in enumerate(engines):
        color = i ^ black_engine
        engine.writeline(f"{color}")  # my_color
        engine.writeline("8")  # board_size

    current_engine = black_engine
    move_history = []
    while not board.is_gameover():
        engine = engines[current_engine]
        legal_moves_ints = board.legal_moves()
        if len(legal_moves_ints) == 0:
            move = otc.MOVE_PASS
        else:
            board_lines = board.get_position_codingame()
            legal_moves_strs = [otc.move_to_str(
                m) for m in legal_moves_ints]
            for line in board_lines:
                engine.writeline(line)
            engine.writeline(f"{len(legal_moves_strs)}")  # action_count
            for lms in legal_moves_strs:
                engine.writeline(lms)
            # wait bestmove
            bestmove_line = engine.readline()  # c3 MSG xxx
            bestmove = bestmove_line.split(" ")[0]
            move = otc.move_from_str(bestmove)
        board.do_move(move)
        move_history.append(otc.move_to_str(move))
        current_engine = 1 - current_engine
    winner = board.winner()
    record = {
        "black_engine": black_engine,
        "moves": move_history,
        "winner": winner,  # BLACK/WHITE/DRAW
    }
    print(".", end="", file=sys.stderr, flush=True)
    return record

class MatchExe:
    def __init__(self, args) -> None:
        self.engine1 = args.engine1
        self.engine2 = args.engine2
        self.games = args.games
        self.out = args.out
        self.parallel = args.parallel

    def run(self) -> None:
        args_list = []
        for game in range(self.games):
            args_list.append({
                "black_engine": game % 2,
                "engine1": self.engine1,
                "engine2": self.engine2,
            })
        with Pool(self.parallel) as p:
            records = p.map(run_one_game, args_list)
        print("", file=sys.stderr, flush=True)
        stats = self._calc_stats(records)
        match_result = {
            "engines": [self.engine1, self.engine2],
            "stats": stats,
            "records": records,
        }
        self._print_stats(stats)
        json.dump(match_result, self.out)

    def _calc_stats(self, records):
        engine_win_count = [0, 0, 0]  # engine1, engine2, draw
        color_win_count = [0, 0, 0]  # black, white, draw
        for record in records:
            winner = record["winner"]  # color
            if winner == 2:  # draw
                win_engine = 2
            else:
                win_engine = record["black_engine"] ^ winner
            engine_win_count[win_engine] += 1
            color_win_count[winner] += 1
        return {"engine_win_count": engine_win_count, "color_win_count": color_win_count}

    def _print_stats(self, stats):
        print(f"""{self.engine1} - {self.engine2}: {stats["engine_win_count"][0]} - {stats["engine_win_count"][2]} - {stats["engine_win_count"][1]}
BLACK - WHITE: {stats["color_win_count"][0]} - {stats["color_win_count"][2]} - {stats["color_win_count"][1]}
""", file=sys.stderr)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("engine1")
    parser.add_argument("engine2")
    parser.add_argument("--games", type=int, default=1)
    parser.add_argument("--out", help="output json file", type=argparse.FileType('w'),
                        default=sys.stdout)
    parser.add_argument("--parallel", type=int, default=1)
    args = parser.parse_args()
    MatchExe(args).run()


if __name__ == "__main__":
    main()
