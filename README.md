# codingame-othello
CodingameのOthello(オセロ)タスクのAI

# codingame提出用ファイル作成

複数のソースファイルに分かれているが、単一のファイルにまとめる必要がある。

```
make
```

`build/codingame.cpp`が成果物。

# 合法手チェック

## データ生成

正しく動作するコミットにおいて

```
./build/make_legal_move_test_data > dataset/legal_move_dataset.txt
```

## チェック

検証したいコミットにおいて

```
./build/legal_move_test < dataset/legal_move_dataset.txt
```

# 教師あり学習

## 教師データ生成

アルファベータ法＋石の数で評価する(学習不要の)簡易AIで生成

```
mkdir -p dataset/alphabeta_train_1
./build/generate_training_data_1 dataset/alphabeta_train_1/raw_game_train.bin 10000
python -m othello_train.shuffle_train_data dataset/alphabeta_train_1/raw_game_train.bin dataset/alphabeta_train_1/train_shuffled.bin
```

## 学習

```
mkdir -p model
python -m othello_train.supervised_train_v1
```

## 自己対局

```
./build/random_match
```

対戦相手は `main_random_match.cpp` 内にハードコードされている

DNNモデルを使うエンジンの場合は、評価サーバを立てておく必要がある

```
python -m othello_train.eval_server_v1 model/alphabeta_supervised_model_v1
```

# 強化学習

最新のモデルで棋譜生成→それを用いてモデルを更新 というループを回す

```
python othello_train/rl_loop.py model/debug
```


## 自己対局

評価サーバを立てる

```
python -m othello_train.eval_server_v1 model/debug/sm_9
```

`sm_`の後ろの番号はエポック数。大きいほうが学習が進んでいる。

別のシェルで対局を実行

```
./build/random_match
```

対戦相手は `main_random_match.cpp` 内にハードコードされている

## モデルをソースコードに埋め込み

codingame提出のため、Tensorflowを用いずC++ソースだけでモデルを実行する必要がある。DNNの重みをソースファイル上の定数として定義する。

```
python -m othello_train.embed_weight model/debug/sm_10 src/_dnn_weight.hpp
```

この状態で `make` すれば `/build/codingame.cpp` にモデルが埋め込まれた状態になる。
