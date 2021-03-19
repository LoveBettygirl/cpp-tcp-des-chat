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

string toBinaryString(uint1024_t num)
{
    string s;
    while (num != 0)
    {
        char bit;
        if ((num & 1) == 1)
            bit = '1';
        else
            bit = '0';
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

uint1024_t parseBinaryBigInt(string src)
{
    uint1024_t result(0);
    for (int i = 0; i < src.length(); i++)
    {
        result <<= 1;
        result |= uint1024_t((int)src[i] - 48);
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

string bigInteger2String(uint1024_t integer)
{
    int bitLen = 0;
    uint1024_t temp = integer;
    while (temp != 0)
    {
        temp >>= 1;
        bitLen++;
    }
    bitLen = bitLen % 8 == 0 ? bitLen : (bitLen / 8 + 1) * 8;
    string binary = toBinaryString(integer), zero;
    // 如果二进制位长度小于bitLen则在前面补0，否则取最后的bitLen位
    if (binary.length() < bitLen)
    {
        for (int i = binary.length(); i < bitLen; i++)
            zero += "0";
        binary = zero + binary;
    }
    else if (binary.length() > bitLen)
        binary = binary.substr(binary.length() - bitLen);
    return binary2String(binary);
}

uint1024_t string2BigInteger(string src)
{
    string s;
    for (int i = 0; i < src.length(); i++)
        s += toBinary(src[i], 8);
    return parseBinaryBigInt(s);
}
