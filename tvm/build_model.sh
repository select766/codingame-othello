#!/bin/bash

set -ex
python tf2tflite.py ../model/debug_ch16_mate1ply/sm_113 debug_debug_ch16_mate1ply_epoch113.tflite
tvmc compile --target="c -keys=cpu -model=host" --input-shapes "data:[1,8,8,3]" --output debug_debug_ch16_mate1ply_epoch113_crt.tar --runtime crt --runtime-crt-system-lib 1 --executor aot --output-format mlf --pass-config tir.disable_vectorize=1 debug_debug_ch16_mate1ply_epoch113.tflite
mkdir crtbuild
cd crtbuild
mkdir model
cd model/ 
tar xf ../../debug_debug_ch16_mate1ply_epoch113_crt.tar
cd ..
mkdir crt crt_config src
cp -r model/runtime/{include,Makefile,src} crt/
cp model/runtime/template/host/Makefile.template Makefile
cp model/runtime/template/crt_config-template.h crt_config/crt_config.h
cp ../Makefile.template Makefile
cp ../main.cc src
cd ../..
# at project root
# do make in docker
docker run --rm --mount type=bind,source=$(pwd),target=/build gcc:12.2.0-bullseye sh -c 'cd /build/tvm/crtbuild && make && strip build/main && chmod -R 777 build'
python scripts/pack_executable_to_py.py tvm/crtbuild/build/main build/codingame.py
