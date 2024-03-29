#include "common.hpp"
#include <fstream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

// 24bytes
struct MoveRecord
{
    BoardPlane planes[N_PLAYER];
    uint8_t turn;          // BLACK / WHITE
    uint8_t move;          // 選んだ指し手
    int8_t game_result;    // 終局時の石の数の差。手番側が多い(勝ち)で正、負けで負、引き分けは0
    uint8_t n_legal_moves; // 合法手の数(0はパス)
    uint8_t pad[4];        // BoardPlaneのアライメント
};

class PlayoutBuffer
{
public:
    float *board_repr;          // Playoutが評価を求めたい盤面表現をこのアドレスに書き込む
    const float *policy_logits; // 前回評価を求められた局面の評価結果をPlayoutに渡す
    const float *value_logit;   // 前回評価を求められた局面の評価結果をPlayoutに渡す
};

// DNN評価結果のキャッシュ機構。初手付近など並行するプレイ間での共有や、局面を進めた後に同じノードを評価する場面で高速化する。
class EvalCache
{
    class CacheEntry
    {
    public:
        Board board;
        SearchMCTSTrain::EvalResult eval_result;

        CacheEntry()
        {
            // 空のボードは、実現する盤面とマッチしない
            memset(&board, 0, sizeof(board));
        }
    };

    size_t size;
    size_t hash_mask;
    vector<CacheEntry> cache;
    int cache_hit, total_get;

public:
    EvalCache(size_t size) : size(size), hash_mask(size - 1), cache(size), cache_hit(0), total_get(0)
    {
        if (size & (size - 1))
        {
            throw runtime_error("EvalCache: size must be power of 2");
        }
    }

    SearchMCTSTrain::EvalResult *get(const Board &board)
    {
        total_get++;
        // if (total_get % 1024 == 0)
        // {
        //     cerr << "cache hit rate: " << (cache_hit * 100 / total_get) << endl;
        // }
        size_t key = board.hash() & hash_mask;
        CacheEntry *entry = &cache[key];
        if (entry->board == board)
        {
            cache_hit++;
            return &entry->eval_result;
        }
        return nullptr;
    }

    void put(const Board &board, const SearchMCTSTrain::EvalResult *eval_result)
    {
        size_t key = board.hash() & hash_mask;
        CacheEntry *entry = &cache[key];
        entry->board = board;
        memcpy(&entry->eval_result, eval_result, sizeof(*eval_result));
    }
};

class SinglePlayout
{
    Board board;
    Board evaluating_board;
    bool has_evaluating_board;
    FeatureExtractor extractor;
    vector<MoveRecord> records;
    shared_ptr<EvalCache> eval_cache;
    int _games_completed;

public:
    shared_ptr<ofstream> fout;
    SearchMCTSTrain engine;

    SinglePlayout(shared_ptr<ofstream> fout, SearchMCTSTrain::SearchMCTSConfig mcts_config, shared_ptr<EvalCache> eval_cache) : fout(fout), engine(mcts_config), extractor(), _games_completed(0), eval_cache(eval_cache), has_evaluating_board(false)
    {
        board.set_hirate();
        engine.board.set(board);
    }

    int games_completed() const
    {
        return _games_completed;
    }

