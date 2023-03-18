#include "common.hpp"

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
