#include "common.hpp"
#include <fstream>

// 24bytes
struct MoveRecord
{
    BoardPlane planes[N_PLAYER];
    uint8_t turn; // BLACK / WHITE
    uint8_t move; // 選んだ指し手
    int8_t game_result; // 終局時の石の数の差。手番側が多い(勝ち)で正、負けで負、引き分けは0
    uint8_t n_legal_moves; // 合法手の数(0はパス)
    uint8_t pad[4]; // BoardPlaneのアライメント
};

void run_one_game(SearchBase *ai, ofstream &fout)
{
    vector<MoveRecord> records;
    Board board;
    board.set_hirate();
    ai->newgame();

    while (!board.is_gameover())
    {
        ai->board.set(board);
        string msg;
        Move move = ai->search(msg);

        // 合法手が1個の場合も、ゲームの流れを記録するために出力
        // シャッフルを行うツールで削除する
        BoardPlane lm;
        board.legal_moves_bb(lm);
        auto n_legal_moves = __builtin_popcountll(lm);
        MoveRecord record;
        record.move = static_cast<decltype(record.move)>(move);
        record.planes[0] = board.plane(0);
        record.planes[1] = board.plane(1);
        record.turn = static_cast<decltype(record.turn)>(board.turn());
        record.n_legal_moves = static_cast<decltype(record.turn)>(n_legal_moves);
        memset(record.pad, 0, sizeof(record.pad));

        records.push_back(record);

        UndoInfo undo_info;
        board.do_move(move, undo_info);
    }

    int8_t stone_diff_black = static_cast<int8_t>(board.piece_num(BLACK) - board.piece_num(WHITE));
    for (auto &record : records)
    {
        record.game_result = record.turn == BLACK ? stone_diff_black : -stone_diff_black;
    }

    fout.write((char*)&records[0], records.size() * sizeof(MoveRecord));
}

int main(int argc, const char* argv[])
{
    if (argc < 3)
    {
        cerr << "usage: " << argv[0] << " outfile n_game" << endl;
    }
    const char* outfile = argv[1];
    int n_game = atoi(argv[2]);

    ofstream fout;
    fout.open(outfile, ios::out|ios::binary|ios::trunc);

    if (!fout)
    {
        cerr << "failed to open " << outfile << endl;
        return 1;
    }

    SearchBase *ai = new SearchAlphaBetaConstantDepth(5, 2.0);
    for (int i = 0; i < n_game; i++)
    {
        run_one_game(ai, fout);
        if (!fout)
        {
            cerr << "failed to write to " << outfile << endl;
            return 1;
        }
        cerr << "\r" << (i * 100 / n_game) << "%";
    }

    fout.close();

    if (!fout)
    {
        cerr << "failed to close " << outfile << endl;
        return 1;
    }

    cerr << "done" << endl;
    return 0;
}
