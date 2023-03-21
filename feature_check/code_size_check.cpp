/*
Codingame othelloでソースコードの長さ制限のチェック
これはテンプレートファイル。data_tableにダミーデータを挿入する

https://www.codingame.com/playgrounds/40701/help-center/languages-versions
Your program is compiled and run in a Linux environment on a 64bit multi-core architecture.
Time limit per turn should be specified on the game’s statement.
The code size for any game is limited to 100k characters.
The memory limit is 768 MB.
C++	g++ 11.2.0 mode C++17	‑lm, ‑lpthread, ‑ldl, ‑lcrypt
*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <sstream>

using namespace std;

const char* data_table = "";

string benchmark()
{
    int len = strlen(data_table);
    int sum = 0;
    for (int i = 0; i < len; i++)
    {
        sum += int(data_table[i]);
    }
    
    stringstream ss;
    ss << "strlen " << len << " table_sum " << sum;
    return ss.str();
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
