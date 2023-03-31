# Unicode文字をソースコードに挿入し、1文字で1バイトより大きい情報を含ませる手段の実験

# ブラウザ上のエディタに貼り付けられる文字のチェック

`make_text.py` を実行して、 `unicode_chars_utf8.txt` を生成。
できたテキストファイルをエディタで開き、クリップボードにコピーする。
Webブラウザでindex.htmlを開き、ページ内のtextareaにペーストする。Runボタンを押すと、存在する文字の範囲を表示してくれる。

実験結果(Windows10, VSCode, Chrome)

```
Skip: 0x0000-0x0000
OK: 0x0001-0x000C
Skip: 0x000D-0x000D
OK: 0x000E-0xD7FF
Skip: 0xD800-0xDFFF
OK: 0xE000-0xFFFF
```

0x00(NUL), 0x0D(LF), 0xD800-0xDFFF(サロゲート)以外はすべて貼り付けることができた。

# Pythonソースの文字列定数に貼り付けられる文字のチェック

ソースコードの文字列定数に文字を埋め込んだ時、正しく読まれるかのチェック。例えばバックスラッシュ`\`はメタ文字であり、その通りに解釈されないため使用できない。文字列定数は`""`で囲むため、ダブルクオーテーション`"`は使用不可。一方でシングルクオーテーション`'`は使用可能。

`check_all_char.py` を実行することで検証できる。読み取りに成功した文字の一覧が`python_ok_chars_utf8.txt`に出力される。

実験結果(Windows10, Python 3.9)

```
Skip: 0x0000-0x0000
OK: 0x0001-0x0009
Skip: 0x000A-0x000A
OK: 0x000B-0x000C
Skip: 0x000D-0x000D
OK: 0x000E-0x0021
Skip: 0x0022-0x0022
OK: 0x0023-0x005B
Skip: 0x005C-0x005C
OK: 0x005D-0xD7FF
Skip: 0xD800-0xDFFF
OK: 0xE000-0xFFFF
```

合計63483文字が読み取り成功。これはWebブラウザにペーストしても問題を生じなかった。

63483文字の集合の情報量は、`log2(63483)=15.954082686406467`。
`efficiency.py` での計算により、base64のようにバイト列を文字列にパックする場合、174文字に347バイトを詰め込み、99.2%の効率となることがわかる。

## Line Separator (U+2028), Paragraph Separator (U+2029)の削除

これらが含まれていると、ファイルを開いたときにVSCodeが警告を表示する。文字集合が63481文字になっても、ぎりぎりで174文字に347バイトを詰め込むことができるので削除する。

```
remove_line_separator.py
```

結果は `python_ok_chars_no_line_separator_utf8.txt` に出力される。

## 変換テーブルの作成

```
python make_decode_offset_table.py
```

を実行し、その結果

```
Encode offset:
{55289: 2055, 8227: 7, 88: 5, 31: 4, 11: 3, 9: 2, 0: 1}
Decode offset:
{9: 1, 12: 2, 33: 3, 91: 4, 8231: 5, 55295: 7, 65536: 2055}
```

を以下のファイルに反映する。

```
scripts/base63481/decode.py
scripts/base63481/encode.py
scripts/pack_executable_to_py.py
```
