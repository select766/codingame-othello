#!/bin/bash
# 大きなResNetモデルを長期間学習し、モデル及び棋譜を得る
PYTHONUNBUFFERED=1 python othello_train/rl_loop.py model/resnet_20230326_earlystop --model OthelloModelResNetV1 --playout_limit 64 --batch_size 256 --games 10000 --train_epoch 1 --epoch 500 | tee -a model/resnet_20230326_earlystop/train.log
PYTHONUNBUFFERED=1 python othello_train/rl_loop.py model/resnet_20230326_earlystop --model OthelloModelResNetV1 --playout_limit 256 --batch_size 256 --games 10000 --train_epoch 1 --epoch 600 | tee -a model/resnet_20230326_earlystop/train500.log
