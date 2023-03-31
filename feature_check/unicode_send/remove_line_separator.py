with open("python_ok_chars_utf8.txt", "r", encoding="utf-8") as f:
    chars = f.read()
chars = "".join([c for c in chars if ord(c) not in [0x2028, 0x2029]])
with open("python_ok_chars_no_line_separator_utf8.txt", "w", encoding="utf-8") as f:
    f.write(chars)
