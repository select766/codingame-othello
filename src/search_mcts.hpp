#ifndef _SEARCH_MCTS_
#define _SEARCH_MCTS_
#include <memory>
#include <cassert>
#include "dnn_evaluator.hpp"
#include "mcts_base.hpp"

// MCTS
class SearchMCTS : public SearchBase
{
    class ChooseMoveResult
    {
    public:
        Move move;
        float score;
    };

    shared_ptr<TreeTable> tree_table;
    int playout_count;

    TreeNode *root_node;
    shared_ptr<DNNEvaluator> dnn_evaluator;
    chrono::system_clock::time_point time_to_stop_search; // 探索を終了すべき時刻

public:
    class SearchMCTSConfig
    {
    public:
        int playout_limit;
        size_t table_size;
        float c_puct;
        int time_limit_ms; // 探索時間の制限[ms]。これを超えたことを検知したら探索を終了する。ルール上の制限時間より短く設定する必要がある。
    };

private:
    SearchMCTSConfig config;

public:
    SearchMCTS(const SearchMCTSConfig &config, shared_ptr<DNNEvaluator> dnn_evaluator)
        : dnn_evaluator(dnn_evaluator),
          config(config),
          tree_table(new TreeTable(config.table_size)),
          root_node(nullptr)
    {
    }

    string name()
    {
        return "MCTS";
    }

    void newgame()
    {
        tree_table->clear();
    }

    // 対局用
    Move search(string &msg)
    {
        auto search_start_time = chrono::system_clock::now();
        time_to_stop_search = search_start_time + chrono::milliseconds(config.time_limit_ms);
        vector<Move> move_list;
        board.legal_moves(move_list);
        if (move_list.empty())
        {
            return MOVE_PASS;
        }
        else if (move_list.size() == 1)
        {
            return move_list[0];
        }
        else
        {
            start_search();

            while (true)
            {
                if (playout_count >= config.playout_limit || chrono::system_clock::now() > time_to_stop_search)
                {
                    // playoutは終わり。指し手を決定する。
                    break;
                }

                search_tree();
            }

            auto choose_move_result = choose_move();
            stringstream ss;
            ss << "value score " << choose_move_result.score << " playouts " << playout_count;
            msg = ss.str();
            return choose_move_result.move;
        }
    }

private:
    void start_search()
    {
        playout_count = 0; // root再利用の場合、すでに子ノードを訪問した回数だけ減らすことが考えられる
        make_root(board);
    }

    void make_root(const Board &b)
    {
        // TODO: 探索木の再利用
        root_node = MCTSBase::make_node(b, tree_table.get());
        if (!root_node->terminal())
        {
            // 評価が必要
            auto eval_result = dnn_evaluator->evaluate(b);
            assign_eval_result_to_leaf(root_node, &eval_result);
        }
        else
        {
            // ゲーム終了しているときに呼ばれるのは異常
            cerr << "make_root called on terminal" << endl;
            exit(1);
        }
    }

    float assign_eval_result_to_leaf(TreeNode *leaf, const DNNEvaluatorResult *eval_result)
    {
        leaf->score = tanh(eval_result->value_logit);
        assert(leaf->n_legal_moves);
        // 合法手のみでsoftmaxを計算
        float max_logit = -1000.0F;
        for (int i = 0; i < leaf->n_legal_moves; i++)
        {
            Move m = leaf->move_list[i];
            float logit = eval_result->policy_logits[m];
            if (max_logit < logit)
            {
                max_logit = logit;
            }
        }
        float expsum = 0.0F;
        for (int i = 0; i < leaf->n_legal_moves; i++)
        {
            Move m = leaf->move_list[i];
            float logit = eval_result->policy_logits[m];
            float explogit = exp(logit - max_logit);
            expsum += explogit;
            leaf->value_p[i] = explogit;
        }
        for (int i = 0; i < leaf->n_legal_moves; i++)
        {
            leaf->value_p[i] /= expsum;
        }

        return leaf->score;
    }

    void search_tree()
    {
        playout_count++;
        search_root();
    }

    void search_root()
    {
        vector<pair<TreeNode *, int>> path;
        search_recursive(board, root_node, path);
    }

    void search_recursive(Board &b, TreeNode *node, vector<pair<TreeNode *, int>> &path)
    {
        if (node->terminal())
        {
            backup_path(path, node->score);
            return;
        }

        int edge = MCTSBase::select_edge(node, config.c_puct);
        UndoInfo undo_info;
        b.do_move(static_cast<Move>(node->move_list[edge]), undo_info);
        path.push_back({node, edge});
        int child_node_idx = node->children[edge];
        node->value_n[edge]++;
        if (child_node_idx)
        {
            search_recursive(b, tree_table->at(child_node_idx), path);
        }
        else
        {
            // 子ノードがまだ生成されていない
            TreeNode *child_node = MCTSBase::make_node(b, tree_table.get());
            node->children[edge] = tree_table->get_index(child_node);
            if (!child_node->terminal())
            {
                // 評価が必要
                auto eval_result = dnn_evaluator->evaluate(b);
                assign_eval_result_to_leaf(child_node, &eval_result);
            }
            // backup
            backup_path(path, child_node->score);
        }

        b.undo_move(undo_info);
    }

    void backup_path(const vector<pair<TreeNode *, int>> &path, float leaf_score)
    {
        float score = leaf_score;
        for (int i = int(path.size()) - 1; i >= 0; i--)
        {
            score = -score;
            path[i].first->value_w[path[i].second] += score;
        }
    }

    ChooseMoveResult choose_move()
    {
        Move move = MOVE_PASS;
        float score = 0.0F;
        if (!root_node->terminal()) // terminalの場合はそもそもsearchが呼ばれないはず
        {
            // TODO: 必要に応じてランダム要素を入れる
            int best_n = -1;
            int best_idx = 0;
            for (int i = 0; i < root_node->n_legal_moves; i++)
            {
                int value_n = root_node->value_n[i];
                if (best_n < value_n)
                {
                    best_n = value_n;
                    best_idx = i;
                }
            }
            move = static_cast<Move>(root_node->move_list[best_idx]);
            score = root_node->value_w[best_idx] / static_cast<float>(root_node->value_n[best_idx]);
        }

        ChooseMoveResult res;
        res.move = move;
        res.score = score;
        return res;
    }
};

#endif
