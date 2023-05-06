#!/bin/bash

set -ex
DATASET=model/supervised_data_from_resnet_20230326_epoch5xx
OPTIONS="--model OthelloModelResNetV1 --epoch 100 --batch_size 4096"
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c8_b3 --model_kwargs '{"ch":8,"blocks":3,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c8_b5 --model_kwargs '{"ch":8,"blocks":5,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c8_b10 --model_kwargs '{"ch":8,"blocks":10,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c16_b3 --model_kwargs '{"ch":16,"blocks":3,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c16_b5 --model_kwargs '{"ch":16,"blocks":5,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c16_b10 --model_kwargs '{"ch":16,"blocks":10,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c6_b3 --model_kwargs '{"ch":6,"blocks":3,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c6_b5 --model_kwargs '{"ch":6,"blocks":5,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c4_b3 --model_kwargs '{"ch":4,"blocks":3,"fc":16,"fc_ch":4}' $OPTIONS
python -m othello_train.supervised_train_v2 $DATASET model/resnet_c4_b5 --model_kwargs '{"ch":4,"blocks":5,"fc":16,"fc_ch":4}' $OPTIONS
