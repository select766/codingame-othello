#!/usr/bin/env python3

"""
codingameに提出可能な単一のC++ソースファイルを、サーバ上でビルドするpythonスクリプトを生成する。
できたpythonファイルを、codingameにPython 3言語として提出する。

最適化について
g++のオプションに-O1を与える。
-O3は時間切れになる。（初手の2000ms制限の中でコンパイルするため）

C++として提出する場合はC++ソース先頭に以下の内容を書くことで少しスピードアップしていた。
しかしこれがあるとビルド制限時間を超えるため外している。
#pragma GCC optimize("O3,unroll-loops,inline")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt,fma")
"""

import argparse


def embed_source(cpp_source) -> str:
    out_source = 's=r"""'
    out_source += cpp_source
    out_source += '''"""
with open('s.cpp','w') as f:
 f.write(s)
import subprocess
subprocess.run('g++ s.cpp -std=c++17 -O1',shell=True)
subprocess.run('./a.out')
'''
    return out_source

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cpp_file", type=argparse.FileType("r", encoding="utf-8"))
    parser.add_argument("dst_python_file", type=argparse.FileType("w", encoding="utf-8"))
    args = parser.parse_args()
    out_source = embed_source(args.cpp_file.read())
    args.dst_python_file.write(out_source)

if __name__ == "__main__":
    main()
