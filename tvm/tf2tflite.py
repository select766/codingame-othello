import argparse
import tensorflow as tf

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src_savedmodel_dir")
    parser.add_argument("dst_tflite_file")
    parser.add_argument("--int8", action="store_true")
    parser.add_argument("--float16", action="store_true")
    
    args = parser.parse_args()
    converter = tf.lite.TFLiteConverter.from_saved_model(args.src_savedmodel_dir)
    if args.int8:
        # ダイナミックレンジの量子化。モデルパラメータの保存は8bit整数にするが、推論時はfloat32に展開。しかしTVMがこのモードに対応していない。
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
    if args.float16:
        # モデルパラメータの保存をfloat16にする。しかしTVMでコンパイルした時点で、ソースにはfloat32として埋め込まれてしまい効果がない。
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        converter.target_spec.supported_types = [tf.float16]
    tflite_model = converter.convert() # type: bytes
    with open(args.dst_tflite_file, "wb") as f:
        f.write(tflite_model)

if __name__ == "__main__":
    main()
