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
