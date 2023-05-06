#!/bin/bash

set -ex
BASE=resnet_c8_b5
#for NAME in release20230331 resnet_c8_b3 resnet_c8_b10 resnet_c16_b3 resnet_c16_b5 resnet_c16_b10
for NAME in resnet_c4_b3
do
python -m othello_train.match_exe bin/${BASE}/codingame.bin bin/${NAME}/codingame.bin --parallel 7 --games 1000 --out ${BASE}_vs_${NAME}.json --opening othello_train/sokutei_kyokumen.txt 
done
