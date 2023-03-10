#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
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
        for (int i = 0; i < action_count; i++) {
            string action; // the action
            cin >> action; cin.ignore();
        }

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;

        cout << "f4" << endl; // a-h1-8
    }
}
