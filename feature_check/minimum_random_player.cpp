// minimum random player for codingame othello (929 bytes)

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;

int main()
{
    std::random_device seed_gen;
    mt19937 engine(seed_gen());

    int id;
    cin >> id; cin.ignore();
    int board_size;
    cin >> board_size; cin.ignore();

    while (1) {
        for (int i = 0; i < board_size; i++) {
            string line;
            cin >> line; cin.ignore();
        }
        int action_count;
        cin >> action_count; cin.ignore();

        vector<string> actions;
        for (int i = 0; i < action_count; i++) {
            string action;
            cin >> action; cin.ignore();
            actions.push_back(action);
        }

        uniform_int_distribution<> dist(0, actions.size() - 1);
        int action_idx = dist(engine);

        cout << actions[action_idx] << endl; // a-h1-8
    }
}
