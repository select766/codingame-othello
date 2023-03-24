#ifndef _MCTS_UTIL_
#define _MCTS_UTIL_

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

    void clear()
    {
        next_idx = 1;
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

    TreeNode *at(int index) const
    {
        if (index <= 0 || index >= int(next_idx))
        {
            cerr << "TreeTable index out of bound" << endl;
            exit(1);
        }
        return &nodes[index];
    }

    int get_index(const TreeNode *node) const
    {
        // この関数は使いたくない
        return node - nodes;
    }
};

namespace MCTSBase
{
    
    TreeNode *make_node(const Board &b, TreeTable* tree_table)
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

    int select_edge(const TreeNode *node, float c_puct)
    {
        int n_sum = 0;
        for (int i = 0; i < node->n_legal_moves; i++)
        {
            n_sum += node->value_n[i];
        }
        float n_sum_sqrt = sqrt(static_cast<float>(n_sum)) + 0.001;
        float best_score = -1000.0F;
        int best_edge = 0;
        for (int i = 0; i < node->n_legal_moves; i++)
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
}
#endif
