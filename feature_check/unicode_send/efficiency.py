# Unicode文字N文字を組としてバイト列を表すときの効率を計算

unicode_patterns = 63483
byte_patterns = 256

for n in range(1, 1000):
    patterns = unicode_patterns ** n
    n_bytes_can_represented = 0
    for b in range(1000000):
        if byte_patterns ** b > patterns:
            n_bytes_can_represented = b - 1
            break
    efficiency = (byte_patterns ** n_bytes_can_represented) / patterns
    print(f"{n},{n_bytes_can_represented},{efficiency}")
# n文字で、n_bytes_can_represented バイトを表したとき、表現できる空間のうちefficiencyの割合が埋まる。
