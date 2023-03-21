#ifndef _SEARCH_POLICY_
#define _SEARCH_POLICY_
#include "search_base.hpp"
#include "dnn_evaluator.hpp"
#include <memory>

// DNNのpolicyに従って指すAI
class SearchPolicy : public SearchBase
{
    shared_ptr<DNNEvaluator> dnn_evaluator;

public:
    SearchPolicy(shared_ptr<DNNEvaluator> dnn_evaluator) : dnn_evaluator(dnn_evaluator)
    {
    }

    string name()
    {
        return "Policy";
    }

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
            auto eval_result = dnn_evaluator->evaluate(board);
            Move bestmove = 0;
            float bestlogit = -1000.0F;
            for (auto move : move_list)
            {
                float policy_logit = eval_result.policy_logits[move];
                if (bestlogit < policy_logit)
                {
                    bestlogit = policy_logit;
                    bestmove = move;
                }
            }
            stringstream ss;
            ss << "policy_logit " << bestlogit << " value_logit " << eval_result.value_logit;
            msg = ss.str();

            return bestmove;
        }
    }
};

#endif
