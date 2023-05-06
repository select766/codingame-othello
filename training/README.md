# 学習に使った泥臭いコード

シェルスクリプトはカレントディレクトリをルートディレクトリとして実行する想定。ipynbは、othello_trainディレクトリに移動させるかそのディレクトリにインポートパスを通す必要がある。

- `large_model_train_loop.sh` 大きなResNetモデルを長期間学習し、モデル及び棋譜を得る
  - 数週間かかるかも
- `model_build.sh` 他のスクリプトから呼び出されて、モデルを入力としてそれを内包するcodingame用バイナリのビルドを行う
- `resnet_builds.sh` `large_model_train_loop.sh`で学習したモデルをcodingame用バイナリにビルドする
- `resnet_matches.sh` ビルドされたバイナリ間で対局して強さを比較する（開始局面集がないので同じ進行が多数発生する問題あり）
- `merge_split_resnet_dataset.ipynb` `large_model_train_loop.sh`で生成した棋譜の500~599エポックのものを用いて教師あり学習用のデータを生成する
- `train_models.sh` 小さなモデルを複数種類学習する
  - 数日かかる
- `make_book_source.ipynb` `large_model_train_loop.sh`で生成した棋譜の500~599エポックのものを用いて、複数回登場した局面と指し手を収集する
- `make_sokutei_kyokumen.ipynb` `make_book_source.ipynb`で加工したデータをもとに、石が10個置かれたユニークな局面を1000個出力する。出力ファイル `sokutei_kyokumen.txt` は強さ測定に使う。
- `variant_builds.sh` 学習したモデルをそれぞれcodingame用バイナリにビルドする。中身の定数は、実際の学習で停止したepoch数に応じて書き換えが必要。
- `compare_match.sh` `variant_builds.sh`で生成したバイナリを対局させて強さを比較する。
