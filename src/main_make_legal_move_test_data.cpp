#include "common.hpp"

// 合法手生成のテストデータを作る。高速化したときに不具合が混入していないかどうかのチェックのため。
int main()
{
    const int n_games = 1000;
    for (int i = 0; i < n_games; i++)
    {
        Board board;
        board.set_hirate();

        while (!board.is_gameover())
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
