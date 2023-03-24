#ifndef _DNN_EVALUATOR_EMBED_
#define _DNN_EVALUATOR_EMBED_

#include <memory>
#include "dnn_evaluator.hpp"
#include "base64.hpp"
#include "_dnn_weight.hpp"

class DNNEvaluatorEmbed : public DNNEvaluator
{
    class DNNWeight
    {
    public:
        float conv2d_8_kernel[8];
        float conv2d_8_bias[1];
        float dense_kernel[512];
        float dense_bias[1];
        float conv_bn_conv2d_kernel[216];
        float conv_bn_conv2d_bias[8];
        float conv_bn_1_conv2d_1_kernel[576];
        float conv_bn_1_conv2d_1_bias[8];
        float conv_bn_2_conv2d_2_kernel[576];
        float conv_bn_2_conv2d_2_bias[8];
        float conv_bn_3_conv2d_3_kernel[576];
        float conv_bn_3_conv2d_3_bias[8];
        float conv_bn_4_conv2d_4_kernel[576];
        float conv_bn_4_conv2d_4_bias[8];
        float conv_bn_5_conv2d_5_kernel[576];
        float conv_bn_5_conv2d_5_bias[8];
        float conv_bn_6_conv2d_6_kernel[576];
        float conv_bn_6_conv2d_6_bias[8];
        float conv_bn_7_conv2d_7_kernel[64];
        float conv_bn_7_conv2d_7_bias[8];
        float conv_bn_8_conv2d_9_kernel[64];
        float conv_bn_8_conv2d_9_bias[8];
    };
    FeatureExtractor extractor;
    DNNWeight weight;

    class Tensor
    {
    public:
        float *data;
        bool owndata = false;
        array<int, 4> shape; // 簡単のため常に4Dで扱う。(batch, h, w, c) or (batch, 1, 1, c) or (1, 1, 1, c)
        array<int, 4> strides;
        int size;
        Tensor(array<int, 4> shape, float *data = nullptr) : data(data), shape(shape)
        {
            size = 1;
            for (size_t i = shape.size() - 1; i < shape.size(); i--)
            {
                strides[i] = size;
                size *= shape[i];
            }
            if (!data)
            {
                owndata = true;
                this->data = new float[size];
            }
        }

        ~Tensor()
        {
            if (owndata)
            {
                delete[] data;
            }
        }

        float &v(int n, int h, int w, int c)
        {
            return data[n * strides[0] + h * strides[1] + w * strides[2] + c * strides[3]];
        }

        float &v(int n, int c)
        {
            return data[n * strides[0] + c * strides[3]];
        }

        float &v(int c)
        {
            return data[c * strides[3]];
        }
    };

    using PTensor = shared_ptr<Tensor>;

    PTensor tensor(array<int, 4> shape, float *data = nullptr)
    {
        return PTensor(new Tensor(shape, data));
    }

    PTensor conv2d(PTensor x, PTensor w, PTensor b, int pad, int stride)
    {
        // x: (n=1, h, w, in_c)
        // w: (kh, kw, in_c, out_c)
        // b: (1, 1, 1, out_c)
        // y: (n=1, out_h, out_w, out_c)
        int in_h = x->shape[1], in_w = x->shape[2], in_c = x->shape[3];
        int kh = w->shape[0], kw = w->shape[1], out_c = w->shape[3];
        int out_h = (in_h + 2 * pad - kh) / stride + 1;
        int out_w = (in_w + 2 * pad - kw) / stride + 1;
        PTensor y = tensor({1, out_h, out_w, out_c});
        for (int out_y = 0; out_y < out_h; out_y++)
            for (int out_x = 0; out_x < out_w; out_x++)
                for (int oc = 0; oc < out_c; oc++)
                {
                    float sum = b->v(oc);
                    for (int ic = 0; ic < in_c; ic++)
                        for (int ky = 0; ky < kh; ky++)
                            for (int kx = 0; kx < kw; kx++)
                            {
                                int in_y = out_y * stride - pad + ky;
                                int in_x = out_x * stride - pad + kx;
                                if (in_y < 0 || in_y >= in_h || in_x < 0 || in_x >= in_w)
                                {
                                    continue;
                                }
                                sum += x->v(0, in_y, in_x, ic) * w->v(ky, kx, ic, oc);
                            }

                    y->v(0, out_y, out_x, oc) = sum;
                }
        return y;
    }

