#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>

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

inline bool move_is_pass(int move)
{
    return move == MOVE_PASS;
}

class UndoInfo
{
public:
    int board[N_PLAYER][BOARD_AREA]; // 手番、各マスの石の有無
    int total_stones;
    int turn; // BLACK / WHITE
    bool last_pass[N_PLAYER];
};

#define N_DIR 8
const int dirsx[N_DIR] = {-1, 0, 1, -1, 1, -1, 0, 1};
const int dirsy[N_DIR] = {-1, -1, -1, 0, 0, 1, 1, 1};

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
    // TODO bitboard
    int board[N_PLAYER][BOARD_AREA]; // 手番、各マスの石の有無
    int total_stones;
    int _turn; // BLACK / WHITE
    bool last_pass[N_PLAYER];

public:
    Board()
    {
        set_hirate();
    }

    void set(const Board& other) {
        memcpy(board, other.board, sizeof(board));
        total_stones = other.total_stones;
        _turn = other._turn;
        memcpy(last_pass, other.last_pass, sizeof(last_pass));
    }

    int turn() const
    {
        return _turn;
    }

    void set_hirate()
    {
        memset(board, 0, N_PLAYER * BOARD_AREA * sizeof(int));
        board[WHITE][R_D + C_4] = 1;
        board[BLACK][R_E + C_4] = 1;
        board[BLACK][R_D + C_5] = 1;
        board[WHITE][R_E + C_5] = 1;
        total_stones = 4;
        last_pass[0] = false;
        last_pass[1] = false;
        _turn = BLACK;
    }

    void set_position_codingame(const vector<string> &lines, int turn)
    {
        // lines[0]: a1 to h1, '.' = nothing, '0' = black, '1' = white
        memset(board, 0, N_PLAYER * BOARD_AREA * sizeof(int));
        total_stones = 0;
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
                board[color][row * BOARD_SIZE + col] = 1;
                total_stones++;
            }
        }
        // 相手が最後にパスしている可能性もあるが、codingameで指し手を求められるのは合法手がある場合のみであり、指し手生成に影響なし。
        last_pass[0] = false;
        last_pass[1] = false;
        this->_turn = turn;
    }

    void do_move(int move, UndoInfo &undo_info)
    {
        memcpy(undo_info.board, board, sizeof(board));
        undo_info.total_stones = total_stones;
        undo_info.turn = _turn;
        undo_info.last_pass[0] = last_pass[0];
        undo_info.last_pass[1] = last_pass[1];
        if (!move_is_pass(move))
        {
            do_flip(move);
            board[_turn][move] = 1;
            total_stones++;
            last_pass[_turn] = 0;
        }
        else
        {
            last_pass[_turn] = 1;
        }
        _turn = 1 - _turn;
    }

    void undo_move(const UndoInfo &undo_info)
    {
        memcpy(board, undo_info.board, sizeof(board));
        total_stones = undo_info.total_stones;
        _turn = undo_info.turn;
        last_pass[0] = undo_info.last_pass[0];
        last_pass[1] = undo_info.last_pass[1];
    }

    void legal_moves(vector<int> &move_list) const
    {
        move_list.clear();
        for (int pos = 0; pos < BOARD_AREA; pos++)
        {
            if (can_move(pos))
            {
                move_list.push_back(pos);
            }
        }
    }

    bool can_move(int move) const
    {
        if (board[0][move] != 0 || board[1][move] != 0)
        {
            return false;
        }

        int opp = 1 - _turn;
        for (int i = 0; i < N_DIR; i++)
        {
            int dirx = dirsx[i];
            int diry = dirsy[i];
            int posx = move % BOARD_SIZE;
            int posy = move / BOARD_SIZE;
            int opp_len = 0;
            int flip_len = 0;
            // 相手の石が連続する数を計算。最後に自分の石があれば、相手の石の数だけひっくりかえせる
            while (true)
            {
                posx += dirx;
                posy += diry;
                if (posx < 0 || posx >= BOARD_SIZE || posy < 0 || posy >= BOARD_SIZE)
                {
                    break;
                }
                int p = posx + posy * BOARD_SIZE;
                if (board[opp][p])
                {
                    opp_len++;
                }
                else if (board[_turn][p])
                {
                    flip_len = opp_len;
                    break;
                }
                else
                {
                    break;
                }
            }

            if (flip_len > 0)
            {
                return true;
            }
        }

        return false;
    }

    bool is_end() const
    {
        if (last_pass[0] && last_pass[1])
        {
            return true;
        }
        if (total_stones == BOARD_AREA)
        {
            return true;
        }
        return false;
    }

    int count_stone(int color) const
    {
        int v = 0;
        for (int i = 0; i < BOARD_AREA; i++)
        {
            v += board[color][i];
        }
        return v;
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
                if (board[BLACK][p])
                {
                    s.append("x");
                }
                else if (board[WHITE][p])
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
    void do_flip(int move)
    {
        int opp = 1 - _turn;
        for (int i = 0; i < N_DIR; i++)
        {
            int dirx = dirsx[i];
            int diry = dirsy[i];
            int posx = move % BOARD_SIZE;
            int posy = move / BOARD_SIZE;
            int opp_len = 0;
            int flip_len = 0;
            // 相手の石が連続する数を計算。最後に自分の石があれば、相手の石の数だけひっくりかえせる
            while (true)
            {
                posx += dirx;
                posy += diry;
                if (posx < 0 || posx >= BOARD_SIZE || posy < 0 || posy >= BOARD_SIZE)
                {
                    break;
                }
                int p = posx + posy * BOARD_SIZE;
                if (board[opp][p])
                {
                    opp_len++;
                }
                else if (board[_turn][p])
                {
                    flip_len = opp_len;
                    break;
                }
                else
                {
                    break;
                }
            }

            posx = move % BOARD_SIZE;
            posy = move / BOARD_SIZE;
            for (int j = 0; j < flip_len; j++)
            {
                posx += dirx;
                posy += diry;
                int p = posx + posy * BOARD_SIZE;
                board[opp][p] = 0;
                board[_turn][p] = 1;
            }
        }
    }
};

