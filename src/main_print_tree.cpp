#include "common.hpp"

// 局面を与えて、MCTS探索木を可視化する

// 盤面入力。複数行に分けてもよい。
bool input_board(Board &b)
{
    string ssum;
    string s;
    while (getline(cin, s))
    {
        ssum += s;
        if (ssum.size() < 66) continue;
        b.set_position_string_with_turn(ssum);
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    shared_ptr<DNNEvaluator> evaluator(new DNNEvaluatorEmbed());
    SearchMCTS::SearchMCTSConfig mcts_config;
    // usage: print_tree [playout_limit]
    if (argc >= 2)
    {
        mcts_config.playout_limit = atoi(argv[1]);
    }
    else
    {
        mcts_config.playout_limit = 16;
    }
    mcts_config.table_size = mcts_config.playout_limit * 2;
    mcts_config.c_puct = 1.0;
    mcts_config.time_limit_ms = 1000;
    mcts_config.mate_1ply = true;
    SearchMCTS *ai = new SearchMCTS(mcts_config, evaluator);
    ai->newgame();

    Board b;
    while (input_board(b))
    {
        string msg;
        ai->newgame();
        ai->board.set(b);
        ai->search(msg);
        cout << b.pretty_print() << endl;
        cout << ai->print_tree() << endl;
    }

    return 0;
}
