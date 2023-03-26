import json
from typing import Optional, Union
from tensorflow.keras.layers import Dense, Flatten, Conv2D, BatchNormalization, ReLU, Layer
from tensorflow.keras import Model, Sequential


class ConvBN(Model):
    def __init__(self, ch, ksize=3) -> None:
        super().__init__()
        self.conv = Conv2D(ch, ksize, activation=None, padding="same")
        self.bn = BatchNormalization()
        self.relu = ReLU()

    def call(self, x):
        x = self.conv(x)
        x = self.bn(x)
        x = self.relu(x)
        return x


class OthelloModelV1(Model):
    def __init__(self, ch=16):
        super().__init__()
        # Conv2Dのデフォルトはdata_format="channels_last" (NHWC)
        # data_format="channels_first" (NCHW)はCPUでの推論が対応してない
        # The Conv2D op currently only supports the NHWC tensor format on the CPU. The op was given the format: NCHW
        self.conv1 = ConvBN(ch)
        self.conv2 = ConvBN(ch)
        self.conv3 = ConvBN(ch)
        self.conv4 = ConvBN(ch)
        self.conv5 = ConvBN(ch)
        self.conv6 = ConvBN(ch)
        self.conv7 = ConvBN(ch)
        self.policy_conv_1 = ConvBN(ch, 1)
        self.policy_conv_2 = Conv2D(1, 1, activation=None, padding="same")
        self.value_conv_1 = ConvBN(ch, 1)
        self.value_flatten = Flatten()
        self.value_fc_1 = Dense(1, activation=None)
        self.policy_flatten = Flatten()

    def call(self, x):
        x = self.conv1(x)
        x = self.conv2(x)
        x = self.conv3(x)
        x = self.conv4(x)
        x = self.conv5(x)
        x = self.conv6(x)
        x = self.conv7(x)
        p = x
        v = x
        p = self.policy_conv_1(p)
        p = self.policy_conv_2(p)
        p = self.policy_flatten(p)
        v = self.value_conv_1(v)
        v = self.value_flatten(v)
        v = self.value_fc_1(v)
        return p, v


class ResBlock(Model):
    def __init__(self, ch):
        super().__init__()
        self.conv1 = Conv2D(ch, 3, activation=None,
                            padding="same", use_bias=False)
        self.bn1 = BatchNormalization()
        self.relu1 = ReLU()
        self.conv2 = Conv2D(ch, 3, activation=None,
                            padding="same", use_bias=False)
        self.bn2 = BatchNormalization()
        self.relu2 = ReLU()

    def call(self, x):
        h = x
        h = self.relu1(self.bn1(self.conv1(h)))
        h = self.bn2(self.conv2(h))
        h = h + x
        h = self.relu2(h)
        return h


class ElementwiseBias(Layer):
    """
    要素別のバイアス
    """

    def __init__(self, shape):
        super().__init__()
        self.b = self.add_weight(
            name="b",
            shape=shape, initializer="zeros", trainable=True)

    def call(self, x):
        return x + self.b


class OthelloModelResNetV1(Model):
    def __init__(self, ch=128, fc=256, fc_ch=8, blocks=10):
        super().__init__()
        self.conv1 = Conv2D(ch, 3, activation=None,
                            padding="same", use_bias=False)
        self.bn1 = BatchNormalization()
        self.relu1 = ReLU()
        self.blocks = Sequential()
        for _ in range(blocks):
            self.blocks.add(ResBlock(ch=ch))

        self.p_conv_1 = Conv2D(1, 1, activation=None,
                               padding="same", use_bias=False)
        self.p_bias_1 = ElementwiseBias((8, 8, 1))
        self.p_flatten = Flatten()

        self.v_conv_1 = Conv2D(fc_ch, 1, activation=None, padding="same")
        self.v_bn_1 = BatchNormalization()
        self.v_relu_1 = ReLU()
        self.v_flatten = Flatten()
        self.v_fc_2 = Dense(fc, activation=None)
        self.v_bn_2 = BatchNormalization()
        self.v_relu_2 = ReLU()
        self.v_fc_3 = Dense(1, activation=None)

    def call(self, x):
        h = x
        h = self.relu1(self.bn1(self.conv1(h)))
        h = self.blocks(h)

        p = h
        p = self.p_bias_1(self.p_conv_1(p))
        p = self.p_flatten(p)

        v = h
        v = self.v_relu_1(self.v_bn_1(self.v_conv_1(v)))
        v = self.v_flatten(v)
        v = self.v_relu_2(self.v_bn_2(self.v_fc_2(v)))
        v = self.v_fc_3(v)

        return p, v


def build_model(name, kwargs: Optional[Union[str, dict]]):
    if kwargs is None:
        kwargs = {}
    elif isinstance(kwargs, str):
        # コマンドライン引数の場合の便利機能
        kwargs = json.loads(kwargs)
    module = {"OthelloModelV1": OthelloModelV1,
              "OthelloModelResNetV1": OthelloModelResNetV1}[name]
    return module(**kwargs)
