# codingame-othello
CodingameのOthello(オセロ)タスクのAI

# 合法手チェック

## データ生成

正しく動作するコミットにおいて、`MODE_MAKE_LEGAL_MOVE_TEST_DATA`マクロを定義した状態でビルド

```
./src/main > dataset/legal_move_dataset.txt
```

## チェック

検証したいコミットにおいて、`MODE_LEGAL_MOVE_TEST`マクロを定義した状態でビルド

```
./src/main < dataset/legal_move_dataset.txt
```