    void proceed(PlayoutBuffer &playout_buffer)
    {
        SearchMCTSTrain::EvalResult eval_result;
        if (playout_buffer.policy_logits)
        {
            memcpy(eval_result.policy_logits, playout_buffer.policy_logits, sizeof(eval_result.policy_logits));
        }
        if (playout_buffer.value_logit)
        {
            memcpy(&eval_result.value_logit, playout_buffer.value_logit, sizeof(eval_result.value_logit));
        }
        if (has_evaluating_board)
        {
            eval_cache->put(evaluating_board, &eval_result);
            has_evaluating_board = false;
        }
        while (true)
        {
            auto search_partial_result = engine.search_partial(&eval_result);
            auto result_move = dynamic_pointer_cast<SearchMCTSTrain::SearchPartialResultMove>(search_partial_result);
            if (result_move)
            {
                // 指し手を進める
                proceed_game(result_move->move);
            }
            auto result_eval = dynamic_pointer_cast<SearchMCTSTrain::SearchPartialResultEvalRequest>(search_partial_result);
            if (result_eval)
            {
                // 評価が必要
                auto cached_result = eval_cache->get(result_eval->board);
                if (cached_result)
                {
                    memcpy(eval_result.policy_logits, cached_result->policy_logits, sizeof(eval_result.policy_logits));
                    memcpy(&eval_result.value_logit, &cached_result->value_logit, sizeof(eval_result.value_logit));
                }
                else
                {
                    DNNInputFeature feat = extractor.extract(result_eval->board);
                    memcpy(playout_buffer.board_repr, feat.board_repr, sizeof(feat.board_repr));
                    evaluating_board = result_eval->board;
                    has_evaluating_board = true;
                    return;
                }
            }
        }
    }

private:
    void do_move_with_record(Move move)
    {
        // boardを進めるとともに指し手を記録
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

    void flush_record_with_game_result()
    {
        // gameoverの時に呼び出す。recordsにゲームの結果を書きこんだうえでファイルに出力する。

        int8_t stone_diff_black = static_cast<int8_t>(board.piece_num(BLACK) - board.piece_num(WHITE));
        for (auto &record : records)
        {
            record.game_result = record.turn == BLACK ? stone_diff_black : -stone_diff_black;
        }

        fout->write((char *)&records[0], records.size() * sizeof(MoveRecord));
        records.clear();
        _games_completed++;
    }

    void proceed_game(Move move)
    {
        // 指定された指し手でゲームを進め、次に指し手選択が必要な状態まで進行する。最新の局面をengineにセットする。
        do_move_with_record(move);

        while (true)
        {
            if (board.is_gameover())
            {
                flush_record_with_game_result();
                board.set_hirate();
                engine.newgame();
                engine.board.set(board);
            }

            vector<Move> move_list;
            board.legal_moves(move_list);
            if (move_list.empty())
            {
                do_move_with_record(MOVE_PASS);
            }
            else if (move_list.size() == 1)
            {
                do_move_with_record(move_list[0]);
            }
            else
            {
                break;
            }
        }

        engine.board.set(board);
        return;
    }
};

class ParallelPlayout
{
public:
    shared_ptr<ofstream> fout;
    vector<unique_ptr<SinglePlayout>> single_playouts;
    shared_ptr<EvalCache> eval_cache;
    int parallel;

    ParallelPlayout(shared_ptr<ofstream> fout, SearchMCTSTrain::SearchMCTSConfig mcts_config, int parallel) : fout(fout), parallel(parallel), eval_cache(new EvalCache(1024 * 1024))
    {
        for (int i = 0; i < parallel; i++)
        {
            single_playouts.push_back(unique_ptr<SinglePlayout>(new SinglePlayout(fout, mcts_config, eval_cache)));
        }
    }

    int games_completed() const
    {
        int sum = 0;
        for (int i = 0; i < parallel; i++)
        {
            sum += single_playouts[i]->games_completed();
        }
        return sum;
    }

