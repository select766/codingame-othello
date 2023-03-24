import tensorflow as tf
import numpy as np

from othello_train import board
from othello_train.model_v1 import OthelloModelV1

# def make_feat_nchw(record):
#     # 手番から見た盤面を作る
#     # 盤の内側かどうかを判定するための、1で埋めたチャンネルを用意
#     ba = board.get_board_array(record["board"])
#     if record["turn"] == board.WHITE:
#         ba = ba[::-1]
#     feat = np.zeros((3, 8, 8), dtype=np.float32)
#     feat[:2] = ba
#     feat[2] = 1
#     return feat


def make_feat_nhwc(record):
    # 手番から見た盤面を作る
    # 盤の内側かどうかを判定するための、1で埋めたチャンネルを用意
    ba = board.get_board_array(record["board"])
    if record["turn"] == board.WHITE:
        ba = ba[::-1]
    feat = np.zeros((8, 8, 3), dtype=np.float32)
    feat[:, :, 0] = ba[0]
    feat[:, :, 1] = ba[1]
    feat[:, :, 2] = 1
    return feat


INPUT_SHAPE = (8, 8, 3)


def encode_record(record):
    # feat, move, game_result
    # game_resultは、もとは手番からみた石の数の差。勝ち=1,負け=-1,引き分け=0に変換
    return (make_feat_nhwc(record),
            record["move"],
            np.clip(record["game_result"], -1, 1))
