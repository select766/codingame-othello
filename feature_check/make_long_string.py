"""
長さチェック用のランダム文字列生成機構
"""

import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dst")
    parser.add_argument("length", type=int)
    parser.add_argument("chartype")
    args = parser.parse_args()

    char = {"ascii": "a", "hiragana": "あ", "sjis2004": "𠀋", "emoji2": "👩‍👩‍👧‍👧"}[args.chartype]

    with open(args.dst, "w", encoding="utf-8") as f:
        f.write(char * args.length)

if __name__ == "__main__":
    main()
