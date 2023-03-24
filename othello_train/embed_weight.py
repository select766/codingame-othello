"""
C++ソースファイルに学習済みのDNN重みを埋め込む

重みをbase64にしたものと、構造体定義を出力する。

構造体定義はstdoutに出力されるので、
/src/dnn_evaluator_embed.hpp
の class DNNWeight 内にコピーする。
"""

import argparse
import base64
import re
import numpy as np
import tensorflow as tf


def model_to_weight_dict(savedmodel_dir):
    model = tf.keras.models.load_model(savedmodel_dir)
    prefix = "othello_model_v1/"
    suffix = ":0"

    weight_dict = {}
    for w in model.weights:
        name = w.name
        name = name.removeprefix(prefix)
        name = name.removesuffix(suffix)
        array = w.numpy()
        weight_dict[name] = array

    return weight_dict


def merge_bn_to_conv(weight_dict, prefix_conv, prefix_bn):
    gamma = weight_dict.pop(prefix_bn+"/gamma")
    beta = weight_dict.pop(prefix_bn+"/beta")
    moving_mean = weight_dict.pop(prefix_bn+"/moving_mean")
    moving_variance = weight_dict.pop(prefix_bn+"/moving_variance")
    kernel = weight_dict.pop(prefix_conv + "/kernel")
    bias = weight_dict.pop(prefix_conv + "/bias")
    inv_std = 1.0 / np.sqrt(moving_variance + 0.001)
    merged_kernel = kernel * inv_std * gamma
    merged_bias = (bias - moving_mean) * inv_std * gamma + beta
    weight_dict[prefix_conv + "/kernel"] = merged_kernel
    weight_dict[prefix_conv + "/bias"] = merged_bias


def merge_bn_to_conv_all(weight_dict):
    # xxx/batch_normalization(.*)/gamma などを xxx/conv2d(.*)/kernel などに統合したい
    prefix_to_bn = {}
    prefix_to_conv = {}
    for name in weight_dict.keys():
        m = re.match("^((.+)/batch_normalization(.*))/(.+)$", name)
        if m is not None:
            # "conv_bn_1" = "conv_bn_1/batch_normalization_1"
            prefix_to_bn[m.group(2)] = m.group(1)
        m = re.match("^((.+)/conv2d(.*))/(.+)$", name)
        if m is not None:
            # "conv_bn_1" = "conv_bn_1/conv2d_2"
            prefix_to_conv[m.group(2)] = m.group(1)
    for common_prefix, prefix_bn in prefix_to_bn.items():
        prefix_conv = prefix_to_conv[common_prefix]
        merge_bn_to_conv(weight_dict, prefix_conv, prefix_bn)

def embed_weight(weight_dict):
    struct_def = ""
    flat_binary = b""
    for name, array in weight_dict.items():
        name = name.replace("/", "_")
        size = array.size
        struct_def += f"float {name}[{size}];\n"
        flat_binary += array.tobytes()
    return struct_def, flat_binary

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("savedmodel_dir")
    parser.add_argument("dst_hpp")
    args = parser.parse_args()

    weight_dict = model_to_weight_dict(args.savedmodel_dir)
    merge_bn_to_conv_all(weight_dict)
    struct_def, flat_binary = embed_weight(weight_dict)

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
