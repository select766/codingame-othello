#include "common.hpp"

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
