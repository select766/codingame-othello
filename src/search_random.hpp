#ifndef _SEARCH_RANDOM_
#define _SEARCH_RANDOM_
#include "search_base.hpp"

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

    Move search(string &msg)
    {
        vector<Move> move_list;
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

#endif
