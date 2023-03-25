#ifndef _BOARD_
#define _BOARD_
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <sstream>
#include <chrono>


#define BOARD_SIZE 8
#define BOARD_AREA 64
#define MOVE_PASS BOARD_AREA
#define N_PLAYER 2
#define BLACK 0
#define WHITE 1
#define DRAW 2
#define R_A 0
#define R_B 1
#define R_C 2
#define R_D 3
#define R_E 4
#define R_F 5
#define R_G 6
#define R_H 7
#define C_1 0
#define C_2 8
#define C_3 16
#define C_4 24
#define C_5 32
#define C_6 40
#define C_7 48
#define C_8 56

using namespace std;

using BoardPlane = uint64_t;
using Color = int32_t; // BLACK / WHITE
using Position = int32_t;
using Move = int32_t;

inline BoardPlane position_plane(Position pos)
{
    return 1ULL << pos;
}

inline bool move_is_pass(Move move)
{
    return move == MOVE_PASS;
}

class UndoInfo
{
public:
    BoardPlane planes[N_PLAYER];
    int pass_count;
};


inline string move_to_str(Move move)
{
    if (move_is_pass(move))
    {
        return "pass";
    }
    char m[3];
    m[2] = 0;
    m[0] = 'a' + move % BOARD_SIZE;
    m[1] = '1' + move / BOARD_SIZE;
    return string(m);
}

inline Move move_from_str(const string &move_str)
{
    if (move_str[0] == 'p')
    {
        return MOVE_PASS;
    }
    return (move_str[0] - 'a') + (move_str[1] - '1') * BOARD_SIZE;
}

inline constexpr uint64_t splitmix64(uint64_t z)
{
    // https://xorshift.di.unimi.it/splitmix64.c
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

class Board
{
    BoardPlane planes[N_PLAYER];
    Color _turn;
    int _pass_count; // 連続パス回数

public:
    Board() {
        set_hirate();
    };

    bool operator==(const Board& other) const {
        return planes[0] == other.planes[0] && planes[1] == other.planes[1] && _turn == other._turn && _pass_count == other._pass_count;
    }

    bool operator!=(const Board& other) const {
        return !(*this == other);
    }

    uint64_t hash() const {
        return (splitmix64(planes[0]) ^ splitmix64(planes[1])) + _turn + (_pass_count << 1);
    }

    void set(const Board &other)
    {
        planes[0] = other.planes[0];
        planes[1] = other.planes[1];
        _turn = other._turn;
        _pass_count = other._pass_count;
    }

    Color turn() const
    {
        return _turn;
    }

    BoardPlane plane(Color c) const
    {
        return planes[c];
    }

    void set_hirate()
    {
        planes[0] = planes[1] = 0;
        planes[WHITE] |= position_plane(R_D + C_4);
        planes[BLACK] |= position_plane(R_E + C_4);
        planes[BLACK] |= position_plane(R_D + C_5);
        planes[WHITE] |= position_plane(R_E + C_5);
        _turn = BLACK;
        _pass_count = 0;
    }

    void set_position_codingame(const vector<string> &lines, Color turn)
    {
        // lines[0]: a1 to h1, '.' = nothing, '0' = black, '1' = white
        planes[0] = planes[1] = 0;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                char c = lines[row][col];
                if (c == '.')
                {
                    continue;
                }
                int color = c - '0';
                planes[color] |= position_plane(row * BOARD_SIZE + col);
            }
        }
        // 相手が最後にパスしている可能性もあるが、codingameで指し手を求められるのは合法手がある場合のみであり、指し手生成に影響なし。
        this->_turn = turn;
        _pass_count = 0;
    }

    // 盤面を表現する、- (なし), O (白), X (黒)を64文字並べた文字列を返す
    string get_position_string()
    {
        char str[BOARD_AREA + 1];
        str[BOARD_AREA] = '\0';
        int i = 0;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                char c = '-';
                if (planes[BLACK] & position_plane(i))
                {
                    c = 'X';
                }
                else if (planes[WHITE] & position_plane(i))
                {
                    c = 'O';
                }
                str[i++] = c;
            }
        }

        return string(str);
    }

    // get_position_string()の末尾に、" b"(黒の手番)または" w"(白の手番)を付加した文字列を返す
    string get_position_string_with_turn()
    {
        string posstr = get_position_string();
        if (this->turn() == BLACK)
        {
            return posstr + " b";
        }
        else
        {
            return posstr + " w";
        }
    }

    // get_position_string()の結果を読み取る
    void set_position_string(const string &position, Color turn)
    {
        // 注意: パス情報については保存できない
        const char *pchars = position.c_str();
        planes[0] = planes[1] = 0;
        int i = 0;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                char c = pchars[i];
                if (c == 'X')
                {
                    planes[BLACK] |= position_plane(i);
                }
                else if (c == 'O')
                {
                    planes[WHITE] |= position_plane(i);
                }
                i++;
            }
        }

        this->_turn = turn;
        _pass_count = 0;
    }

    // get_position_string_with_turn()の結果を読み取る
    void set_position_string_with_turn(const string &position_with_turn)
    {
        char turn_char = position_with_turn[BOARD_AREA + 1];
        set_position_string(position_with_turn, turn_char == 'b' ? BLACK : WHITE); // 余分な文字がついていても先頭64文字しか見ない
    }

    void do_move(Move move, UndoInfo &undo_info)
    {
        undo_info.planes[0] = planes[0];
        undo_info.planes[1] = planes[1];
        undo_info.pass_count = _pass_count;
        if (!move_is_pass(move))
        {
            BoardPlane player = planes[_turn], opponent = planes[1 - _turn], position = position_plane(move);
            BoardPlane reverse_plane = reverse(player, opponent, position);
            planes[_turn] = player ^ position ^ reverse_plane;
            planes[1 - _turn] = opponent ^ reverse_plane;
            _pass_count = 0;
        }
        else
        {
            _pass_count++;
        }
        _turn = 1 - _turn;
    }

    void undo_move(const UndoInfo &undo_info)
    {
        planes[0] = undo_info.planes[0];
        planes[1] = undo_info.planes[1];
        _pass_count = undo_info.pass_count;
        _turn = 1 - _turn;
    }

    // 合法手を列挙する。
    void legal_moves(vector<Move> &move_list, bool with_pass = false) const
    {
        // ビットボード参考 https://zenn.dev/kinakomochi/articles/othello-bitboard
        move_list.clear();
        BoardPlane bb;
        legal_moves_bb(bb);
        for (int pos = 0; pos < BOARD_AREA; pos++)
        {
            if (bb & position_plane(pos))
            {
                move_list.push_back(pos);
            }
        }

        if (with_pass && move_list.empty())
        {
            move_list.push_back(MOVE_PASS);
        }
    }

    void legal_moves_bb(BoardPlane &result) const
    {
        result = 0;
        BoardPlane buffer;
        BoardPlane player = planes[_turn], opponent = planes[1 - _turn];
        legal_calc(result, buffer, player, opponent, 0x7e7e7e7e7e7e7e7e, 1);
        legal_calc(result, buffer, player, opponent, 0x007e7e7e7e7e7e00, 7);
        legal_calc(result, buffer, player, opponent, 0x00ffffffffffff00, 8);
        legal_calc(result, buffer, player, opponent, 0x007e7e7e7e7e7e00, 9);

        result &= ~(player | opponent);
    }

    // 手が合法かチェックする。ただし、MOVE_PASSは扱えない。
    bool is_legal(Move move) const
    {
        BoardPlane player = planes[_turn], opponent = planes[1 - _turn], position = position_plane(move);
        if ((player | opponent) & position)
        {
            // すでにある場所には置けない
            return false;
        }
        BoardPlane reverse_plane = reverse(player, opponent, position);
        // ひっくりかえせない場所には置けない
        return reverse_plane ? true : false;
    }

    bool is_gameover() const
    {
        if (_pass_count == 2)
        {
            return true;
        }
        if (planes[0] == 0 || planes[1] == 0)
        {
            // 全滅
            return true;
        }
        if (~(planes[0] | planes[1]) == 0)
        {
            return true;
        }
        return false;
    }

    int piece_sum() const
    {
        return __builtin_popcountll(planes[BLACK]) + __builtin_popcountll(planes[WHITE]);
    }

    // 手番側の石の数
    int piece_num() const
    {
        return __builtin_popcountll(planes[_turn]);
    }

    int piece_num(int color) const
    {
        return __builtin_popcountll(planes[color]);
    }

    // 石の数の差。手番側-相手側。
    int count_stone_diff() const
    {
        return piece_num(_turn) - piece_num(1-_turn);
    }

    // 勝者。BLACK/WHITE/DRAW
    int winner() const
    {
        // is_gameover()のチェックはしない
        int v = piece_num(BLACK) - piece_num(WHITE);
        if (v > 0)
        {
            return BLACK;
        }
        else if (v < 0)
        {
            return WHITE;
        }
        return DRAW;
    }

    string pretty_print() const
    {
        string s;
        s.append(" |abcdefgh\n");
        s.append("-+--------\n");
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            s.append(to_string(row + 1));
            s.append("|");
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                int p = row * BOARD_SIZE + col;
                if (planes[BLACK] & position_plane(p))
                {
                    s.append("x");
                }
                else if (planes[WHITE] & position_plane(p))
                {
                    s.append("o");
                }
                else
                {
                    s.append(".");
                }
            }
            s.append("\n");
        }
        return s;
    }

