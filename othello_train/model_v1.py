from tensorflow.keras.layers import Dense, Flatten, Conv2D
from tensorflow.keras import Model

class OthelloModelV1(Model):
  def __init__(self, ch=16):
    super().__init__()
    # Conv2Dのデフォルトはdata_format="channels_last" (NHWC)
    # data_format="channels_first" (NCHW)はCPUでの推論が対応してない
    # The Conv2D op currently only supports the NHWC tensor format on the CPU. The op was given the format: NCHW
    self.conv1 = Conv2D(ch, 3, activation='relu', padding="same")
    self.conv2 = Conv2D(ch, 3, activation='relu', padding="same")
    self.conv3 = Conv2D(ch, 3, activation='relu', padding="same")
    self.conv4 = Conv2D(ch, 3, activation='relu', padding="same")
    self.conv5 = Conv2D(ch, 3, activation='relu', padding="same")
    self.conv6 = Conv2D(1, 3, activation=None, padding="same")
    self.value_flatten = Flatten()
    self.value_fc_1 = Dense(16, activation='relu')
    self.value_fc_2 = Dense(1, activation=None)
    self.policy_flatten = Flatten()

  def call(self, x):
    x = self.conv1(x)
    x = self.conv2(x)
    x = self.conv3(x)
    x = self.conv4(x)
    x = self.conv5(x)
    p = x
    v = x
    p = self.conv6(p)
    p = self.policy_flatten(p)
    v = self.value_flatten(v)
    v = self.value_fc_1(v)
    v = self.value_fc_2(v)
    return p, v
