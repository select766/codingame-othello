# Codingame othelloの実行環境の検証

サイト上の説明:
<https://www.codingame.com/playgrounds/40701/help-center/languages-versions>

```
Your program is compiled and run in a Linux environment on a 64bit multi-core architecture.
Time limit per turn should be specified on the game’s statement.
The code size for any game is limited to 100k characters.
The memory limit is 768 MB.
C++	g++ 11.2.0 mode C++17	‑lm, ‑lpthread, ‑ldl, ‑lcrypt
```

## コードサイズチェック

検証日: 2023-03-21

100k charactersとは100k bytesか？

```
const char* data_table = "<ここに長い文字列を入れる>"
```

- ascii文字"a"を90k挿入した場合: OK
- ascii文字"a"を100k挿入した場合(周辺のコードと合わせると101964bytes): NG
- ひらがな「あ」を90k文字挿入: OK
- ひらがな「あ」を100k文字挿入: NG
- SJIS2004の漢字「𠀋」を90k文字挿入: NG

コンパイルしたプログラム内では、文字列定数はUTF-8でエンコーディングされたバイト列として読み取れる。

「あ」はUTF-8で3バイト、UTF-16では2バイト。「𠀋」はUTF-8で4バイト、UTF-16では4バイト(サロゲートペア)。「あ」は1文字としてカウントされるため、3バイト分の情報を持つことが可能。評価関数等の大きなデータを、100kB以上埋め込むテクニックが開発可能。

検証用のコードはサイズが大きいため圧縮した状態でgitにコミットしている。

## CPU命令チェック

検証日: 2023-03-21

`/proc/cpuinfo` の結果

```
processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 60
model name	: Intel Core Processor (Haswell, no TSX)
stepping	: 1
microcode	: 0x1
cpu MHz		: 2194.842
cache size	: 16384 KB
physical id	: 0
siblings	: 1
core id		: 0
cpu cores	: 1
apicid		: 0
initial apicid	: 0
fpu		: yes
fpu_exception	: yes
cpuid level	: 13
wp		: yes
flags		: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 syscall nx rdtscp lm constant_tsc rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm cpuid_fault invpcid_single pti tpr_shadow vnmi flexpriority ept vpid fsgsbase bmi1 avx2 smep bmi2 erms invpcid xsaveopt arat md_clear
bugs		: cpu_meltdown spectre_v1 spectre_v2 spec_store_bypass l1tf mds swapgs
bogomips	: 4389.68
clflush size	: 64
cache_alignment	: 64
address sizes	: 40 bits physical, 48 bits virtual
power management:
processor	: 1
vendor_id	: GenuineIntel
cpu family	: 6
model		: 60
model name	: Intel Core Processor (Haswell, no TSX)
...
```

## マルチコアチェック

Codingame othelloでマルチコアが利用できるかのチェック。

検証日: 2023-03-21

結論: スレッドを立ててもエラーにはならないが、シングルコアしか使用できず高速化にはならない。

重い処理を実装し、一定時間内で実行できた回数を計測する。複数のスレッドで並列に実行し、総実行回数を計測する。

結果(実行ごとに変動する):

- n_threads = 1 (シングルスレッド)のとき162回実行
- n_threads = 2のとき161回実行

ローカルでの結果(Intel Core i7-10700F, 物理8コア、論理16コア):

```
g++ multicore_check.cpp -O3 -DENV_LOCAL
```

- n_threads = 1 => 702
- n_threads = 2 => 1208
- n_threads = 8 => 4605
- n_threads = 16 => 7150
- n_threads = 32 => 8124

マルチコアCPUで実行すれば、スレッド数の増加によって出力が大きくなることが観測できる。

## Ponderチェック

ponder(相手番の間の思考)が利用できるかのチェック。

検証日: 2023-03-21

結論: 実行は可能なものの、速度が非常に低い。相手番の間は、プロセスが停止または低優先度になっているものと思われる。

マルチコアチェックの結果と合わせると、思考には1コアしか使えないため、Codingameでの思考部の実装はシングルスレッドで十分。ノンブロッキングIOやスレッドを使用して相手の番が終了するのを待機しながら思考する処理を追加するのは、複雑な割に効果が薄い。
