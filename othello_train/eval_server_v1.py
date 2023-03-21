import argparse
import socket

import numpy as np
import tensorflow as tf

# without batch dimension
INPUT_SHAPE = (8, 8, 3)
INPUT_BYTE_LENGTH = int(np.prod(INPUT_SHAPE) * 4)
POLICY_SHAPE = (64,)
POLICY_BYTE_LENGTH = int(np.prod(POLICY_SHAPE) * 4)
VALUE_SHAPE = (1,)
VALUE_BYTE_LENGTH = int(np.prod(VALUE_SHAPE) * 4)
OUTPUT_BYTE_LENGTH = POLICY_BYTE_LENGTH + VALUE_BYTE_LENGTH


class DisconnectedError(RuntimeError):
    pass


def recv_until_size(sock, size) -> bytes:
    buf = b""
    while len(buf) < size:
        extbuf = sock.recv(min(4096, size - len(buf)))
        if len(extbuf) == 0:
            raise DisconnectedError
        buf += extbuf
    return buf


def read_input_array(sock):
    return np.frombuffer(recv_until_size(sock, INPUT_BYTE_LENGTH), dtype=np.float32).reshape((1,) + INPUT_SHAPE)


def request_loop(model, sock):
    try:
        while True:
            board_array = read_input_array(sock)
            predicted = model(board_array)
            policy_data = predicted[0].numpy()
            value_data = predicted[1].numpy()
            send_data = policy_data.tobytes() + value_data.tobytes()
            assert len(send_data) == OUTPUT_BYTE_LENGTH
            sock.sendall(send_data)
    except DisconnectedError:
        pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("savedmodel_dir")
    # バッチサイズ1のため、CPU (/CPU:0)のほうがGPU (/GPU:0)より速いと予想される
    parser.add_argument("--device", default="/CPU:0")
    parser.add_argument("--port", type=int, default=8099)
    parser.add_argument("--host", default="")
    args = parser.parse_args()
    with tf.device(args.device):
        model = tf.keras.models.load_model(args.savedmodel_dir)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((args.host, args.port))
        sock.listen(1)
        print("listening on port", args.port)
        while True:
            conn, addr = sock.accept()
            print("connected by", addr)
            request_loop(model, conn)
            print("disconnected from", addr)


if __name__ == '__main__':
    main()
