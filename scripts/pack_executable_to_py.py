#!/usr/bin/env python3

"""
codingameサーバ上で実行可能なバイナリを、提出可能なpythonコード形式にパックする。
できたpythonファイルを、codingameにPython 3言語として提出する。
"""

import argparse
import lzma
import base64

def embed_source(binary) -> str:
    packed_binary = base64.b64encode(lzma.compress(binary, preset=9)).decode('ascii')
    out_source = 's="'+packed_binary+'"'
    out_source += '''
import lzma
import base64
import subprocess
import os
open('x','wb').write(lzma.decompress(base64.b64decode(s.encode('ascii'))))
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
