import sys
import math

_id = int(input())
board_size = int(input())

import subprocess
print(subprocess.getoutput("cat /etc/*-release"), file=sys.stderr, flush=True)
print(subprocess.getoutput("g++ --version"), file=sys.stderr, flush=True)
print(subprocess.getoutput("ldd --version"), file=sys.stderr, flush=True)


while True:
    for i in range(board_size):
        line = input() 
    action_count = int(input())
    for i in range(action_count):
        action = input()
    print(action)
