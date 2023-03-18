#pragma GCC optimize("O3")
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

BoardPlane position_plane(int pos)
{
    return 1ULL << pos;
}

inline bool move_is_pass(int move)
{
    return move == MOVE_PASS;
}

class UndoInfo
{
public:
    BoardPlane planes[N_PLAYER];
    int turn; // BLACK / WHITE
    int pass_count;
};


string move_to_str(int move)
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

int move_from_str(const string &move_str)
{
    if (move_str[0] == 'p')
    {
        return MOVE_PASS;
    }
    return (move_str[0] - 'a') + (move_str[1] - '1') * BOARD_SIZE;
}

class Board
{
    BoardPlane planes[N_PLAYER];
    int _turn; // BLACK / WHITE
    int _pass_count; // 連続パス回数

public:
    Board()
    {
        set_hirate();
    }

    void set(const Board &other)
    {
        planes[0] = other.planes[0];
        planes[1] = other.planes[1];
        _turn = other._turn;
        _pass_count = other._pass_count;
    }

    int turn() const
    {
        return _turn;
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

    void set_position_codingame(const vector<string> &lines, int turn)
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
    void set_position_string(const string &position, int turn)
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

    void do_move(int move, UndoInfo &undo_info)
    {
        undo_info.planes[0] = planes[0];
        undo_info.planes[1] = planes[1];
        undo_info.turn = _turn;
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
        _turn = undo_info.turn;
        _pass_count = undo_info.pass_count;
    }

    // 合法手を列挙する。
    void legal_moves(vector<int> &move_list, bool with_pass = false) const
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

    bool can_move(int move) const
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

    bool is_end() const
    {
        if (_pass_count == 2)
        {
            return true;
        }
        if (~(planes[0] | planes[1]) == 0)
        {
            return true;
        }
        return false;
    }

    int count_stone(int color) const
    {
        return __builtin_popcountll(planes[color]);
    }

    // 石の数の差。黒-白。
    int count_stone_diff() const
    {
        // when is_end(), this function is used for checking winner
        // ret > 0: BLACK wins, ret < 0: WHITE wins, ret == 0: draw
        return count_stone(BLACK) - count_stone(WHITE);
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

// AIのインターフェース
class SearchBase
{
public:
    Board board;
    virtual int search(string &msg) = 0;
    virtual string name() = 0;
};

// ベースラインとなるランダムAI
class SearchRandom : public SearchBase
{
    std::random_device seed_gen;
    mt19937 engine;

public:
    SearchRandom() : seed_gen(), engine(seed_gen())
    {
    }

    string name()
    {
        return "Random";
    }

    int search(string &msg)
    {
        vector<int> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else
        {
            uniform_int_distribution<> dist(0, move_list.size() - 1);
            int move_idx = dist(engine);
            return move_list[move_idx];
        }
    }
};

// 最も多くの石をひっくりかえすAI
class SearchGreedy : public SearchBase
{
public:
    SearchGreedy()
    {
    }

    string name()
    {
        return "Greedy";
    }

    int search(string &msg)
    {
        vector<int> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else
        {
            int bestmove = 0;
            int bestcount = -1;
            int player = board.turn();
            for (auto move : move_list)
            {
                UndoInfo undo_info;
                board.do_move(move, undo_info);
                int count = board.count_stone(player);
                if (count > bestcount)
                {
                    bestmove = move;
                    bestcount = count;
                }
                board.undo_move(undo_info);
            }

            return bestmove;
        }
    }
};

// 固定深さでアルファベータ法で探索するAI
class SearchAlphaBetaConstantDepth : public SearchBase
{
    std::random_device seed_gen;
    mt19937 engine;
    uniform_int_distribution<> dist;
    int node_count; // 評価関数を呼び出した回数

public:
    SearchAlphaBetaConstantDepth() : seed_gen(), engine(seed_gen()), dist(0, 255)
    {
    }

    string name()
    {
        return "AlphaBetaConstantDepth";
    }

    int search(string &msg)
    {
        node_count = 0;
        vector<int> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else
        {
            auto search_start_time = chrono::system_clock::now();
            int bestmove;
            int score = alphabeta(5, -100000, 100000, &bestmove) / 256;
            auto search_end_time = chrono::system_clock::now();
            auto search_duration = search_end_time - search_start_time;
            stringstream ss;
            ss << "score " << score << " time " << chrono::duration_cast<chrono::milliseconds>(search_duration).count() << " nodes " << node_count;
            msg = ss.str();

            return bestmove;
        }
    }

    int alphabeta(int depth, int alpha, int beta, int *bestmove)
    {
        if (board.is_end() || depth == 0)
        {
            // 黒から見たスコアなので、手番から見たスコアにする
            // 乱数要素がないと強さ測定が難しいので入れている
            int score = board.count_stone_diff() * 256 + dist(engine);
            if (board.turn() != BLACK)
            {
                score = -score;
            }
            node_count++;
            return score;
        }

        vector<int> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            move_list.push_back(MOVE_PASS);
        }
        for (auto move : move_list)
        {
            UndoInfo undo_info;
            board.do_move(move, undo_info);
            int child_score = -alphabeta(depth - 1, -beta, -alpha, nullptr);
            board.undo_move(undo_info);
            if (child_score > alpha)
            {
                if (bestmove != nullptr)
                {
                    *bestmove = move;
                }
                alpha = child_score;
            }
            if (alpha >= beta)
            {
                return alpha;
            }
        }
        return alpha;
    }
};

// 反復深化探索でアルファベータ法で探索するAI
class SearchAlphaBetaIterative : public SearchBase
{
    std::random_device seed_gen;
    mt19937 engine;
    uniform_int_distribution<> dist;
    int node_count;    // 評価関数を呼び出した回数
    int time_limit_ms; // 探索時間の制限[ms]。これを超えたことを検知したら探索を終了する。ルール上の制限時間より短く設定する必要がある。
    bool stop;         // 探索の内部で、時間切れなどで中断すべき場合にtrueにセットする。
    int check_time_skip;
    chrono::system_clock::time_point time_to_stop_search; // 探索を終了すべき時刻

public:
    SearchAlphaBetaIterative(int time_limit_ms = 1000) : seed_gen(), engine(seed_gen()), dist(0, 255), time_limit_ms(time_limit_ms), check_time_skip(0)
    {
    }

    string name()
    {
        return "AlphaBetaIterative";
    }

    int search(string &msg)
    {
        node_count = 0;
        stop = false;
        vector<int> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else
        {
            auto search_start_time = chrono::system_clock::now();
            time_to_stop_search = search_start_time + chrono::milliseconds(time_limit_ms);
            int bestmove = 0, score = 0, valid_depth = 0;
            for (int depth = 1; depth < 20; depth++)
            {
                int cur_bestmove;
                int cur_score = alphabeta(depth, -100000, 100000, &cur_bestmove) / 256;
                if (stop)
                {
                    // stopで終了した探索は途中で打ち切られているので使用しない
                    break;
                }
                valid_depth = depth;
                bestmove = cur_bestmove;
                score = score;
            }
            auto search_end_time = chrono::system_clock::now();
            auto search_duration = search_end_time - search_start_time;
            stringstream ss;
            ss << "score " << score << " time " << chrono::duration_cast<chrono::milliseconds>(search_duration).count() << " nodes " << node_count << " depth " << valid_depth;
            msg = ss.str();

            return bestmove;
        }
    }

private:
    bool check_stop()
    {
        if (stop)
        {
            return true;
        }

        // システムコール回数を減らす。数msに1回の呼び出しになる。
        if (check_time_skip == 0)
        {
            if (chrono::system_clock::now() > time_to_stop_search)
            {
                stop = true;
                return true;
            }
            check_time_skip = 4096;
        }
        else
        {
            check_time_skip--;
        }
        return false;
    }

    int alphabeta(int depth, int alpha, int beta, int *bestmove)
    {
        if (board.is_end() || depth == 0)
        {
            // 黒から見たスコアなので、手番から見たスコアにする
            // 乱数要素がないと強さ測定が難しいので入れている
            int score = board.count_stone_diff() * 256 + dist(engine);
            if (board.turn() != BLACK)
            {
                score = -score;
            }
            node_count++;
            return score;
        }

        if (check_stop())
        {
            return 0;
        }

        BoardPlane move_bb;
        board.legal_moves_bb(move_bb);
        if (!move_bb)
        {
            int move = MOVE_PASS;
            UndoInfo undo_info;
            board.do_move(move, undo_info);
            int child_score = -alphabeta(depth - 1, -beta, -alpha, nullptr);
            board.undo_move(undo_info);
            if (child_score > alpha)
            {
                if (bestmove != nullptr)
                {
                    *bestmove = move;
                }
                alpha = child_score;
            }
            if (alpha >= beta)
            {
                return alpha;
            }
        }
        else
        {
            for (int move = 0; move < BOARD_AREA; move++)
            {
                if (!(move_bb & position_plane(move)))
                {
                    continue;
                }
                UndoInfo undo_info;
                board.do_move(move, undo_info);
                int child_score = -alphabeta(depth - 1, -beta, -alpha, nullptr);
                board.undo_move(undo_info);
                if (child_score > alpha)
                {
                    if (bestmove != nullptr)
                    {
                        *bestmove = move;
                    }
                    alpha = child_score;
                }
                if (alpha >= beta)
                {
                    return alpha;
                }
            }
        }
        return alpha;
    }
};

#if defined(MODE_INTERACTIVE)
int main()
{
    Board board;
    board.set_hirate();

    while (!board.is_end())
    {
        cout << board.pretty_print();
        cout << "Your turn: "
             << "xo"[board.turn()] << endl;
        vector<int> legal_moves;
        board.legal_moves(legal_moves);
        if (legal_moves.empty())
        {
            cout << "No legal moves." << endl;
            UndoInfo undo_info;
            board.do_move(MOVE_PASS, undo_info);
            continue;
        }
        cout << "Legal moves: ";
        for (auto legal_move : legal_moves)
        {
            cout << move_to_str(legal_move) << ", ";
        }
        cout << endl;

        int move = legal_moves[0];
        while (true)
        {
            string move_str;
            getline(cin, move_str);
            if (!move_str.empty())
            {
                move = move_from_str(move_str);
            }
            else
            {
                move = legal_moves[0];
                break;
            }
            auto idx = find(legal_moves.begin(), legal_moves.end(), move);
            if (idx != legal_moves.end())
            {
                break;
            }
            cout << "Illegal move" << endl;
        }

        UndoInfo undo_info;
        board.do_move(move, undo_info);
    }

    cout << board.pretty_print();
    cout << "End: " << board.count_stone(BLACK) << " - " << board.count_stone(WHITE) << endl;
    int diff = board.count_stone_diff();
    if (diff < 0)
    {
        cout << "WHITE wins" << endl;
    }
    else if (diff > 0)
    {
        cout << "BLACK wins" << endl;
    }
    else
    {
        cout << "DRAW" << endl;
    }

    return 0;
}
#elif defined(MODE_RANDOM_MATCH)
int main()
{
    const int n_games = 100;
    SearchBase *ais[] = {new SearchRandom(), new SearchAlphaBetaIterative(100)};
    int player_win_count[N_PLAYER] = {0};
    int color_win_count[N_PLAYER] = {0};
    int draw_count = 0;
    for (int i = 0; i < n_games; i++)
    {
        cout << "game " << i << endl;
        Board board;
        board.set_hirate();

        int black_player = i % 2;

        while (!board.is_end())
        {
            int turn_player = board.turn() ^ black_player;
            SearchBase *ai = ais[turn_player];
            ai->board.set(board);
            string msg;
            int move = ai->search(msg);

            UndoInfo undo_info;
            board.do_move(move, undo_info);
        }

        int diff = board.count_stone_diff();
        if (diff < 0)
        {
            // white wins
            player_win_count[1 - black_player]++;
            color_win_count[WHITE]++;
        }
        else if (diff > 0)
        {
            // black wins
            player_win_count[black_player]++;
            color_win_count[BLACK]++;
        }
        else
        {
            draw_count++;
        }
    }

    cout << "Summary" << endl;
    cout << ais[0]->name() << " - " << ais[1]->name() << " : " << player_win_count[0] << " - " << draw_count << " - " << player_win_count[1] << endl;
    cout << "black - white : " << color_win_count[0] << " - " << color_win_count[1] << endl;

    return 0;
}
#elif defined(MODE_MAKE_LEGAL_MOVE_TEST_DATA)
// 合法手生成のテストデータを作る。高速化したときに不具合が混入していないかどうかのチェックのため。
int main()
{
    const int n_games = 1000;
    for (int i = 0; i < n_games; i++)
    {
        Board board;
        board.set_hirate();

        while (!board.is_end())
        {
            // 現在局面を出力
            cout << board.get_position_string_with_turn() << "/";
            // 合法手を列挙
            vector<int> move_list;
            board.legal_moves(move_list);
            if (move_list.empty())
            {
                cout << "pass" << endl;
                UndoInfo undo_info;
                board.do_move(MOVE_PASS, undo_info);
                continue;
            }
            // 合法手リストを出力(カンマ区切り)
            bool first_move = true;
            for (auto move : move_list)
            {
                if (!first_move)
                {
                    cout << ",";
                }
                else
                {
                    first_move = false;
                }
                cout << move_to_str(move);
            }
            // 各合法手で1手進めた局面を出力（局面は戻す）
            for (auto move : move_list)
            {
                UndoInfo undo_info;
                board.do_move(move, undo_info);
                cout << "/" << board.get_position_string_with_turn();
                board.undo_move(undo_info);
            }
            cout << endl;
            // 合法手からランダムに1つ選び、局面を進める
            UndoInfo undo_info;
            board.do_move(move_list[i % int(move_list.size())], undo_info);
        }
    }

    return 0;
}
#elif defined(MODE_LEGAL_MOVE_TEST)
// 合法手生成が正しいかテストする
vector<string> string_split(const string &str, char sep)
{
    vector<string> elems;
    string::size_type len = str.length();

    for (string::size_type i = 0, n; i < len; i = n + 1)
    {
        n = str.find_first_of(sep, i);
        if (n == string::npos)
        {
            n = len;
        }
        elems.push_back(str.substr(i, n - i));
    }

    return elems;
}

int main()
{
    string test_case;
    int i = 0;
    int ok = 0;
    while (getline(cin, test_case))
    {
        i++;
        // 入力: 局面/合法手カンマ区切り/合法手1で進めた局面/合法手2で進めた局面...
        auto elems = string_split(test_case, '/');

        Board board;
        board.set_position_string_with_turn(elems[0]);
        auto legal_moves_str = string_split(elems[1], ',');
        vector<int> expected_legal_moves;
        for (auto legal_move_str : legal_moves_str)
        {
            expected_legal_moves.push_back(move_from_str(legal_move_str));
        }
        vector<int> sorted_expected_legal_moves = expected_legal_moves;
        sort(sorted_expected_legal_moves.begin(), sorted_expected_legal_moves.end());

        vector<int> actual_legal_moves;
        board.legal_moves(actual_legal_moves, true);
        sort(actual_legal_moves.begin(), actual_legal_moves.end());

        if (sorted_expected_legal_moves != actual_legal_moves)
        {
            cout << "In case " << i << endl;
            cout << test_case << endl;
            cout << "Expected legal moves:";
            for (auto move : expected_legal_moves)
            {
                cout << " " << move;
            }
            cout << endl;
            cout << "Actual legal moves:";
            for (auto move : actual_legal_moves)
            {
                cout << " " << move;
            }
            cout << endl;

            continue;
        }

        if (actual_legal_moves[0] != MOVE_PASS)
        {
            // 各合法手で進めてみる
            int elem_idx = 2;
            bool success = true;
            for (auto move : expected_legal_moves)
            {
                UndoInfo undo_info;
                board.do_move(move, undo_info);
                auto actual_board = board.get_position_string_with_turn();
                if (actual_board != elems[elem_idx])
                {
                    cout << "In case " << i << endl;
                    cout << test_case << endl;
                    cout << "Board after move " << move_to_str(move) << " does not match." << endl;
                    cout << elems[elem_idx] << " != " << actual_board << endl;
                    success = false;
                    break;
                }
                board.undo_move(undo_info);

                elem_idx++;
            }

            if (!success)
            {
                continue;
            }
        }

        ok++;
    }

    cout << i << " cases, " << ok << " passed, " << (i - ok) << " failed" << endl;

    return i == ok ? 0 : 1;
}
#else
int main()
{
    int my_color; // player's color. BLACK / WHITE.
    cin >> my_color;
    cin.ignore();
    int board_size;
    cin >> board_size;
    cin.ignore();

    if (board_size != BOARD_SIZE)
    {
        return 1;
    }

    SearchBase *ai = new SearchAlphaBetaIterative(120);
    // game loop
    while (1)
    {
        vector<string> position_lines;
        for (int i = 0; i < board_size; i++)
        {
            string line; // rows from top to bottom (viewer perspective).
            cin >> line;
            cin.ignore();
            position_lines.push_back(line);
        }
        ai->board.set_position_codingame(position_lines, my_color);
        int action_count; // number of legal actions for this turn.
        cin >> action_count;
        cin.ignore();

        vector<string> actions;
        for (int i = 0; i < action_count; i++)
        {
            string action; // the action
            cin >> action;
            cin.ignore();
            actions.push_back(action);
        }

        string msg;
        int bestmove = ai->search(msg);
        cout << move_to_str(bestmove) << " MSG " << msg << endl; // a-h1-8
    }
}
#endif
