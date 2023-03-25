#ifndef _SEARCH_MCTS_TRAIN
#define _SEARCH_MCTS_TRAIN
#include <memory>
#include <cassert>
#include "mcts_base.hpp"

// MCTS
class SearchMCTSTrain : public SearchBase
{
public:

    class SearchMCTSConfig
    {
    public:
        // プレイアウト回数
        int playout_limit;
        // 置換表の要素数
        size_t table_size;
        // プレイアウト時の子ノード選択のパラメータ
        float c_puct;
        // ルートノードに加算するディリクレノイズのパラメータ
        float root_noise_dirichret_alpha;
        // ルートノードに加算するノイズの重み(0=ノイズなし、最大1)
        float root_noise_epsilon;
        // 盤上の石の数がこの値以下の時、ノードの訪問回数に比例した確率で指し手を選択する
        int select_move_proportional_until_move;
    };

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
        float value_logit;
        float policy_logits[BOARD_AREA];
    };

private:
    const SearchMCTSConfig config;
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

    random_device seed_gen;
    default_random_engine random_engine;
    gamma_distribution<float> gamma_distribution_for_dirichret;
    shared_ptr<SearchPartialResultEvalRequest> prev_request;

public:

    SearchMCTSTrain(const SearchMCTSConfig &config)
        : config(config),
          tree_table(new TreeTable(config.table_size)),
          next_task(START_SEARCH),
          root_node(nullptr),
          random_engine(seed_gen()),
          gamma_distribution_for_dirichret(config.root_noise_dirichret_alpha, 1.0F)
    {
    }

    string name()
    {
        return "MCTSTrain";
    }

    void newgame()
    {
        tree_table->clear();
        root_node = nullptr;
        next_task = START_SEARCH;
    }

    Move search(string &msg)
    {
        // 非対応
        return MOVE_PASS;
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

        prev_request = dynamic_pointer_cast<SearchPartialResultEvalRequest>(result);

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
        // 学習時は探索木の再利用をしない。ルートにディレクレノイズを足した状態で探索する必要があるため、ノイズが足されていない子ノードをルートとして再利用すると結果が変化する。
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

    vector<float> make_dirichret(size_t size)
    {
        vector<float> d(size);
        float sum = 0.0F;
        for (size_t i = 0; i < size; i++)
        {
            float r = gamma_distribution_for_dirichret(random_engine);
            sum += r;
            d[i] = r;
        }
        for (size_t i = 0; i < size; i++)
        {
            d[i] /= sum;
        }

        return d;
    }

    shared_ptr<SearchPartialResult> assign_root_eval(const EvalResult *eval_result)
    {
        assert(eval_result);
        assert(prev_request);
        auto leaf = prev_request->leaf;
        assign_eval_result_to_leaf(leaf, eval_result);

        // ルートノードにディリクレノイズを加算。
        if (config.root_noise_epsilon > 0.0F)
        {
            auto dirichret = make_dirichret(prev_request->leaf->n_legal_moves);
            auto value_p = prev_request->leaf->value_p;
            for (int i = 0; i < prev_request->leaf->n_legal_moves; i++)
            {
                value_p[i] = (1.0F - config.root_noise_epsilon) * value_p[i] + config.root_noise_epsilon * dirichret[i];
            }
        }

        next_task = NextTask::SEARCH_TREE;
        prev_request = nullptr;
        return nullptr;
    }

    shared_ptr<SearchPartialResult> assign_leaf_eval(const EvalResult *eval_result)
    {
        assert(eval_result);
        assert(prev_request);
        auto leaf = prev_request->leaf;
        assign_eval_result_to_leaf(leaf, eval_result);
        backup_path(prev_request->tree_path, leaf->score);
        prev_request = nullptr;
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
        if (playout_count >= config.playout_limit)
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
            int best_idx = 0;
            if (board.piece_sum() <= config.select_move_proportional_until_move)
            {
                // 訪問回数に比例
                int v_sum = 0;
                for (int i = 0; i < root_node->n_legal_moves; i++)
                {
                    int value_n = root_node->value_n[i];
                    v_sum += value_n;
                }

                uniform_int_distribution<int> dist(0, v_sum-1);
                int ctr = dist(random_engine);

                for (int i = 0; i < root_node->n_legal_moves; i++)
                {
                    int value_n = root_node->value_n[i];
                    if (ctr < value_n)
                    {
                        best_idx = i;
                        break;
                    }
                    ctr -= value_n;
                }
            }
            else
            {
                // 訪問回数最大を選択
                int best_n = -1;
                for (int i = 0; i < root_node->n_legal_moves; i++)
                {
                    int value_n = root_node->value_n[i];
                    if (best_n < value_n)
                    {
                        best_n = value_n;
                        best_idx = i;
                    }
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
