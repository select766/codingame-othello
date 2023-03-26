# 強化学習用の学習機

import argparse
from tqdm import tqdm
import tensorflow as tf
import numpy as np

from othello_train import board
from othello_train.model_v1 import build_model
from othello_train.feat_v1 import INPUT_SHAPE, encode_record


class OthelloRecordSequence(tf.keras.utils.Sequence):
    def __init__(self, path, batch_size):
        records = np.fromfile(path, dtype=board.move_record_dtype)
        idxs = []
        for i, d in enumerate(records):
            # 合法手が1個の場合を除く
            if d['n_legal_moves'] >= 2:
                idxs.append(i)
        idxs = np.array(idxs, dtype=np.int32)
        # シャッフル
        np.random.shuffle(idxs)
        self.records = records[idxs]
        self.batch_size = batch_size

    def __len__(self):
        return len(self.records) // self.batch_size

    def __getitem__(self, idx):
        low = idx * self.batch_size
        high = (idx + 1) * self.batch_size
        cur_bs = high - low
        all_feats = np.zeros((cur_bs,) + INPUT_SHAPE, dtype=np.float32)  # NHWC
        all_moves = np.zeros((cur_bs,), dtype=np.int32)
        all_game_results = np.zeros((cur_bs, 1), dtype=np.float32)
        for i in range(cur_bs):
            f, m, g = encode_record(self.records[i+low])
            all_feats[i] = f
            all_moves[i] = m
            all_game_results[i] = g
        return all_feats, all_moves, all_game_results


def run_train(args):
    model = build_model(args.model, args.model_kwargs)
    model.load_weights(args.src_checkpoint)

    train_dataset = OthelloRecordSequence(args.records, args.batch_size)

    optimizer = tf.keras.optimizers.Adam()

    policy_loss_object = tf.keras.losses.SparseCategoricalCrossentropy(
        from_logits=True)
    value_loss_object = tf.keras.losses.MeanSquaredError()
    train_loss = tf.keras.metrics.Mean(name='train_loss')
    train_policy_loss = tf.keras.metrics.Mean(name='train_policy_loss')
    train_value_loss = tf.keras.metrics.Mean(name='train_value_loss')
    train_policy_accuracy = tf.keras.metrics.SparseCategoricalAccuracy(
        name='train_policy_accuracy')

    @tf.function
    def train_step(images, moves, game_results):
        with tf.GradientTape() as tape:
            # training=True is only needed if there are layers with different
            # behavior during training versus inference (e.g. Dropout).
            predictions_policy, predictions_value = model(
                images, training=True)
            policy_loss = policy_loss_object(moves, predictions_policy)
            value_loss = value_loss_object(
                game_results, tf.nn.tanh(predictions_value))
            loss = policy_loss * 0.5 + value_loss  # 1:1の比率だとvalueがchance rateから動かない
        gradients = tape.gradient(loss, model.trainable_variables)
        optimizer.apply_gradients(zip(gradients, model.trainable_variables))
        train_loss(loss)
        train_policy_loss(policy_loss)
        train_value_loss(value_loss)
        train_policy_accuracy(moves, predictions_policy)

    for epoch in range(args.epoch):
        train_loss.reset_states()
        train_policy_loss.reset_states()
        train_value_loss.reset_states()
        train_policy_accuracy.reset_states()

        for images, moves, game_results in tqdm(train_dataset):
            train_step(images, moves, game_results)

        print(
            f'Epoch {epoch + 1}, '
            f'Loss: {train_loss.result()}, '
            f'Policy Loss: {train_policy_loss.result()}, '
            f'Value Loss: {train_value_loss.result()}, '
            f'Policy Accuracy: {train_policy_accuracy.result() * 100}, '
        )

    model.save_weights(args.dst_checkpoint)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src_checkpoint")
    parser.add_argument("dst_checkpoint")
    parser.add_argument("records")
    parser.add_argument("--model", required=True)
    parser.add_argument("--model_kwargs")
    parser.add_argument("--epoch", type=int, default=1)
    parser.add_argument("--device", default="/GPU:0")
    parser.add_argument("--batch_size", type=int, default=256)
    args = parser.parse_args()

    with tf.device(args.device):
        run_train(args)


if __name__ == "__main__":
    main()
