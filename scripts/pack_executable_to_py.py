#!/usr/bin/env python3

"""
codingameサーバ上で実行可能なバイナリを、提出可能なpythonコード形式にパックする。
できたpythonファイルを、codingameにPython 3言語として提出する。
"""

import argparse
import lzma
from base63483.encode import encode

# 先頭にcodingの行がないと、なぜか以下のエラーが生じる
# SyntaxError: Non-UTF-8 code starting with '\xe3' in file

def embed_source(binary) -> str:
    packed_binary = lzma.compress(binary, preset=9)
    coded_binary = encode(packed_binary)
    out_source = '# coding:utf8\ns="'+coded_binary+'"\nl='+str(len(packed_binary))
    out_source += '''
import lzma
import base64
import subprocess
import os

CHARSET_SIZE = 63483
UNIT_CHARS = 174
UNIT_BYTES = 347
OFFSET_TABLE = {9: 1, 12: 2, 33: 3, 91: 4, 55295: 5, 65536: 2053}

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


def decode_all(src_text):
    assert len(src_text) % UNIT_CHARS == 0
    dst_binary = b""
    for i in range(len(src_text) // UNIT_CHARS):
        dst_binary += decode_chunk(src_text[i*UNIT_CHARS:(i+1)*UNIT_CHARS])
    return dst_binary

open('x','wb').write(lzma.decompress(decode_all(s)[:l]))
os.chmod('x',511)
subprocess.run('./x')
'''
    return out_source

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("bin", type=argparse.FileType("rb"))
    parser.add_argument("dst_python_file", type=argparse.FileType("w", encoding="utf-8"))
    args = parser.parse_args()
    out_source = embed_source(args.bin.read())
    args.dst_python_file.write(out_source)

if __name__ == "__main__":
    main()
