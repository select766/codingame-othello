#ifndef _DNN_EVALUATOR_TVM_
#define _DNN_EVALUATOR_TVM_

#include <memory>
#include "dnn_evaluator.hpp"

#include <inttypes.h>
#include <time.h>
#include <tvm/runtime/c_runtime_api.h>
#include <tvm/runtime/crt/logging.h>
#include <tvm/runtime/crt/page_allocator.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <cstring>

#include <tvm/runtime/crt/aot_executor_module.h>

extern "C"
{
    ssize_t MicroTVMWriteFunc(void *context, const uint8_t *data, size_t num_bytes)
    {
        return (ssize_t)(num_bytes);
    }

    size_t TVMPlatformFormatMessage(char *out_buf, size_t out_buf_size_bytes, const char *fmt,
                                    va_list args)
    {
        return vsnprintf(out_buf, out_buf_size_bytes, fmt, args);
    }

    void TVMPlatformAbort(tvm_crt_error_t error_code)
    {
        std::cerr << "TVMPlatformAbort: " << error_code << std::endl;
        throw "Aborted";
    }

    MemoryManagerInterface *memory_manager;

    tvm_crt_error_t TVMPlatformMemoryAllocate(size_t num_bytes, DLDevice dev, void **out_ptr)
    {
        auto r = memory_manager->Allocate(memory_manager, num_bytes, dev, out_ptr);
        // fprintf(stderr, "alloc %ld %p\n", num_bytes, *out_ptr);
        return r;
    }

    tvm_crt_error_t TVMPlatformMemoryFree(void *ptr, DLDevice dev)
    {
        // fprintf(stderr, "free %p\n", ptr);
        return memory_manager->Free(memory_manager, ptr, dev);
    }

    chrono::steady_clock::time_point g_microtvm_start_time;
    int g_microtvm_timer_running = 0;

    tvm_crt_error_t TVMPlatformTimerStart()
    {
        if (g_microtvm_timer_running)
        {
            std::cerr << "timer already running" << std::endl;
            return kTvmErrorPlatformTimerBadState;
        }
        g_microtvm_start_time = std::chrono::steady_clock::now();
        g_microtvm_timer_running = 1;
        return kTvmErrorNoError;
    }

    tvm_crt_error_t TVMPlatformTimerStop(double *elapsed_time_seconds)
    {
        if (!g_microtvm_timer_running)
        {
            std::cerr << "timer not running" << std::endl;
            return kTvmErrorPlatformTimerBadState;
        }
        auto microtvm_stop_time = std::chrono::steady_clock::now();
        std::chrono::microseconds time_span = std::chrono::duration_cast<std::chrono::microseconds>(
            microtvm_stop_time - g_microtvm_start_time);
        *elapsed_time_seconds = static_cast<double>(time_span.count()) / 1e6;
        g_microtvm_timer_running = 0;
        return kTvmErrorNoError;
    }

    static_assert(RAND_MAX >= (1 << 8), "RAND_MAX is smaller than acceptable");
    unsigned int random_seed = 0;
    tvm_crt_error_t TVMPlatformGenerateRandom(uint8_t *buffer, size_t num_bytes)
    {
        if (random_seed == 0)
        {
            random_seed = (unsigned int)time(NULL);
        }
        for (size_t i = 0; i < num_bytes; ++i)
        {
            int random = rand_r(&random_seed);
            buffer[i] = (uint8_t)random;
        }

        return kTvmErrorNoError;
    }
}

extern "C" int32_t tvmgen_default_run(TVMValue *args, int *type_code, int num_args, TVMValue *out_value, int *out_type_code, void *resource_handle);

void TVMLogf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

class DNNEvaluatorTVM : public DNNEvaluator
{
    FeatureExtractor extractor;
    uint8_t memory[16777216];

    DLTensor input_tensor, output_policy_tensor, output_value_tensor;
    TVMArrayHandle input_handle, output_policy_handle, output_value_handle;

public:
    DNNEvaluatorTVM() : extractor()
    {
        input_handle = &input_tensor;
        output_policy_handle = &output_policy_tensor;
        output_value_handle = &output_value_tensor;
        int status =
            PageMemoryManagerCreate(&memory_manager, memory, sizeof(memory), 8 /* page_size_log2 */);
        if (status != 0)
        {
            cerr << "PageMemoryManagerCreate" << endl;
            exit(1);
        }
        tvm_index_t input_shape[] = {1, 8, 8, 3}, output_policy_shape[] = {1, 64}, output_value_shape[] = {1, 1};
        int dtype_code = kDLFloat;
        int dtype_bits = 32;
        int dtype_lanes = 1;
        int device_type = kDLCPU;
        int device_id = 0;
        if (TVMArrayAlloc(input_shape, (int)(sizeof(input_shape) / sizeof(input_shape[0])), dtype_code, dtype_bits,
                          dtype_lanes, device_type, device_id, &input_handle) != 0)
        {
            cerr << "TVMArrayAlloc failed" << endl;
            exit(1);
        }
        if (TVMArrayAlloc(output_policy_shape, (int)(sizeof(output_policy_shape) / sizeof(output_policy_shape[0])), dtype_code, dtype_bits,
                          dtype_lanes, device_type, device_id, &output_policy_handle) != 0)
        {
            cerr << "TVMArrayAlloc failed" << endl;
            exit(1);
        }
        if (TVMArrayAlloc(output_value_shape, (int)(sizeof(output_value_shape) / sizeof(output_value_shape[0])), dtype_code, dtype_bits,
                          dtype_lanes, device_type, device_id, &output_value_handle) != 0)
        {
            cerr << "TVMArrayAlloc failed" << endl;
            exit(1);
        }
    }

    ~DNNEvaluatorTVM()
    {
        // TVMArrayFreeは、DLTensorへのポインタを、サイズの違うTVMNDArrayへのポインタとみなして誤操作するバグがあるため行わない
    }

    DNNEvaluatorResult evaluate(const Board &board)
    {
        DNNInputFeature req = extractor.extract(board);
        memcpy(input_handle->data, req.board_repr, sizeof(req.board_repr));

        TVMValue model_args[3];
        model_args[0].v_handle = input_handle;
        model_args[1].v_handle = output_policy_handle;
        model_args[2].v_handle = output_value_handle;
        int arg_type_ids[3] = {kTVMArgFloat, kTVMArgFloat, kTVMArgFloat};
        if (tvmgen_default_run(model_args, arg_type_ids, 3, nullptr, nullptr, nullptr) != 0)
        {
            cerr << "tvmgen_default_run failed" << endl;
            exit(1);
        }
        DNNEvaluatorResult res;
        memcpy(res.policy_logits, output_policy_handle->data, sizeof(res.policy_logits));
        memcpy(&res.value_logit, output_value_handle->data, sizeof(res.value_logit));
        return res;
    }
};
#endif
