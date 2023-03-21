import tensorflow as tf
print("TensorFlow version:", tf.__version__)

from tensorflow.keras.layers import Dense, Flatten, Conv2D
from tensorflow.keras import Model
import numpy as np
import sys

from othello_train import board

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

def load_dataset(path):
    records = np.fromfile(path, dtype=board.move_record_dtype)
    all_feats = np.zeros((len(records), 8, 8, 3), dtype=np.float32) # NHWC
    all_moves = np.zeros((len(records),), dtype=np.int32)
    all_game_results = np.zeros((len(records),), dtype=np.float32)
    for i, record in enumerate(records):
        all_feats[i] = make_feat_nhwc(record)
        all_moves[i] = record["move"]
        all_game_results[i] = np.clip(record["game_result"], -1, 1) # もとは手番からみた石の数の差。勝ち=1,負け=-1,引き分け=0に変換
    return all_feats, all_moves, all_game_results

class MyModel(Model):
  def __init__(self):
    super(MyModel, self).__init__()
    # Conv2Dのデフォルトはdata_format="channels_last" (NHWC)
    # data_format="channels_first" (NCHW)はCPUでの推論が対応してない
    # The Conv2D op currently only supports the NHWC tensor format on the CPU. The op was given the format: NCHW
    self.conv1 = Conv2D(32, 3, activation='relu', padding="same")
    self.conv2 = Conv2D(32, 3, activation='relu', padding="same")
    self.conv3 = Conv2D(32, 3, activation='relu', padding="same")
    self.conv4 = Conv2D(32, 3, activation='relu', padding="same")
    self.conv5 = Conv2D(32, 3, activation='relu', padding="same")
    self.conv6 = Conv2D(1, 3, activation=None, padding="same")
    self.value_flatten = Flatten()
    self.value_fc_1 = Dense(16, activation='relu')
    self.value_fc_2 = Dense(1, activation=None)
    self.policy_flatten = Flatten()

  def call(self, x):
    x = self.conv1(x)
    x = self.conv2(x)
    x = self.conv3(x)
    x = self.conv4(x)
    x = self.conv5(x)
    p = x
    v = x
    p = self.conv6(p)
    p = self.policy_flatten(p)
    v = self.value_flatten(v)
    v = self.value_fc_1(v)
    v = self.value_fc_2(v)
    return p, v


def main():
    train_feats, train_moves, train_game_results = load_dataset("dataset/alphabeta_train_1/train_shuffled.bin")
    val_feats, val_moves, val_game_results = load_dataset("dataset/alphabeta_train_1/val_shuffled.bin")
    train_ds = tf.data.Dataset.from_tensor_slices(
        (train_feats, train_moves, train_game_results)).batch(256)

    test_ds = tf.data.Dataset.from_tensor_slices((val_feats, val_moves, val_game_results)).batch(256)
    model = MyModel()
    policy_loss_object = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)
    value_loss_object = tf.keras.losses.MeanSquaredError()

    optimizer = tf.keras.optimizers.Adam()

    train_loss = tf.keras.metrics.Mean(name='train_loss')
    train_policy_loss = tf.keras.metrics.Mean(name='train_policy_loss')
    train_value_loss = tf.keras.metrics.Mean(name='train_value_loss')
    train_policy_accuracy = tf.keras.metrics.SparseCategoricalAccuracy(name='train_policy_accuracy')

    test_loss = tf.keras.metrics.Mean(name='test_loss')
    test_policy_loss = tf.keras.metrics.Mean(name='test_policy_loss')
    test_value_loss = tf.keras.metrics.Mean(name='test_value_loss')
    test_policy_accuracy = tf.keras.metrics.SparseCategoricalAccuracy(name='test_policy_accuracy')


    @tf.function
    def train_step(images, moves, game_results):
        with tf.GradientTape() as tape:
            # training=True is only needed if there are layers with different
            # behavior during training versus inference (e.g. Dropout).
            predictions_policy, predictions_value = model(images, training=True)
            policy_loss = policy_loss_object(moves, predictions_policy)
            value_loss = value_loss_object(game_results, tf.nn.tanh(predictions_value))
            loss = policy_loss * 0.5 + value_loss # 1:1の比率だとvalueがchance rateから動かない
        gradients = tape.gradient(loss, model.trainable_variables)
        optimizer.apply_gradients(zip(gradients, model.trainable_variables))
        train_loss(loss)
        train_policy_loss(policy_loss)
        train_value_loss(value_loss)
        train_policy_accuracy(moves, predictions_policy)
    
    @tf.function
    def test_step(images, moves, game_results):
        # training=False is only needed if there are layers with different
        # behavior during training versus inference (e.g. Dropout).
        predictions_policy, predictions_value = model(images, training=False)
        policy_loss = policy_loss_object(moves, predictions_policy)
        value_loss = value_loss_object(game_results, tf.nn.tanh(predictions_value))
        loss = policy_loss * 0.5 + value_loss

        test_loss(loss)
        test_policy_loss(policy_loss)
        test_value_loss(value_loss)
        test_policy_accuracy(moves, predictions_policy)

    EPOCHS = 20

    for epoch in range(EPOCHS):
        # Reset the metrics at the start of the next epoch
        train_loss.reset_states()
        train_policy_loss.reset_states()
        train_value_loss.reset_states()
        train_policy_accuracy.reset_states()
        test_loss.reset_states()
        test_policy_loss.reset_states()
        test_value_loss.reset_states()
        test_policy_accuracy.reset_states()

        for images, moves, game_results in train_ds:
            train_step(images, moves, game_results)

        for images, moves, game_results in test_ds:
            test_step(images, moves, game_results)

        print(
            f'Epoch {epoch + 1}, '
            f'Loss: {train_loss.result()}, '
            f'Policy Loss: {train_policy_loss.result()}, '
            f'Value Loss: {train_value_loss.result()}, '
            f'Policy Accuracy: {train_policy_accuracy.result() * 100}, '
            f'Test Loss: {test_loss.result()}, '
            f'Test Policy Loss: {test_policy_loss.result()}, '
            f'Test Value Loss: {test_value_loss.result()}, '
            f'Test Policy Accuracy: {test_policy_accuracy.result() * 100}, '
        )
    
    model.save("model/alphabeta_supervised_model_v1")

if __name__ == "__main__":
    main()
