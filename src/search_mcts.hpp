#ifndef _SEARCH_MCTS_
#define _SEARCH_MCTS_
#include <memory>
#include <cassert>
#include "dnn_evaluator.hpp"
#include "mcts_base.hpp"

// MCTS
class SearchMCTS : public SearchBase
{

    class SearchPartialResult
    {
    public:
        virtual ~SearchPartialResult() = default;
    };

    class SearchPartialResultMove : public SearchPartialResult
    {
    public:
        Move move;
        float score;
    };

    class SearchPartialResultEvalRequest : public SearchPartialResult
    {
    public:
        Board board;                             // 評価対象局面
        vector<pair<TreeNode *, int>> tree_path; // 探索木の経路。TreeNodeと、その中のエッジのインデックスのペア。leafがルートの場合は空。
        TreeNode *leaf;                          // 末端ノードのTreeNode。
    };

    class EvalResult
    {
    public:
        shared_ptr<SearchPartialResultEvalRequest> request;
        float value_logit;
        float policy_logits[BOARD_AREA];
    };

    shared_ptr<TreeTable> tree_table;
    enum NextTask
    {
        START_SEARCH,
        ASSIGN_ROOT_EVAL,
        SEARCH_TREE,
        ASSIGN_LEAF_EVAL,
        CHOOSE_MOVE,
    };
    NextTask next_task;
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
          next_task(START_SEARCH),
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
            // TODO: 探索末端でdnn_evaluatorをコールする単純な構造に書き換え
            EvalResult *eval_result = nullptr;
            while (true)
            {
                auto search_partial_result = search_partial(eval_result);
                delete eval_result;
                eval_result = nullptr;
                auto result_move = dynamic_pointer_cast<SearchPartialResultMove>(search_partial_result);
                if (result_move)
                {
                    stringstream ss;
                    ss << "value score " << result_move->score << " playouts " << playout_count;
                    msg = ss.str();
                    return result_move->move;
                }
                auto result_eval = dynamic_pointer_cast<SearchPartialResultEvalRequest>(search_partial_result);
                if (result_eval)
                {
                    auto dnn_result = dnn_evaluator->evaluate(result_eval->board);
                    eval_result = new EvalResult();
                    memcpy(eval_result->policy_logits, dnn_result.policy_logits, sizeof(dnn_result.policy_logits));
                    eval_result->value_logit = dnn_result.value_logit;
                    eval_result->request = result_eval;
                }
            }
        }
    }

    // 局面の評価が必要か、指し手が決定するまで探索する
    shared_ptr<SearchPartialResult> search_partial(const EvalResult *eval_result)
    {
        shared_ptr<SearchPartialResult> result;
        do
        {
            switch (next_task)
            {
            case NextTask::START_SEARCH:
                result = start_search();
                break;
            case NextTask::ASSIGN_ROOT_EVAL:
                result = assign_root_eval(eval_result);
                break;
            case NextTask::SEARCH_TREE:
                result = search_tree();
                break;
            case NextTask::ASSIGN_LEAF_EVAL:
                result = assign_leaf_eval(eval_result);
                break;
            case NextTask::CHOOSE_MOVE:
                result = choose_move();
                break;
            }
        } while (!result);

        return result;
    }

private:
    shared_ptr<SearchPartialResult> start_search()
    {
        playout_count = 0; // root再利用の場合、すでに子ノードを訪問した回数だけ減らすことが考えられる
        return make_root(board);
    }

    shared_ptr<SearchPartialResult> make_root(const Board &b)
    {
        // TODO: 探索木の再利用
        root_node = MCTSBase::make_node(b, tree_table.get());
        if (!root_node->terminal())
        {
            // 評価が必要
            SearchPartialResultEvalRequest *req = new SearchPartialResultEvalRequest();
            req->board.set(b);
            req->leaf = root_node;
            next_task = NextTask::ASSIGN_ROOT_EVAL;
            return shared_ptr<SearchPartialResult>(req);
        }
        else
        {
            // ゲーム終了しているときに呼ばれるのは異常
            cerr << "make_root called on terminal" << endl;
            exit(1);
        }
    }

    shared_ptr<SearchPartialResult> assign_root_eval(const EvalResult *eval_result)
    {
        assert(eval_result);
        auto prev_request = dynamic_pointer_cast<SearchPartialResultEvalRequest>(eval_result->request);
        assert(prev_request);
        auto leaf = prev_request->leaf;
        assign_eval_result_to_leaf(leaf, eval_result);
        // TODO: 学習データ作成時、ルートノードではディリクレノイズを加算。
        next_task = NextTask::SEARCH_TREE;
        return nullptr;
    }

    shared_ptr<SearchPartialResult> assign_leaf_eval(const EvalResult *eval_result)
    {
        assert(eval_result);
        auto prev_request = dynamic_pointer_cast<SearchPartialResultEvalRequest>(eval_result->request);
        assert(prev_request);
        auto leaf = prev_request->leaf;
        assign_eval_result_to_leaf(leaf, eval_result);
        backup_path(prev_request->tree_path, leaf->score);
        next_task = NextTask::SEARCH_TREE;
        return nullptr;
    }

    float assign_eval_result_to_leaf(TreeNode *leaf, const EvalResult *eval_result)
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

    shared_ptr<SearchPartialResult> search_tree()
    {
        if (playout_count >= config.playout_limit || chrono::system_clock::now() > time_to_stop_search)
        {
            // playoutは終わり。指し手を決定する。
            next_task = NextTask::CHOOSE_MOVE;
            return nullptr;
        }

        playout_count++;
        auto result = search_root();
        if (result)
        {
            next_task = NextTask::ASSIGN_LEAF_EVAL;
        }
        else
        {
            next_task = NextTask::SEARCH_TREE;
        }
        return result;
    }

    shared_ptr<SearchPartialResult> search_root()
    {
        vector<pair<TreeNode *, int>> path;
        return search_recursive(board, root_node, path);
    }

    shared_ptr<SearchPartialResult> search_recursive(Board &b, TreeNode *node, vector<pair<TreeNode *, int>> &path)
    {
        if (node->terminal())
        {
            backup_path(path, node->score);
            return nullptr;
        }

        int edge = MCTSBase::select_edge(node, config.c_puct);
        UndoInfo undo_info;
        b.do_move(static_cast<Move>(node->move_list[edge]), undo_info);
        path.push_back({node, edge});
        int child_node_idx = node->children[edge];
        node->value_n[edge]++;
        shared_ptr<SearchPartialResult> result;
        if (child_node_idx)
        {
            result = search_recursive(b, tree_table->at(child_node_idx), path);
        }
        else
        {
            // 子ノードがまだ生成されていない
            TreeNode *child_node = MCTSBase::make_node(b, tree_table.get());
            node->children[edge] = tree_table->get_index(child_node);
            if (!child_node->terminal())
            {
                // 評価が必要
                SearchPartialResultEvalRequest *req = new SearchPartialResultEvalRequest();
                req->board.set(b);
                req->leaf = child_node;
                req->tree_path = path;
                result = shared_ptr<SearchPartialResult>(req);
            }
            else
            {
                // backup
                backup_path(path, child_node->score);
            }
        }

        b.undo_move(undo_info);
        return result;
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

    shared_ptr<SearchPartialResult> choose_move()
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

        next_task = NextTask::START_SEARCH;
        auto result = new SearchPartialResultMove();
        result->move = move;
        result->score = score;
        return shared_ptr<SearchPartialResult>(result);
    }
};

#endif
