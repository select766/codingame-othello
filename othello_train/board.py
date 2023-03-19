# 局面のパックなどを行う

from typing import Tuple
import numpy as np

BLACK = 0
WHITE = 1
N_PLAYER = 2
BOARD_SIZE = 8
BOARD_AREA = 64
MOVE_PASS = BOARD_AREA
MOVE_RECORD_SIZE = 24
move_record_dtype = np.dtype([('board', 'B', (16,)), ('turn', 'B'), ('move', 'B'), ('game_result', 'b'), ('n_legal_moves', 'B'), ('pad', 'B', (4,))])

def get_board_array(packed):
    # packed: uint8 len=16
    # result: uint8 (2, 8, 8) value=0 or 1
    # 盤面は64bit数値2個(BLACK, WHITEの順)で格納
    # LSBが盤の左上で、次のビットがその右
    # little endianを想定しており、下位バイト=盤の上が最初のバイトに格納
    # 1byteの数値をさらにunpackbitsで、要素が0か1のuint8の数値8個に分解する
    return np.unpackbits(packed[:,np.newaxis],axis=1,bitorder="little").reshape(N_PLAYER,BOARD_SIZE,BOARD_SIZE)

def print_board_array(board):
    s = " |abcdefgh\n"+"-+--------\n"
    for row in range(BOARD_SIZE):
        s += f"{row+1}|"
        for col in range(BOARD_SIZE):
            c = "."
            if board[BLACK,row,col] != 0:
                # BLACK
                c = 'X'
            elif board[WHITE,row,col] != 0:
                # WHITE
                c = 'O'
            s += c
        s += "\n"
    print(s, end="")

def move_to_str(move: int):
    if move == MOVE_PASS:
        return "pass"
    return chr(ord('a')+move%BOARD_SIZE)+chr(ord('1')+move//BOARD_SIZE)

def move_to_pos(move: int) -> Tuple[int, int]:
    """
    y, x座標に変換
    MOVE_PASSは渡さないこと
    """
    assert move < BOARD_AREA
    return move // BOARD_SIZE, move % BOARD_SIZE

def move_to_onehot(move: int) -> np.ndarray:
    """
    (8, 8), dtype=np.uint8の配列を作り、Moveに対応する位置だけ1、それ以外を0にして返す
    """
    a = np.zeros((BOARD_SIZE, BOARD_SIZE), dtype=np.uint8)
    a[move_to_pos(move)] = 1
    return a

def print_move_record(record):
    ba = get_board_array(record['board'])
    print_board_array(ba)
    print(f"turn: {['BLACK','WHITE'][record['turn']]}")
    print(f"move: {move_to_str(record['move'])}")
    print(f"result: {record['game_result']}")
