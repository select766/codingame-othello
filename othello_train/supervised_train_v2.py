import argparse
import os
import tensorflow as tf
import numpy as np

from othello_train import board
from othello_train.model_v1 import build_model
from othello_train.feat_v1 import INPUT_SHAPE, encode_record


class OthelloRecordSequence(tf.keras.utils.Sequence):
    def __init__(self, records, batch_size):
        # フィルタとシャッフルは済んでいる
        self.records = records
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
        return all_feats, (all_moves, all_game_results)


def custom_tanh_loss(y_true, y_pred):
    y_pred_tanh = tf.nn.tanh(y_pred)
    return tf.keras.losses.mean_squared_error(y_true, y_pred_tanh)

# https://stackoverflow.com/questions/49127214/keras-how-to-output-learning-rate-onto-tensorboard


class LRTensorBoard(tf.keras.callbacks.TensorBoard):
    def __init__(self, log_dir, **kwargs):
        super().__init__(log_dir=log_dir, **kwargs)

    def on_epoch_end(self, epoch, logs=None):
        logs = logs or {}
        logs.update({'lr': self.model.optimizer.lr})
        super().on_epoch_end(epoch, logs)


def run_train(args):
    os.makedirs(args.model_dir, exist_ok=True)
    model = build_model(args.model, args.model_kwargs)

    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=1e-2), # バッチサイズ4096など大きい値を想定
        loss=[
            tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
            custom_tanh_loss
        ],
        loss_weights=[0.5, 1.0],
        metrics=[['accuracy'], []],
    )
    cp_callback_all = tf.keras.callbacks.ModelCheckpoint(
        filepath=args.model_dir + "/cp-{epoch:04d}.ckpt",
        verbose=1,
        save_weights_only=True,
        save_freq='epoch')
    cp_callback_best = tf.keras.callbacks.ModelCheckpoint(
        monitor='val_loss',
        filepath=args.model_dir + "/cp-best.ckpt",
        verbose=1,
        save_best_only=True,
        save_weights_only=True,
        save_freq='epoch')
    tensorboard_callback = LRTensorBoard(
        log_dir=args.model_dir + "/log")
    early_stop_callback = tf.keras.callbacks.EarlyStopping(
        monitor='val_loss', patience=5, min_delta=1e-3)

    train_dataset = OthelloRecordSequence(np.fromfile(
        args.train_data_dir + "/records_train.bin", dtype=board.move_record_dtype), args.batch_size)
    val_dataset = OthelloRecordSequence(np.fromfile(
        args.train_data_dir + "/records_val.bin", dtype=board.move_record_dtype), args.batch_size)

    reduce_lr = tf.keras.callbacks.ReduceLROnPlateau(
        monitor='val_loss', factor=0.1, patience=2, min_lr=1e-5)
    # verbose=1（デフォルト）だと、ターミナルを持っているVSCodeを最小化したあとしばらくすると進みが遅くなる？
    model.fit(train_dataset, validation_data=val_dataset, epochs=args.epoch, verbose=2, callbacks=[
              tensorboard_callback, reduce_lr, cp_callback_all, cp_callback_best, early_stop_callback])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("train_data_dir")
    parser.add_argument("model_dir")
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
