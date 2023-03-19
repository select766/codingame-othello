# 学習データセットをシャッフルする
# 同時に、合法手が1個の局面を削除

import argparse
import numpy as np
from . import board

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src")
    parser.add_argument("dst")
    parser.add_argument("--no_remove_one_legal_move", action="store_true", help="合法手が1個しかない局面を除かない(デフォルトでは取り除く)")
    parser.add_argument("--no_shuffle", action="store_true", help="シャッフルしない")
    args = parser.parse_args()

    data = np.fromfile(args.src, dtype=board.move_record_dtype)
    if not args.no_remove_one_legal_move:
        mask = [d['n_legal_moves'] >= 2 for d in data]
        data = data[mask]
    if not args.no_shuffle:
        data = data[np.random.permutation(len(data))]
    data.tofile(args.dst)

if __name__ == "__main__":
    main()
