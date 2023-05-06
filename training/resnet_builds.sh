#!/bin/bash

for NAME in 0 10 100 500 525 550 575 600
do
./model_build.sh model/resnet_20230326_earlystop/sm_${NAME} bin/res_${NAME}_1s
done
