#!/usr/bin/env python3

"""
C++ソースとヘッダを連結し、codingameに提出可能な単一のC++ソースファイルにする。

文字列置換を用いた簡易的なものであり、C++の構文を理解していないことに注意。

仮定
ソースファイルは単一のディレクトリ(root_dir)内にある。
#include ""の形式ではroot_dirにあるヘッダファイルをインクルードしている。
#include <>の形式では標準ライブラリをインクルードしている。
cppファイルは、常に先頭行で#include "common.hpp"を記載し、他のinclude文を書かない。
cppファイルはどの順序で単一のcppファイルとして連結されても文法エラーを生じない(例: static修飾子のついたグローバル変数が重複しない)
ヘッダファイルにはifdefを用いたインクルードガードが書かれている(pragma onceは機能しない。2回目以降のロードで副作用を生じない。)
"""

import argparse
from pathlib import Path
import sys
from typing import List
import re

def read_file_lines(root_dir: Path, basename: str) -> List[str]:
    return (root_dir / basename).read_text(encoding="utf-8").splitlines()

def include_header_recursive(root_dir: Path, basename: str, loaded_files: List[str]) -> List[str]:
    all_lines = []
    if basename in loaded_files:
        # ファイル容量削減のため、同じファイルが複数回インクルードされても初回しか出力しない
        return all_lines
    loaded_files.append(basename)
    hpp_source = read_file_lines(root_dir, basename)
    for line in hpp_source:
        m = re.match('^#include\\s+"(.+?)"\\s*$', line) # #include <iostream>のような形式は標準ライブラリとみなす
        if m is not None:
            # インクルード行
            target_name = m.group(1)
            all_lines.extend(include_header_recursive(root_dir, target_name, loaded_files))
        else:
            # 通常の行
            all_lines.append(line)
    return all_lines

def concat_source(root_dir: Path, cpp_files: List[str]) -> str:
    is_first_file = True
    concat_lines = []
    for cpp_file in cpp_files:
        cpp_source = read_file_lines(root_dir, cpp_file)
        assert len(cpp_source) > 0
        assert cpp_source[0] == '#include "common.hpp"'
        assert all(re.match('^#include\\s', line) is None for line in cpp_source[1:])
        if is_first_file:
            concat_lines.extend(include_header_recursive(root_dir, 'common.hpp', []))
        concat_lines.extend(cpp_source[1:])
        is_first_file = False
    return '\n'.join(concat_lines)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("root_dir")
    parser.add_argument("cpp_file", nargs="+")
    parser.add_argument("-o", type=argparse.FileType("w", encoding="utf-8"),
                        default=sys.stdout)
    args = parser.parse_args()
    root_dir = Path(args.root_dir)
    result = concat_source(root_dir, args.cpp_file)
    args.o.write(result)

if __name__ == "__main__":
    main()
