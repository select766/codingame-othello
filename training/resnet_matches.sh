#!/bin/bash

set -ex

BASE=10
for TARGET in 0 100 500
do
python -m othello_train.match_exe bin/res_${BASE}_1s/codingame.bin bin/res_${TARGET}_1s/codingame.bin --out log_res_${BASE}_1s_res_${TARGET}_1s.json --games 100 --parallel 7
done

BASE=500
for TARGET in 525 550 575 600
do
python -m othello_train.match_exe bin/res_${BASE}_1s/codingame.bin bin/res_${TARGET}_1s/codingame.bin --out log_res_${BASE}_1s_res_${TARGET}_1s.json --games 100 --parallel 7
done
