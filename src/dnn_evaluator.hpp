#ifndef _DNN_EVALUATOR_
#define _DNN_EVALUATOR_

#include "board.hpp"

class DNNEvaluatorResult
{
public:
    float policy_logits[BOARD_AREA];
    float value_logit;
};

class DNNInputFeature
{
public:
    float board_repr[BOARD_AREA * 3];
};

class FeatureExtractor
{
public:
    DNNInputFeature extract(const Board &board)
    {
        DNNInputFeature req;
        memset(&req, 0, sizeof(req));
        // board_repr: [pos(y,x),3] (NHWC)
        const int n_ch = 3;
        for (int i = 0; i < N_PLAYER; i++)
        {
            int turn = i == 0 ? board.turn() : 1 - board.turn();
            BoardPlane bb = board.plane(turn);
            for (int pos = 0; pos < BOARD_AREA; pos++)
            {
                if (bb & position_plane(pos))
                {
                    req.board_repr[pos * n_ch + i] = 1.0F;
                }
            }
        }
        // fill 1
        for (int pos = 0; pos < BOARD_AREA; pos++)
        {
            req.board_repr[pos * n_ch + 2] = 1.0F;
        }

        return req;
    }
};

class DNNEvaluator
{
public:
    virtual DNNEvaluatorResult evaluate(const Board &board) = 0;
};
#endif
