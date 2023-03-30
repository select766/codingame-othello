import argparse
import tensorflow as tf

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src_savedmodel_dir")
    parser.add_argument("dst_tflite_file")
    args = parser.parse_args()
    converter = tf.lite.TFLiteConverter.from_saved_model(args.src_savedmodel_dir)
    tflite_model = converter.convert() # type: bytes
    with open(args.dst_tflite_file, "wb") as f:
        f.write(tflite_model)

if __name__ == "__main__":
    main()