    void proceed(float *batch_board_repr, const float *batch_policy_logits, const float *batch_value_logit)
    {
        for (int i = 0; i < parallel; i++)
        {
            PlayoutBuffer pb;
            pb.board_repr = (float *)((char *)batch_board_repr + sizeof(DNNInputFeature::board_repr) * i);
            pb.policy_logits = (float *)((char *)batch_policy_logits + sizeof(SearchMCTSTrain::EvalResult::policy_logits) * i);
            pb.value_logit = (float *)((char *)batch_value_logit + sizeof(SearchMCTSTrain::EvalResult::value_logit) * i);
            single_playouts[i]->proceed(pb);
        }
    }
};

shared_ptr<ParallelPlayout> parallel_playout;

int init_playout(const string &record_path, int parallel, int playout_limit)
{
    shared_ptr<ofstream> fout(new ofstream());
    fout->open(record_path, ios::out | ios::binary | ios::trunc);

    if (!fout)
    {
        cerr << "failed to open " << record_path << endl;
        return 1;
    }

    SearchMCTSTrain::SearchMCTSConfig mcts_config;
    mcts_config.playout_limit = playout_limit;
    mcts_config.table_size = mcts_config.playout_limit + 16; // 1手ごとに探索木を初期化するので、プレイアウト数＋マージンでOK
    mcts_config.c_puct = 1.0;
    mcts_config.root_noise_dirichret_alpha = 1.6; // AlphaZeroで将棋の場合0.15。平均合法手数に反比例。将棋は80、オセロは（手元の実測で）7.5。
    mcts_config.root_noise_epsilon = 0.25;
    mcts_config.select_move_proportional_until_move = BOARD_AREA; // 常に訪問回数に比例
    mcts_config.mate_1ply = true;

    parallel_playout = shared_ptr<ParallelPlayout>(new ParallelPlayout(fout, mcts_config, parallel));

    return 0;
}

void proceed_playout(py::array_t<float> batch_board_repr, py::array_t<float> batch_policy_logits, py::array_t<float> batch_value_logit)
{
    auto bbr = batch_board_repr.mutable_unchecked<4>();
    auto bpl = batch_policy_logits.unchecked<2>();
    auto bvl = batch_value_logit.unchecked<2>();
    parallel_playout->proceed(bbr.mutable_data(0, 0, 0, 0), bpl.data(0, 0), bvl.data(0, 0));
}

void end_playout()
{
    // close output file
    parallel_playout = nullptr;
}

int games_completed()
{
    return parallel_playout->games_completed();
}

PYBIND11_MODULE(othello_train_cpp, m)
{
    m.doc() = "othello_train c++ module";

    m.def("init_playout", &init_playout, "Initializes playout");
    m.def("proceed_playout", &proceed_playout, "Proceeds playout");
    m.def("end_playout", &end_playout, "Ends playout (close output file)");
    m.def("games_completed", &games_completed, "Gets the number of completed games");

    m.attr("BOARD_SIZE") = py::int_(BOARD_SIZE);
    m.attr("BOARD_AREA") = py::int_(BOARD_AREA);
    m.attr("MOVE_PASS") = py::int_(MOVE_PASS);
    m.attr("N_PLAYER") = py::int_(N_PLAYER);
    m.attr("BLACK") = py::int_(BLACK);
    m.attr("WHITE") = py::int_(WHITE);
    m.attr("DRAW") = py::int_(DRAW);

    m.def("move_is_pass", move_is_pass);
    m.def("move_to_str", move_to_str);
    m.def("move_from_str", move_from_str);

    py::class_<UndoInfo>(m, "UndoInfo");
    py::class_<Board>(m, "Board")
        .def(py::init<>())
        .def("turn", &Board::turn)
        .def("plane", &Board::plane)
        .def("set", &Board::set_pybind11)
        .def("set_hirate", &Board::set_hirate)
        .def("get_position_codingame", &Board::get_position_codingame)
        .def("set_position_codingame", &Board::set_position_codingame)
        .def("get_position_string", &Board::get_position_string)
        .def("get_position_string_with_turn", &Board::get_position_string_with_turn)
        .def("set_position_string", &Board::set_position_string)
        .def("set_position_string_with_turn", &Board::set_position_string_with_turn)
        .def("do_move", &Board::do_move_py)
        .def("undo_move", &Board::undo_move)
        .def("legal_moves", &Board::legal_moves_py)
        .def("is_gameover", &Board::is_gameover)
        .def("piece_sum", &Board::piece_sum)
        .def("piece_num", static_cast<int (Board::*)() const>(&Board::piece_num))
        .def("piece_num", static_cast<int (Board::*)(int) const>(&Board::piece_num))
        .def("count_stone_diff", &Board::count_stone_diff)
        .def("winner", &Board::winner)
        .def("__str__", &Board::pretty_print);
}
