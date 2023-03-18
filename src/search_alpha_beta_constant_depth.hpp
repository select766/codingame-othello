#ifndef _SEARCH_ALPHA_BETA_CONSTANT_DEPTH_
#define _SEARCH_ALPHA_BETA_CONSTANT_DEPTH_
#include "search_base.hpp"

// 固定深さでアルファベータ法で探索するAI
class SearchAlphaBetaConstantDepth : public SearchBase
{
    std::random_device seed_gen;
    mt19937 engine;
    uniform_int_distribution<> dist;
    int node_count; // 評価関数を呼び出した回数
    int depth;

public:
    SearchAlphaBetaConstantDepth(int depth = 5) : seed_gen(), engine(seed_gen()), dist(0, 255), depth(depth)
    {
    }

    string name()
    {
        return "AlphaBetaConstantDepth";
    }

    Move search(string &msg)
    {
        node_count = 0;
        vector<Move> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else
        {
            auto search_start_time = chrono::system_clock::now();
            int bestmove;
            int score = alphabeta(depth, -100000, 100000, &bestmove) / 256;
            auto search_end_time = chrono::system_clock::now();
            auto search_duration = search_end_time - search_start_time;
            stringstream ss;
            ss << "score " << score << " time " << chrono::duration_cast<chrono::milliseconds>(search_duration).count() << " nodes " << node_count;
            msg = ss.str();

            return bestmove;
        }
    }

    int alphabeta(int depth, int alpha, int beta, Move *bestmove)
    {
        if (board.is_gameover() || depth == 0)
        {
            // 乱数要素がないと強さ測定が難しいので入れている
            int score = board.count_stone_diff() * 256 + dist(engine);
            node_count++;
            return score;
        }

        vector<Move> move_list;
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

#endif
