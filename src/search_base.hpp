#ifndef _SEARCH_BASE_
#define _SEARCH_BASE_
#include "board.hpp"

#define MAX_LEGAL_MOVES 32 // 一局面に対する合法手として想定する最大数

// AIのインターフェース
class SearchBase
{
public:
    Board board;
    virtual Move search(string &msg) = 0;
    virtual string name() = 0;
    // 新しいゲームが開始する際に呼ぶ。内部データのクリアを行う。
    virtual void newgame() {}
};
#endif
