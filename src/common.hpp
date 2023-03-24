#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt")
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <sstream>
#include <chrono>
#include <memory>
#include "base64.hpp"
#include "board.hpp"
#include "dnn_evaluator_embed.hpp"
#include "dnn_evaluator_socket.hpp"
#include "search_alpha_beta_constant_depth.hpp"
#include "search_alpha_beta_iterative.hpp"
#include "search_base.hpp"
#include "search_greedy.hpp"
#include "search_mcts.hpp"
#include "search_mcts_train.hpp"
#include "search_policy.hpp"
#include "search_random.hpp"
