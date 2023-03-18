#include "common.hpp"

int main()
{
    Board board;
    board.set_hirate();

    while (!board.is_gameover())
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
    cout << "End: " << board.piece_num(BLACK) << " - " << board.piece_num(WHITE) << endl;
    int winner = board.winner();
    if (winner == WHITE)
    {
        cout << "WHITE wins" << endl;
    }
    else if (winner == BLACK)
    {
        cout << "BLACK wins" << endl;
    }
    else
    {
        cout << "DRAW" << endl;
    }

    return 0;
}
