from tensorflow.keras.layers import Dense, Flatten, Conv2D, BatchNormalization, ReLU
from tensorflow.keras import Model

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
    def __init__(self, ch=8):
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
