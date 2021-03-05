#ifndef UTILS_H
#define UTILS_H

#include <string>
using namespace std;

extern string toBinaryString(uint32_t num);
extern string toBinary(uint32_t num, int digits);
extern int parseBinaryInt(string src);
extern string binary2String(string src);

#endif
