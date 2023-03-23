#include "common.hpp"

#pragma GCC optimize("O3")

int main()
{
    Color my_color; // player's color. BLACK / WHITE.
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
    ai->newgame();
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
        Move bestmove = ai->search(msg);
        cout << move_to_str(bestmove) << " MSG " << msg << endl; // a-h1-8
    }
}
