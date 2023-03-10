#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
    std::random_device seed_gen;
    mt19937 engine(seed_gen());

    int id; // id of your player.
    cin >> id; cin.ignore();
    int board_size;
    cin >> board_size; cin.ignore();

    // game loop
    while (1) {
        for (int i = 0; i < board_size; i++) {
            string line; // rows from top to bottom (viewer perspective).
            cin >> line; cin.ignore();
        }
        int action_count; // number of legal actions for this turn.
        cin >> action_count; cin.ignore();

        vector<string> actions;
        for (int i = 0; i < action_count; i++) {
            string action; // the action
            cin >> action; cin.ignore();
            actions.push_back(action);
        }

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        uniform_int_distribution<> dist(0, actions.size() - 1);

        int action_idx = dist(engine);

        cout << actions[action_idx] << endl; // a-h1-8
    }
}
