/*
Codingame othelloでponder(相手番の間の思考)が利用できるかのチェック。
検証日: 2023-03-21
結論: 実行は可能なものの、速度が非常に低い。

出力メッセージ例
d1 MSG normal_calc_ms 120 done_count 166 result_sum 498 ponder_time_ms 401 done_count 22 result_sum 66
通常思考では120msのあいだに、重い処理(grid_search)を166回実行できている。
ponder中では401msのあいだに、22回しか実行できていない。
相手の思考中は、ほぼ計算ができないと思われる。
*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <atomic>
#include <thread>
#include <sstream>

using namespace std;

// configs
const int time_limit_ms = 120;

#define ACKLEY_DIM 2

float ackley(float x[ACKLEY_DIM])
{
    int n = ACKLEY_DIM;
    float sqsum = 0.0F;
    float cossum = 0.0F;
    for (int i = 0; i < n; i++)
    {
        sqsum += x[i] * x[i];
        cossum += cos(2.0F * float(M_PI) * x[i]);
    }
    return 20.0F - 20.0F * exp(-0.2F * sqrt(sqsum / n)) + exp(1.0F) - exp(cossum / n);
}

float grid_search(int size)
{
    float minval = 100000.0F;
    float ofs = -32.768F;
    float step = (32.768F * 2) / (size - 1);
    for (int i0 = 0; i0 < size; i0++)
        for (int i1 = 0; i1 < size; i1++)
        {
            float x[ACKLEY_DIM];
            x[0] = ofs + step * i0;
            x[1] = ofs + step * i1;
            float v = ackley(x);
            if (minval > v)
            {
                minval = v;
            }
        }
    return minval;
}

atomic_int done_count; // grid_searchを実行できた回数
atomic_int result_sum; // 最適化で消えないように合計を計算して表示
atomic_llong ponder_time_ms;
atomic_bool stop_ponder_flag;

void dummy_calc()
{
    auto search_start_time = chrono::system_clock::now();
    while (!stop_ponder_flag)
    {
        result_sum += int(grid_search(100));
        done_count++;
    }
    auto search_end_time = chrono::system_clock::now();
    ponder_time_ms = chrono::duration_cast<chrono::milliseconds>(search_end_time - search_start_time).count();
}

thread* ponder_thread = nullptr;
void start_ponder()
{
    stop_ponder_flag = false;
    result_sum = 0;
    done_count = 0;
    ponder_thread = new thread(dummy_calc);
}

string stop_ponder()
{
    if (!ponder_thread) {
        return "";
    }
    stop_ponder_flag = true;
    ponder_thread->join();
    delete ponder_thread;
    ponder_thread = nullptr;

    stringstream ss;
    ss << "ponder_time_ms " << ponder_time_ms << " done_count " << done_count << " result_sum " << result_sum;
    return ss.str();
}


string normal_calc()
{
    result_sum = 0;
    done_count = 0;
    auto search_start_time = chrono::system_clock::now();
    auto time_to_stop_search = search_start_time + chrono::milliseconds(time_limit_ms);
    while (chrono::system_clock::now() < time_to_stop_search)
    {
        result_sum += int(grid_search(100));
        done_count++;
    }
    
    auto normal_time_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - search_start_time).count();
    
    stringstream ss;
    ss << "normal_calc_ms " << normal_time_ms << " done_count " << done_count << " result_sum " << result_sum;
    return ss.str();
}

int main()
{
    std::random_device seed_gen;
    mt19937 engine(seed_gen());

    int id;
    cin >> id;
    cin.ignore();
    int board_size;
    cin >> board_size;
    cin.ignore();

    while (1)
    {
        for (int i = 0; i < board_size; i++)
        {
            string line;
            cin >> line;
            cin.ignore();
        }
        int action_count;
        cin >> action_count;
        cin.ignore();

        vector<string> actions;
        for (int i = 0; i < action_count; i++)
        {
            string action;
            cin >> action;
            cin.ignore();
            actions.push_back(action);
        }

        string ponder_msg = stop_ponder();
        string normal_msg = normal_calc();

        uniform_int_distribution<> dist(0, actions.size() - 1);
        int action_idx = dist(engine);

        cout << actions[action_idx] << " MSG " << normal_msg << " " << ponder_msg << endl;

        start_ponder();
    }
}
