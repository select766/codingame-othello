"""
与えられたモデルでプレイアウトを行い、棋譜を生成する
"""

import argparse

import numpy as np
from tqdm import tqdm
import tensorflow as tf
from othello_train import othello_train_cpp

# without batch dimension
INPUT_SHAPE = (8, 8, 3)
POLICY_SHAPE = (64,)
VALUE_SHAPE = (1,)


def run(model, batch_size, games):
    batch_board_repr = np.zeros((batch_size, ) + INPUT_SHAPE, dtype=np.float32)
    batch_policy_logits = np.zeros(
        (batch_size, ) + POLICY_SHAPE, dtype=np.float32)
    batch_value_logit = np.zeros(
        (batch_size, ) + VALUE_SHAPE, dtype=np.float32)
    pbar = tqdm(total=games)
    while (games_completed := othello_train_cpp.games_completed()) < games:
        if pbar.n != games_completed:
            pbar.update(games_completed - pbar.n)
        othello_train_cpp.proceed_playout(
            batch_board_repr, batch_policy_logits, batch_value_logit)
        predicted = model(batch_board_repr)
        batch_policy_logits[...] = predicted[0].numpy()
        batch_value_logit[...] = predicted[1].numpy()
    pbar.update(games_completed - pbar.n)
    pbar.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("savedmodel_dir")
    parser.add_argument("records")
    parser.add_argument("--device", default="/GPU:0")
    parser.add_argument("--batch_size", type=int, default=256)
    parser.add_argument("--playout_limit", type=int, default=64)
    parser.add_argument("--games", type=int, default=1000)
    args = parser.parse_args()
    with tf.device(args.device):
        model = tf.keras.models.load_model(args.savedmodel_dir)
        if othello_train_cpp.init_playout(args.records, args.batch_size, args.playout_limit) != 0:
            raise RuntimeError("othello_train_cpp.init_playout failed")
        run(model, args.batch_size, args.games)
        othello_train_cpp.end_playout()


if __name__ == '__main__':
    main()