// AIのインターフェース
class SearchBase
{
public:
    Board board;
    virtual int search() = 0;
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

    string name() {
        return "Random";
    }

    int search()
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
class SearchGreedy : public SearchBase {
public:
    SearchGreedy()
    {
    }

    string name() {
        return "Greedy";
    }

    int search()
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
            for (auto move : move_list) {
                UndoInfo undo_info;
                board.do_move(move, undo_info);
                int count = board.count_stone(player);
                if (count > bestcount) {
                    bestmove = move;
                    bestcount = count;
                }
                board.undo_move(undo_info);
            }

            return bestmove;
        }
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
    const int n_games = 1000;
    SearchBase *ais[] = {new SearchRandom(), new SearchGreedy()};
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
            int move = ai->search();           

            UndoInfo undo_info;
            board.do_move(move, undo_info);
        }

        int diff = board.count_stone_diff();
        if (diff < 0)
        {
            // white wins
            player_win_count[1-black_player]++;
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
#else
int main()
{
    std::random_device seed_gen;
    mt19937 engine(seed_gen());

    int id; // player's color. BLACK / WHITE.
    cin >> id;
    cin.ignore();
    int board_size;
    cin >> board_size;
    cin.ignore();

    if (board_size != BOARD_SIZE)
    {
        return 1;
    }

    // game loop
    while (1)
    {
        for (int i = 0; i < board_size; i++)
        {
            string line; // rows from top to bottom (viewer perspective).
            cin >> line;
            cin.ignore();
        }
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

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        uniform_int_distribution<> dist(0, actions.size() - 1);

        int action_idx = dist(engine);

        cout << actions[action_idx] << endl; // a-h1-8
    }
}
#endif
