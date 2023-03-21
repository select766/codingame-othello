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

# 学習

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
python -m othello_train.train_v1
```

# 自己対局

```
./build/random_match
```

対戦相手は `main_random_match.cpp` 内にハードコードされている

DNNモデルを使うエンジンの場合は、評価サーバを立てておく必要がある

```
python -m othello_train.eval_server_v1 model/alphabeta_supervised_model_v1
```
