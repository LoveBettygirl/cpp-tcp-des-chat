#include "utils.h"

string toBinaryString(uint32_t num)
{
    string s;
    while (num)
    {
        char bit = (char)((num & 1) + 48);
        s = string(1, bit) + s;
        num >>= 1;
    }
    return s;
}

string toBinary(uint32_t num, int digits)
{
    string s = toBinaryString(num);
    if (s.length() < digits)
    {
        string cover = toBinaryString(1 << digits).substr(1);
        return cover.substr(s.length()) + s;
    }
    else if (s.length() > digits)
        return s.substr(s.length() - digits);
    return s;
}

int parseBinaryInt(string src)
{
    int result = 0;
    for (int i = 0; i < src.length(); i++)
    {
        result <<= 1;
        result |= ((int)src[i] - 48);
    }
    return result;
}

string binary2String(string src)
{
    string s;
    for (int i = 0; i < src.length(); i += 8)
        s.push_back(parseBinaryInt(src.substr(i, 8)));
    return s;
}
