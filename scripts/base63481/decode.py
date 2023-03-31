import argparse

CHARSET_SIZE = 63481
UNIT_CHARS = 174
UNIT_BYTES = 347
OFFSET_TABLE = {9: 1, 12: 2, 33: 3, 91: 4, 8231: 5, 55295: 7, 65536: 2055}


def decode_chunk(chunk_text):
    bigint = 0
    chunk_binary = bytearray(UNIT_BYTES)
    assert len(chunk_binary) == UNIT_BYTES
    for i in range(UNIT_CHARS):
        c = ord(chunk_text[i])
        for upper_bound, offset in OFFSET_TABLE.items():
            if c <= upper_bound:
                c -= offset
                break
        bigint = bigint * CHARSET_SIZE + c
    for i in range(UNIT_BYTES):
        char_idx = bigint % 256
        bigint = bigint // 256
        chunk_binary[i] = char_idx
    return bytes(chunk_binary)


def decode(src_text):
    assert len(src_text) % UNIT_CHARS == 0
    dst_binary = b""
    for i in range(len(src_text) // UNIT_CHARS):
        dst_binary += decode_chunk(src_text[i*UNIT_CHARS:(i+1)*UNIT_CHARS])
    return dst_binary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("src", help="source text file")
    parser.add_argument("dst", help="dst binary file")
    parser.add_argument("--size", type=int, help="original data size [bytes]. Unless it, padding of zeros may exist.")
    args = parser.parse_args()

    with open(args.src, "r", encoding="utf-8") as f:
        src_text = f.read()

    dst_binary = decode(src_text)
    if args.size is not None:
        dst_binary = dst_binary[:args.size]

    with open(args.dst, "wb") as f:
        f.write(dst_binary)


if __name__ == "__main__":
    main()
