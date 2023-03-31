import argparse

CHARSET_SIZE = 63481
UNIT_CHARS = 174
UNIT_BYTES = 347
ENCODE_OFFSET_TABLE = {55289: 2055, 8227: 7, 88: 5, 31: 4, 11: 3, 9: 2, 0: 1}


def to_char(idx):
    for idx_ofs, char_ofs in ENCODE_OFFSET_TABLE.items():
        if idx >= idx_ofs:
            return chr(idx + char_ofs)


def encode_chunk(chunk_binary):
    bigint = 0
    chunk_chars = ""
    assert len(chunk_binary) == UNIT_BYTES
    # 順序はデコーダが簡素になるように設定
    # 最初のバイトがbigintの最下位ビットになるようにする
    for i in reversed(range(UNIT_BYTES)):
        bigint = bigint * 256 + chunk_binary[i]
    # 最初の文字がbigintの最上位桁になるようにする
    for i in range(UNIT_CHARS):
        char_idx = bigint % CHARSET_SIZE
        bigint = bigint // CHARSET_SIZE
        chunk_chars = to_char(char_idx) + chunk_chars
    assert bigint == 0

    return chunk_chars


def encode(src_binary):
    dst_chars = ""
    if len(src_binary) % UNIT_BYTES != 0:
        pad_len = UNIT_BYTES - len(src_binary) % UNIT_BYTES
        src_binary = src_binary + b"\0" * pad_len
    for i in range(len(src_binary) // UNIT_BYTES):
        dst_chars += encode_chunk(src_binary[i*UNIT_BYTES:(i+1)*UNIT_BYTES])
    return dst_chars


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src", help="source binary file")
    parser.add_argument("dst", help="dst text file")
    args = parser.parse_args()

    with open(args.src, "rb") as f:
        src_binary = f.read()

    dst_text = encode(src_binary)

    with open(args.dst, "w", encoding="utf-8") as f:
        f.write(dst_text)


if __name__ == "__main__":
    main()
