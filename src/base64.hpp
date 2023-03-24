#ifndef _BASE64_
#define _BASE64_

#include <vector>
#include <cstdint>

using namespace std;

uint32_t b64ord(char c)
{
    if (c == '+')
    {
        return 62;
    }
    else if (c == '/')
    {
        return 63;
    }
    else if (c >= 'A' && c <= 'Z')
    {
        return c - 'A';
    }
    else if (c >= 'a' && c <= 'z')
    {
        return c - 'a' + 26;
    }
    else if (c >= '0' && c <= '9')
    {
        return c - '0' + 52;
    }
    return 0; // '='
}

vector<uint8_t> b64decode(const char *data, size_t len)
{
    if (len == 0)
    {
        return {};
    }
    size_t orig_len = len / 4 * 3;
    vector<uint8_t> orig(orig_len);

    for (size_t sofs = 0, dofs = 0; sofs < len; sofs += 4, dofs += 3)
    {
        uint32_t v = b64ord(data[sofs]);
        v = (v << 6) | b64ord(data[sofs + 1]);
        v = (v << 6) | b64ord(data[sofs + 2]);
        v = (v << 6) | b64ord(data[sofs + 3]);
        orig[dofs] = (uint8_t)(v >> 16);
        orig[dofs + 1] = (uint8_t)(v >> 8);
        orig[dofs + 2] = (uint8_t)(v);
    }
    if (data[len - 1] == '=')
    {
        if (data[len - 2] == '=')
        {
            orig_len -= 2;
        }
        else
        {
            orig_len -= 1;
        }
    }
    orig.resize(orig_len);
    return orig;
}
#endif
