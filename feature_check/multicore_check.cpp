/*
Codingame othelloでマルチコアが利用できるかのチェック。
検証日: 2023-03-21
結論: スレッドを立ててもエラーにはならないが、シングルコアしか使用できず高速化にはならない。
n_threads 定数を書き換えることで計算スレッド数を変更できる。

重い関数(grid_search)を実行できた回数を計測する。
以下のように、ランダムな指し手とともに結果を表示する。
a6 MSG threads 1 done_count 162 result_sum 486
done_countの後ろの数字(162)が実行回数。

結果(実行ごとに変動する):
n_threads = 1 (シングルスレッド)のとき162
n_threads = 2のとき161
スレッドを立ててもエラーにはならないが、シングルコアしか使用できず高速化にはならない。

ローカルでの結果(Intel Core i7-10700F, 物理8コア、論理16コア):
g++ multicore_check.cpp -O3 -DENV_LOCAL
n_threads = 1 => 702
n_threads = 2 => 1208
n_threads = 8 => 4605
n_threads = 16 => 7150
n_threads = 32 => 8124
マルチコアCPUで実行すれば、スレッド数の増加によって出力が大きくなることが観測できる。
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
const int n_threads = 32;

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

void dummy_calc()
{
    auto search_start_time = chrono::system_clock::now();
    auto time_to_stop_search = search_start_time + chrono::milliseconds(time_limit_ms);
    while (chrono::system_clock::now() < time_to_stop_search)
    {
        result_sum += int(grid_search(100));
        done_count++;
    }
}

string multicore_check()
{
    done_count = 0;
    result_sum = 0.0F;

    vector<thread*> threads;
    for (int i = 0; i < n_threads; i++)
    {
        threads.push_back(new thread(dummy_calc));
    }

    for (int i = 0; i < n_threads; i++)
    {
        threads[i]->join();
        delete threads[i];
    }

    stringstream ss;
    ss << "threads " << n_threads << " done_count " << done_count << " result_sum " << result_sum;
    return ss.str();
}

string benchmark()
{
    return multicore_check();
}

#ifdef ENV_LOCAL
int main()
{
    cout << benchmark() << endl;
}
#else
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

        uniform_int_distribution<> dist(0, actions.size() - 1);
        int action_idx = dist(engine);

        string msg = benchmark();

        cout << actions[action_idx] << " MSG " << msg << endl;
    }
}
#endif
