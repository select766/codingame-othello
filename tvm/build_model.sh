#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Error: build_model.sh src_savedmodel dst_archive.a"
    exit 1
fi

set -ex

SRC=$1
DST=$2

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BUILD_DIR="$SCRIPT_DIR/crtbuild"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
python "$SCRIPT_DIR/tf2tflite.py" "$SRC" "$BUILD_DIR/model.tflite"
pushd "$BUILD_DIR"
tvmc compile --target="c -keys=cpu -model=host" --input-shapes "data:[1,8,8,3]" --output "model.tar" --runtime crt --runtime-crt-system-lib 1 --executor aot --output-format mlf --pass-config tir.disable_vectorize=1 "model.tflite"
mkdir model
pushd model/
tar xf ../model.tar
popd
mkdir crt crt_config src
cp -r model/runtime/{include,Makefile,src} crt/
cp model/runtime/template/host/Makefile.template Makefile
cp model/runtime/template/crt_config-template.h crt_config/crt_config.h
cp ../Makefile.template Makefile
# do make in docker
docker run --rm --mount type=bind,source=$(pwd),target=/build gcc:12.2.0-bullseye sh -c 'cd /build && make && chmod -R a+w .'
popd

# copy include directory
rm -rf "$SCRIPT_DIR/include"
cp -a "$BUILD_DIR/crt/include" "$SCRIPT_DIR/include"
cp "$BUILD_DIR/build/model.a" "$DST"
