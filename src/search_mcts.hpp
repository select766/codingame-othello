#ifndef _SEARCH_MCTS_
#define _SEARCH_MCTS_
#include <memory>
#include <cassert>
#include "search_base.hpp"

// 置換表のノード
class TreeNode
{
    // 552バイト
public:
    float score;                        // 静的評価値（ゲーム終了なら手番側の勝ちで+1、負けで-1、引き分けで0）
    int n_legal_moves;                  // 合法手の数。パスも1個。0なら末端ノード。
    uint8_t move_list[MAX_LEGAL_MOVES]; // 子ノードへの指し手
    int children[MAX_LEGAL_MOVES];      // 置換表上の子ノードのインデックス(0はnullptrに相当)
    int value_n[MAX_LEGAL_MOVES];       // 子ノードの訪問回数
    float value_w[MAX_LEGAL_MOVES];     // 子ノードからバックアップされたスコア合計
    float value_p[MAX_LEGAL_MOVES];     // 指し手の事前確率

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }

    bool terminal() const
    {
        return n_legal_moves == 0;
    }
};

// 置換表
class TreeTable
{
    size_t next_idx; // index=0は、nullptr扱いとして使用しない
    size_t _size;
    TreeNode *nodes;

public:
    TreeTable(size_t size) : _size(size), next_idx(1)
    {
        // 試算では、固定長確保、ゲーム終了まで解放せずに進めてメモリが足りる。
        nodes = new TreeNode[size];
    }

    ~TreeTable()
    {
        delete[] nodes;
    }

    // 新しい要素を確保する。内容は初期化されないため、必要に応じてTreeNode.clear()を用いる。
    TreeNode *alloc()
    {
        if (next_idx >= _size)
        {
            cerr << "TreeTable out of memory" << endl;
            exit(1);
        }
        return &nodes[next_idx++];
    }

    TreeNode *at(int index)
    {
        if (index <= 0 || index >= int(next_idx))
        {
            cerr << "TreeTable index out of bound" << endl;
            exit(1);
        }
        return &nodes[index];
    }

    int get_index(TreeNode *node)
    {
        // この関数は使いたくない
        return (node - nodes) / sizeof(TreeNode);
    }
};

class SearchPartialResult
{
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

// MCTS
class SearchMCTS : public SearchBase
{
    int playout_limit;
    float c_puct;
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

public:
    SearchMCTS(int playout_limit, size_t table_size, float c_puct)
        : playout_limit(playout_limit), tree_table(new TreeTable(table_size)), c_puct(c_puct), next_task(START_SEARCH), root_node(nullptr)
    {
    }

    string name()
    {
        return "MCTS";
    }

    // 対局用
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
            // TODO
        }
        return MOVE_PASS; // TODO
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
        root_node = make_node(b);
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

    TreeNode *make_node(const Board &b)
    {
        TreeNode *tn = tree_table->alloc();
        tn->clear();
        bool terminal = false;
        float score = 0.0F;
        if (b.is_gameover())
        {
            int winner = b.winner();
            if (winner == DRAW)
            {
                score = 0.0F;
            }
            else if (winner == b.turn())
            {
                score = 1.0F;
            }
            else
            {
                score = -1.0F;
            }
            terminal = true;
        }

        tn->score = score;
        if (!terminal)
        {
            vector<Move> legal_moves;
            b.legal_moves(legal_moves, true);
            tn->n_legal_moves = legal_moves.size();
            for (int i = 0; i < legal_moves.size(); i++)
            {
                tn->move_list[i] = static_cast<uint8_t>(legal_moves[i]);
            }
            // tn->childrenは0初期化されているので、子ノードが存在していない状態を表す。エッジの情報(value_n)なども0になる。
        }
        else
        {
            tn->n_legal_moves = 0;
        }
        return tn;
    }

    shared_ptr<SearchPartialResult> search_tree()
    {
        if (playout_count >= playout_limit)
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

        int edge = select_edge(node);
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
            TreeNode *child_node = make_node(b);
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

    void backup_path(vector<pair<TreeNode *, int>> &path, float leaf_score)
    {
        float score = leaf_score;
        for (int i = int(path.size()) - 1; i >= 0; i--)
        {
            score = -score;
            path[i].first->value_w[path[i].second] += score;
        }
    }

    int select_edge(TreeNode *node)
    {
        int n_sum = 0;
        for (int i = 0; node->n_legal_moves; i++)
        {
            n_sum += node->value_n[i];
        }
        float n_sum_sqrt = sqrt(static_cast<float>(n_sum)) + 0.001;
        float best_score = -1000.0F;
        int best_edge = 0;
        for (int i = 0; node->n_legal_moves; i++)
        {
            float u = node->value_p[i] / static_cast<float>(node->value_n[i] + 1) * n_sum_sqrt * c_puct;
            // 未訪問ノードのスコアは現局面と同じと仮定
            float q = node->value_n[i] == 0 ? node->score : (node->value_w[i] / static_cast<float>(node->value_n[i]));
            float s = u + q;
            if (best_score < s)
            {
                best_score = s;
                best_edge = i;
            }
        }

        return best_edge;
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
