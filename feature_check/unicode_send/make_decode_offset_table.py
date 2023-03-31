# 文字のコードポイントがX以下ならYを減算して文字のインデックスを算出する。
# X:Yのテーブルを計算する。

def make_encode_offset_table(charset):
    last_code_point = -2
    table = {}
    for i, c in enumerate(charset):
        code_point = ord(c)
        assert code_point > last_code_point
        if code_point > last_code_point + 1:
            # 区間が切れている
            # インデックスi以上の数値は、オフセットcode_pointの付加が必要
            table[i] = code_point - i
        last_code_point = code_point
    # 逆順にする
    table_rev = {}
    for k, v in reversed(table.items()):
        table_rev[k] = v
    return table_rev


def make_decode_offset_table(charset):
    last_code_point = -2
    span_first = -2
    table = {}
    for i, c in enumerate(charset):
        code_point = ord(c)
        assert code_point > last_code_point
        if code_point > last_code_point + 1:
            # 区間が切れている
            # span_firstをspan_first_iに変換するオフセットが必要
            if last_code_point >= 0:
                table[last_code_point] = span_first - span_first_i
            span_first = code_point
            span_first_i = i
        last_code_point = code_point
    upper_bound = 65536
    table[upper_bound] = span_first - span_first_i
    return table


def main():
    with open("python_ok_chars_no_line_separator_utf8.txt", "r", encoding="utf-8") as f:
        charset = f.read()
    print("Encode offset:")
    print(make_encode_offset_table(charset))
    print("Decode offset:")
    print(make_decode_offset_table(charset))


if __name__ == "__main__":
    main()
