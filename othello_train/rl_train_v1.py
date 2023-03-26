# 強化学習用の学習機

import argparse
from tqdm import tqdm
import tensorflow as tf
import numpy as np

from othello_train import board
from othello_train.model_v1 import build_model
from othello_train.feat_v1 import INPUT_SHAPE, encode_record


class OthelloRecordSequence(tf.keras.utils.Sequence):
    def __init__(self, records, batch_size):
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


def generate_sequence_splits(path, batch_size, train_ratio=0.9):
    """
    データファイルをtrain/valに分割し、それぞれのOthelloRecordSequenceを生成する
    """
    records = np.fromfile(path, dtype=board.move_record_dtype)
    border = int(train_ratio * len(records))
    return (
        OthelloRecordSequence(records[:border], batch_size),
        OthelloRecordSequence(records[border:], batch_size),
    )


def make_loss_objects():
    policy_loss_object = tf.keras.losses.SparseCategoricalCrossentropy(
        from_logits=True)
    value_loss_object = tf.keras.losses.MeanSquaredError()
    return policy_loss_object, value_loss_object


def make_metric_objects(name_prefix):
    train_loss = tf.keras.metrics.Mean(name=f'{name_prefix}_loss')
    train_policy_loss = tf.keras.metrics.Mean(
        name=f'{name_prefix}_policy_loss')
    train_value_loss = tf.keras.metrics.Mean(name=f'{name_prefix}_value_loss')
    train_policy_accuracy = tf.keras.metrics.SparseCategoricalAccuracy(
        name=f'{name_prefix}_policy_accuracy')
    return train_loss, train_policy_loss, train_value_loss, train_policy_accuracy


def run_train(args):
    model = build_model(args.model, args.model_kwargs)
    model.load_weights(args.src_checkpoint)

    train_dataset, val_dataset = generate_sequence_splits(
        args.records, args.batch_size)

    optimizer = tf.keras.optimizers.Adam()

    policy_loss_object, value_loss_object = make_loss_objects()
    train_loss, train_policy_loss, train_value_loss, train_policy_accuracy = make_metric_objects(
        'train')
    val_loss, val_policy_loss, val_value_loss, val_policy_accuracy = make_metric_objects(
        'val')

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

    @tf.function
    def val_step(images, moves, game_results):
        # training=False is only needed if there are layers with different
        # behavior during training versus inference (e.g. Dropout).
        predictions_policy, predictions_value = model(images, training=False)
        policy_loss = policy_loss_object(moves, predictions_policy)
        value_loss = value_loss_object(
            game_results, tf.nn.tanh(predictions_value))
        loss = policy_loss * 0.5 + value_loss

        val_loss(loss)
        val_policy_loss(policy_loss)
        val_value_loss(value_loss)
        val_policy_accuracy(moves, predictions_policy)

    last_val_loss = 10000.0

    for epoch in range(args.epoch):
        train_loss.reset_states()
        train_policy_loss.reset_states()
        train_value_loss.reset_states()
        train_policy_accuracy.reset_states()
        val_loss.reset_states()
        val_policy_loss.reset_states()
        val_value_loss.reset_states()
        val_policy_accuracy.reset_states()

        for images, moves, game_results in tqdm(train_dataset):
            train_step(images, moves, game_results)

        for images, moves, game_results in tqdm(val_dataset):
            val_step(images, moves, game_results)

        print(
            f'Epoch {epoch + 1}, '
            f'Loss: {train_loss.result():.4f}, '
            f'Policy Loss: {train_policy_loss.result():.4f}, '
            f'Value Loss: {train_value_loss.result():.4f}, '
            f'Policy Accuracy: {train_policy_accuracy.result() * 100:.4f}, '
            f'Val Loss: {val_loss.result():.4f}, '
            f'Val Policy Loss: {val_policy_loss.result():.4f}, '
            f'Val Value Loss: {val_value_loss.result():.4f}, '
            f'Val Policy Accuracy: {val_policy_accuracy.result() * 100:.4f}, '
        )

        cur_val_loss = val_loss.result()
        if args.early_stop and not (cur_val_loss < last_val_loss):
            print('Value loss not improved')
            break
        last_val_loss = cur_val_loss

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
    parser.add_argument("--early_stop", action='store_true')
    args = parser.parse_args()

    with tf.device(args.device):
        run_train(args)


if __name__ == "__main__":
    main()
