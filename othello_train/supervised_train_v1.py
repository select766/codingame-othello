import tensorflow as tf
import numpy as np

from othello_train import board
from othello_train.model_v1 import build_model
from othello_train.feat_v1 import INPUT_SHAPE, encode_record


def load_dataset(path):
    records = np.fromfile(path, dtype=board.move_record_dtype)
    all_feats = np.zeros((len(records),) + INPUT_SHAPE, dtype=np.float32) # NHWC
    all_moves = np.zeros((len(records),), dtype=np.int32)
    all_game_results = np.zeros((len(records), 1), dtype=np.float32)
    for i, record in enumerate(records):
        f, m, g = encode_record(record)
        all_feats[i] = f
        all_moves[i] = m
        all_game_results[i] = g
    return all_feats, all_moves, all_game_results


def main():
    train_feats, train_moves, train_game_results = load_dataset("dataset/alphabeta_train_1/train_shuffled.bin")
    val_feats, val_moves, val_game_results = load_dataset("dataset/alphabeta_train_1/val_shuffled.bin")
    train_ds = tf.data.Dataset.from_tensor_slices(
        (train_feats, train_moves, train_game_results)).batch(256)

    test_ds = tf.data.Dataset.from_tensor_slices((val_feats, val_moves, val_game_results)).batch(256)
    model = build_model("OthelloModelResNetV1", None)
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
