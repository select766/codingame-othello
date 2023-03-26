import argparse
import numpy as np

from othello_train.model_v1 import build_model
from othello_train.feat_v1 import INPUT_SHAPE


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src_checkpoint")
    parser.add_argument("dst_savedmodel")
    parser.add_argument("--model", required=True)
    parser.add_argument("--model_kwargs")
    args = parser.parse_args()

    model = build_model(args.model, args.model_kwargs)
    model.load_weights(args.src_checkpoint)
    empty_feats = np.zeros((4, ) + INPUT_SHAPE, dtype=np.float32)
    model(empty_feats, training=False)
    model.save(args.dst_savedmodel)


if __name__ == "__main__":
    main()
