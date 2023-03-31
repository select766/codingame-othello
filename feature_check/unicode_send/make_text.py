# unicodeのコードポイント1～65535を含む文字列を生成する。
s = ""
# UnicodeEncodeError: 'utf-8' codec can't encode characters in position 55296-57343: surrogates not allowed
# chr(0)はクリップボード上で終端文字として扱われ、それ以降がペーストされないことが判明
# chr(0x0A)=LFは問題なく使える。
# chr(0x0D)=CRは、ペーストするとLF(0x0A)とみなされた
for code_point in range(1, 55296):
    if code_point == 0xD: # CR
        continue
    s += chr(code_point)
for code_point in range(57344, 65536):
    s += chr(code_point)

with open("unicode_chars_utf8.txt", "w", encoding="utf-8") as f:
    f.write(s)
