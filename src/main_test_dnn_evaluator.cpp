// DNNEvaluatorの結果が正しいかチェック
// DNNEvaluatorEmbedの結果を、DNNEvaluatorSocket(つまりTensorflow)の結果と比較する。

#include "common.hpp"

bool isclose(float expect, float actual)
{
    float atol = 1e-3;
    float rtol = 1e-2;

    return abs(expect - actual) <= atol + rtol * abs(expect);
}

bool compare_eval(const DNNEvaluatorResult &expect, const DNNEvaluatorResult &actual)
{
    for (size_t i = 0; i < (sizeof(expect.policy_logits) / sizeof(expect.policy_logits[0])); i++)
    {
        if (!isclose(expect.policy_logits[i], actual.policy_logits[i])) {
            cerr << "policy_logits index " << i << ": " << expect.policy_logits[i] << " != " << actual.policy_logits[i] << endl;
            return false;
        }
    }
    if (!isclose(expect.value_logit, actual.value_logit)) {
        cerr << "value_logit: " << expect.value_logit << " != " << actual.value_logit << endl;
        return false;
    }

    return true;
}

int main()
{
    Board board;
    board.set_hirate();

    int n_test = 0;
    shared_ptr<DNNEvaluator> evaluator_expect(new DNNEvaluatorSocket("127.0.0.1", 8099));
    shared_ptr<DNNEvaluator> evaluator_actual(new DNNEvaluatorEmbed());

    while (!board.is_gameover())
    {
        vector<Move> legal_moves;
        board.legal_moves(legal_moves);
        if (legal_moves.empty())
        {
            UndoInfo undo_info;
            board.do_move(MOVE_PASS, undo_info);
            continue;
        }

        auto expect = evaluator_expect->evaluate(board);
        auto actual = evaluator_actual->evaluate(board);
        if (!compare_eval(expect, actual))
        {
            cerr << "board:" << endl;
            cerr << board.pretty_print() << endl;
            break;
        }
        n_test++;

        Move move = legal_moves[0]; // 最初の合法手で進めていく

        UndoInfo undo_info;
        board.do_move(move, undo_info);
    }

    cerr << "Tested " << n_test << " cases" << endl;

    return 0;
}
