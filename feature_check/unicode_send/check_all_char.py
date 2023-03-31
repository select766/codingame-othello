# pythonソースコードの文字列定数として埋め込んでエラーを生じない文字のリストを作成する。
# ダブルクオーテーションで囲むため、ダブルクオーテーションはエラーを生じる。
import subprocess
from tqdm import tqdm
src = '''s="PPP"
assert ord(s) == CCC
'''

def check_ok(s, code_point):
    with open("/tmp/char.py", "w", encoding="utf-8") as f:
        f.write(src.replace("PPP", s).replace("CCC", str(code_point)))
    ret = subprocess.call(["python", "/tmp/char.py"])
    return ret == 0

ok_chars = ""
for code_point in tqdm(range(0, 65536)):
    try:
        s = chr(code_point)
        if check_ok(s, code_point):
            ok_chars += s
    except:
        pass

with open("python_ok_chars_utf8.txt", "w", encoding="utf-8") as f:
    f.write(ok_chars)
