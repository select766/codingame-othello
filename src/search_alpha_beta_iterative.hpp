#ifndef _SEARCH_ALPHA_BETA_ITERATIVE_
#define _SEARCH_ALPHA_BETA_ITERATIVE_
#include "search_base.hpp"

// 反復深化探索でアルファベータ法で探索するAI
class SearchAlphaBetaIterative : public SearchBase
{
    std::random_device seed_gen;
    mt19937 engine;
    uniform_int_distribution<> dist;
    int node_count;    // 評価関数を呼び出した回数
    int time_limit_ms; // 探索時間の制限[ms]。これを超えたことを検知したら探索を終了する。ルール上の制限時間より短く設定する必要がある。
    bool stop;         // 探索の内部で、時間切れなどで中断すべき場合にtrueにセットする。
    int check_time_skip;
    chrono::system_clock::time_point time_to_stop_search; // 探索を終了すべき時刻

public:
    SearchAlphaBetaIterative(int time_limit_ms = 1000) : seed_gen(), engine(seed_gen()), dist(0, 255), time_limit_ms(time_limit_ms), check_time_skip(0)
    {
    }

    string name()
    {
        return "AlphaBetaIterative";
    }

    Move search(string &msg)
    {
        node_count = 0;
        stop = false;
        vector<Move> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else
        {
            auto search_start_time = chrono::system_clock::now();
            time_to_stop_search = search_start_time + chrono::milliseconds(time_limit_ms);
            Move bestmove = 0;
            int score = 0, valid_depth = 0;
            for (int depth = 1; depth < 20; depth++)
            {
                Move cur_bestmove;
                int cur_score = alphabeta(depth, -100000, 100000, &cur_bestmove) / 256;
                if (stop)
                {
                    // stopで終了した探索は途中で打ち切られているので使用しない
                    break;
                }
                valid_depth = depth;
                bestmove = cur_bestmove;
                score = score;
            }
            auto search_end_time = chrono::system_clock::now();
            auto search_duration = search_end_time - search_start_time;
            stringstream ss;
            ss << "score " << score << " time " << chrono::duration_cast<chrono::milliseconds>(search_duration).count() << " nodes " << node_count << " depth " << valid_depth;
            msg = ss.str();

            return bestmove;
        }
    }

private:
    bool check_stop()
    {
        if (stop)
        {
            return true;
        }

        // システムコール回数を減らす。数msに1回の呼び出しになる。
        if (check_time_skip == 0)
        {
            if (chrono::system_clock::now() > time_to_stop_search)
            {
                stop = true;
                return true;
            }
            check_time_skip = 4096;
        }
        else
        {
            check_time_skip--;
        }
        return false;
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

        if (check_stop())
        {
            return 0;
        }

        BoardPlane move_bb;
        board.legal_moves_bb(move_bb);
        if (!move_bb)
        {
            int move = MOVE_PASS;
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
        else
        {
            for (Move move = 0; move < BOARD_AREA; move++)
            {
                if (!(move_bb & position_plane(move)))
                {
                    continue;
                }
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
        }
        return alpha;
    }
};
#endif