private:
    template <typename ShiftFunc>
    void line(BoardPlane &result, BoardPlane position, BoardPlane mask, ShiftFunc shift, int n) const
    {
        result = mask & shift(position, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
    }

    void legal_calc(BoardPlane &result, BoardPlane &buffer, BoardPlane player, BoardPlane opponent, BoardPlane mask, int n) const
    {
        BoardPlane _mask = opponent & mask;
        line(
            buffer, player, _mask, [](BoardPlane p, int c)
            { return p << c; },
            n);
        result |= buffer << n;
        line(
            buffer, player, _mask, [](BoardPlane p, int c)
            { return p >> c; },
            n);
        result |= buffer >> n;
    }

    void reverse_calc(BoardPlane &result, BoardPlane &buffer, BoardPlane s, BoardPlane opponent, BoardPlane position, BoardPlane mask, int n) const
    {
        BoardPlane _mask = opponent & mask;
        line(
            buffer, position, _mask, [](BoardPlane p, int c)
            { return p << c; },
            n);
        if (s & (buffer << n))
        {
            result |= buffer;
        }
        line(
            buffer, position, _mask, [](BoardPlane p, int c)
            { return p >> c; },
            n);
        if (s & (buffer >> n))
        {
            result |= buffer;
        }
    }

    BoardPlane reverse(BoardPlane player, BoardPlane opponent, BoardPlane position) const
    {
        BoardPlane result = 0;
        BoardPlane buffer;
        reverse_calc(result, buffer, player, opponent, position, 0x7e7e7e7e7e7e7e7e, 1);
        reverse_calc(result, buffer, player, opponent, position, 0x007e7e7e7e7e7e00, 7);
        reverse_calc(result, buffer, player, opponent, position, 0x00ffffffffffff00, 8);
        reverse_calc(result, buffer, player, opponent, position, 0x007e7e7e7e7e7e00, 9);
        return result;
    }
};

#endif
