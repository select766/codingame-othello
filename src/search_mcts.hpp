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
    Board root_board;
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
        bool mate_1ply;    // 一手詰め探索を用いるか
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
        root_node = nullptr;
    }

    // 対局用
    Move search(string &msg)
    {
        auto search_start_time = chrono::system_clock::now();
        time_to_stop_search = search_start_time + chrono::milliseconds(config.time_limit_ms);
        vector<Move> move_list;
        if (config.mate_1ply)
        {
            Move mate_move;
            if (board.legal_moves_with_mate_1ply(move_list, false, mate_move))
            {
                // 詰みあり
                msg = "mate found";
                return mate_move;
            }
        }
        else
        {
            board.legal_moves(move_list, false);
        }
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

    string print_tree() const
    {
        /*
        木構造を可視化
        指し手、静的評価、policy確率、訪問回数、平均報酬
        root
        -d3   +0.12 30% 100 +0.62
        --e3  -0.02 40%  50 +0.25
        -c4   +0.01 40%  25 -0.50
        */
        stringstream ss;
        ss << "move   score policy visit avg" << endl;
        print_tree_recursive(ss, root_node, MOVE_PASS, 0.0F, 0, 0.0F, 0);
        return ss.str();
    }

private:
    void start_search()
    {
        TreeNode *existing_root = find_existing_root(root_node, root_board, board);
        playout_count = 0; // root再利用の場合、すでに子ノードを訪問した回数だけ減らす
        root_board = board;
        // existing_root->terminal()となるのは詰み探索で詰みと判定された場合に起こりうる。ただしsearch()内で同様の詰み判定をしている限りはstart_search()は実行されない。詰み判定基準が異なる場合にはこの条件判断が起こりうる。
        if (existing_root && !existing_root->terminal())
        {
            // 探索木中にルート局面があった
            for (int edge = 0; edge < existing_root->n_legal_moves; edge++)
            {
                playout_count += existing_root->value_n[edge];
            }
            root_node = existing_root;
        }
        else
        {
            make_root(board);
        }
    }

    void make_root(const Board &b)
    {
        bool mate_found;
        Move mate_move;
        // 詰み探索は、searchの合法手列挙のタイミングで実施済み
        root_node = MCTSBase::make_node(b, tree_table.get(), false, mate_found, mate_move);
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

    TreeNode *find_existing_root(TreeNode *last_root_node, const Board &last_root_board, const Board &query)
    {
        // 探索開始局面が既存の木に存在すれば、それを返す
        if (last_root_node == nullptr)
        {
            return nullptr;
        }
        Board b(last_root_board);
        return find_existing_root_recursive(last_root_node, b, query, 4); // パスや、合法手が1つしかなかった場合、2手では足りない可能性がある
    }

    TreeNode *find_existing_root_recursive(TreeNode *node, Board &b, const Board &query, int depth)
    {
        if (b == query)
        {
            return node;
        }
        if (depth == 0)
        {
            return nullptr;
        }
        if (node->terminal())
        {
            return nullptr;
        }

        for (int edge = 0; edge < node->n_legal_moves; edge++)
        {
            int child_node_idx = node->children[edge];
            if (child_node_idx)
            {
                UndoInfo undo_info;
                b.do_move(node->move_list[edge], undo_info);
                TreeNode *found = find_existing_root_recursive(tree_table->at(child_node_idx), b, query, depth - 1);
                b.undo_move(undo_info);
                if (found)
                {
                    return found;
                }
            }
        }

        return nullptr;
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
            bool mate_found;
            Move mate_move;
            TreeNode *child_node = MCTSBase::make_node(b, tree_table.get(), config.mate_1ply, mate_found, mate_move);
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

    void print_tree_recursive(stringstream &ss, const TreeNode *node, Move last_move, float value_p, int value_n, float avg_w, int depth) const
    {
        float sign = depth % 2 == 0 ? 1.0 : -1.0; // rootからみた評価値の符号にする
        if (depth != 0)
        {
            for (int i = 0; i < depth - 1; i++)
            {
                ss << " ";
            }
            ss << move_to_str(last_move);
            for (int i = 0; i < 6 - depth; i++)
            {
                ss << " ";
            }
            char buf[1024];
            if (node)
            {
                snprintf(buf, sizeof(buf), "%+.2f ", node->score * sign);
            }
            else
            {
                snprintf(buf, sizeof(buf), "    * ");
            }
            ss << buf;
            snprintf(buf, sizeof(buf), "%2d%%    %5d %+.2f", int(value_p * 100), value_n, avg_w);
            ss << buf << endl;
        }

        if (node)
        {
            // 訪問回数の降順で表示
            auto &child_value_n = node->value_n;
            int sum_value_n = reduce(&child_value_n[0], &child_value_n[node->n_legal_moves]);
            if (sum_value_n == 0)
            {
                // 子ノードのいずれにも訪問していない場合は、子ノードリストを表示しない（非常に長くなるため）
                return;
            }
            std::vector<size_t> indices(node->n_legal_moves);
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(),
                      [&child_value_n](int left, int right) -> bool
                      {
                          // sort indices according to corresponding array element
                          return child_value_n[left] > child_value_n[right];
                      });
            for (auto edge : indices)
            {
                int child_idx = node->children[edge];
                TreeNode *child_node = nullptr;
                if (child_idx)
                {
                    child_node = tree_table->at(child_idx);
                }
                print_tree_recursive(ss, child_node, node->move_list[edge], node->value_p[edge], node->value_n[edge], node->value_w[edge] / (max(node->value_n[edge], 1)) * sign, depth + 1);
            }
        }
    }
};

#endif
