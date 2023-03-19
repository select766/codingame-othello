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
