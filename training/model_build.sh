#!/bin/bash

SAVEDMODEL="$1"
DST_DIR="$2"
make clean
rm -f model/savedmodel
SAVEDMODEL_ABSPATH=$(cd $(dirname $SAVEDMODEL); pwd)/$(basename $SAVEDMODEL)
ln -s $SAVEDMODEL_ABSPATH model/savedmodel
make build/codingame.bin
mkdir -p $DST_DIR
cp build/codingame.bin $DST_DIR/