    PTensor dense(PTensor x, PTensor w, PTensor b)
    {
        // x: (n=1, 1, 1, in_c)
        // w: (in_c, 1, 1, out_c)
        // b: (1, 1, 1, out_c)
        // y: (n=1, 1, 1, out_c)
        PTensor y = tensor({1, 1, 1, w->shape[3]});
        for (int n = 0; n < w->shape[3]; n++)
        {
            float sum = b->v(n);
            for (int k = 0; k < w->shape[0]; k++)
            {
                sum += x->v(k) * w->v(k, n);
            }
            y->v(n) = sum;
        }
        return y;
    }

    void relu_inplace(PTensor x)
    {
        for (int i = 0; i < x->size; i++)
        {
            x->data[i] = max(x->data[i], 0.0F);
        }
    }

    void flatten_inplace(PTensor x)
    {
        // flatten inplace
        // (n, h, w, c) => (n, 1, 1, c)
        int newc = x->shape[1] * x->shape[2] * x->shape[3];
        x->shape = {x->shape[0], 1, 1, newc};
        x->strides = {x->strides[0], 0, 0, 1};
    }

public:
    DNNEvaluatorEmbed() : extractor()
    {
        auto weight_raw = b64decode(dnn_weight_base64, sizeof(dnn_weight_base64) - 1);
        memcpy(&weight, &weight_raw[0], sizeof(weight));
    }

    ~DNNEvaluatorEmbed()
    {
    }

    DNNEvaluatorResult evaluate(const Board &board)
    {
        DNNInputFeature req = extractor.extract(board);
        PTensor h = tensor({1, BOARD_SIZE, BOARD_SIZE, 3}, req.board_repr);
        h = conv2d(h, tensor({3, 3, 3, 8}, weight.conv_bn_conv2d_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_conv2d_bias), 1, 1);
        relu_inplace(h);
        h = conv2d(h, tensor({3, 3, 8, 8}, weight.conv_bn_1_conv2d_1_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_1_conv2d_1_bias), 1, 1);
        relu_inplace(h);
        h = conv2d(h, tensor({3, 3, 8, 8}, weight.conv_bn_2_conv2d_2_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_2_conv2d_2_bias), 1, 1);
        relu_inplace(h);
        h = conv2d(h, tensor({3, 3, 8, 8}, weight.conv_bn_3_conv2d_3_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_3_conv2d_3_bias), 1, 1);
        relu_inplace(h);
        h = conv2d(h, tensor({3, 3, 8, 8}, weight.conv_bn_4_conv2d_4_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_4_conv2d_4_bias), 1, 1);
        relu_inplace(h);
        h = conv2d(h, tensor({3, 3, 8, 8}, weight.conv_bn_5_conv2d_5_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_5_conv2d_5_bias), 1, 1);
        relu_inplace(h);
        h = conv2d(h, tensor({3, 3, 8, 8}, weight.conv_bn_6_conv2d_6_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_6_conv2d_6_bias), 1, 1);
        relu_inplace(h);

        auto p = h, v = h;

        p = conv2d(p, tensor({1, 1, 8, 8}, weight.conv_bn_7_conv2d_7_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_7_conv2d_7_bias), 0, 1);
        relu_inplace(p);
        p = conv2d(p, tensor({1, 1, 8, 1}, weight.conv2d_8_kernel), tensor({1, 1, 1, 1}, weight.conv2d_8_bias), 0, 1);
    
        v = conv2d(v, tensor({1, 1, 8, 8}, weight.conv_bn_8_conv2d_9_kernel), tensor({1, 1, 1, 8}, weight.conv_bn_8_conv2d_9_bias), 0, 1);
        relu_inplace(v);
        flatten_inplace(v);
        v = dense(v, tensor({512, 1, 1, 1}, weight.dense_kernel), tensor({1, 1, 1, 1}, weight.dense_bias));

        DNNEvaluatorResult res;
        memcpy(res.policy_logits, p->data, sizeof(res.policy_logits));
        memcpy(&res.value_logit, v->data, sizeof(res.value_logit));
        return res;
    }
};
#endif
