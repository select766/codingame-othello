#ifndef _SEARCH_BASE_
#define _SEARCH_BASE_
#include "board.hpp"

// AIのインターフェース
class SearchBase
{
public:
    Board board;
    virtual Move search(string &msg) = 0;
    virtual string name() = 0;
};
#endif
