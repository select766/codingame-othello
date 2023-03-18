#ifndef _SEARCH_GREEDY_
#define _SEARCH_GREEDY_
#include "search_base.hpp"

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
                int count = board.piece_num(player);
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

#endif
