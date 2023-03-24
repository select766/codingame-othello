"""
C++ソースファイルに学習済みのDNN重みを埋め込む

重みをbase64にしたものと、構造体定義を出力する。

構造体定義はstdoutに出力されるので、
/src/dnn_evaluator_embed.hpp
の class DNNWeight 内にコピーする。
"""

import argparse
import base64
import numpy as np
import tensorflow as tf
from othello_train.model_v1 import OthelloModelV1
from othello_train.feat_v1 import INPUT_SHAPE


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("savedmodel_dir")
    parser.add_argument("dst_hpp")
    args = parser.parse_args()

    model = tf.keras.models.load_model(args.savedmodel_dir)
    prefix = "othello_model_v1/"
    suffix = ":0"

    struct_def = ""
    flat_binary = b""
    for w in model.weights:
        name = w.name
        name = name.removeprefix(prefix)
        name = name.removesuffix(suffix)
        name = name.replace("/", "_")
        array = w.numpy()
        size = array.size
        struct_def += f"float {name}[{size}];\n"
        flat_binary += array.tobytes()

    print(struct_def)
    hpp_src = f"""#ifndef _DNN_WEIGHT_
#define _DNN_WEIGHT_
const char dnn_weight_base64[] = "{base64.b64encode(flat_binary).decode("ascii")}";
#endif
"""
    with open(args.dst_hpp, "w", encoding="utf-8") as f:
        f.write(hpp_src)

if __name__ == "__main__":
    main()
