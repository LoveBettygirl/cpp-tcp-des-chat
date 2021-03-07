#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <boost/multiprecision/cpp_int.hpp>
using namespace std;
using namespace boost::multiprecision;

extern string toBinaryString(uint32_t num);
extern string toBinaryString(uint1024_t num);
extern string toBinary(uint32_t num, int digits);
extern int parseBinaryInt(string src);
extern uint1024_t parseBinaryBigInt(string src);
extern string binary2String(string src);
extern string bigInteger2String(uint1024_t integer);
extern uint1024_t string2BigInteger(string src);

#endif
