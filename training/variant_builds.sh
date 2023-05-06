#!/bin/bash

set -ex

NAME=c8_b3
CP=0100
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":8,"blocks":3,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}

NAME=c8_b5
CP=0051
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":8,"blocks":5,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}

NAME=c8_b10
CP=0029
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":8,"blocks":10,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}

NAME=c16_b3
CP=0015
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":16,"blocks":3,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}

NAME=c16_b5
CP=0022
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":16,"blocks":5,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}


NAME=c16_b10
CP=0015
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":16,"blocks":10,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}

NAME=c6_b3
CP=0022
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":6,"blocks":3,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}

NAME=c4_b3
CP=0020
python -m othello_train.checkpoint_to_savedmodel_v1 model/resnet_$NAME/cp-${CP}.ckpt model/resnet_${NAME}/sm-${CP} --model OthelloModelResNetV1 --model_kwargs '{"ch":4,"blocks":3,"fc":16,"fc_ch":4}'
./model_build.sh model/resnet_${NAME}/sm-${CP} bin/resnet_${NAME}
