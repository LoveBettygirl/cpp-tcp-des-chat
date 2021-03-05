#ifndef DES_H
#define DES_H

#include <cstdint>
#include <string>
using namespace std;

class DES {
private:
    /* 基本常量定义 */
    static const uint8_t pc_first[]; // 初始置换IP
    static const uint8_t pc_last[]; // 逆初始置换IP^{-1}
    static const uint8_t des_P[]; // 置换运算P
    static const uint8_t des_E[]; // 选择扩展运算E盒
    static const uint8_t des_S[][65]; // 选择压缩运算S盒
	static const uint8_t keyleftright[]; // 等分密钥PC-1
	static const uint8_t lefttable[]; // 密钥循环左移
	static const uint8_t keychoose[]; // 密钥选取PC-2

    string plaintext; // 要加密的明文
    string ciphertext; // 要解密的密文
    string srcKey; // 初始密钥
    string keys[16]; // 供16轮迭代的子密钥
    bool mode; // 模式选择，true为加密，false为解密

    static string strxor(string s1, string s2); // 二进制字符串异或
    static string genBinaryMsg(string src); // 生成二进制文本字符串
    static string check64(string src); // 检查二进制字符串的长度是否是64位
    static string genBinaryKey(string src, int len); // 生成二进制密钥字符串
    string firstIP(string src); // 初始置换IP
    string lastIP(string src); // 逆初始置换IP^{-1}
    string ope_E(string right); // 选择扩展运算E
    string ope_S(string right); // 选择压缩运算S，将48位的二进制字符串压缩成32位
    string ope_P(string right); // 置换运算P
    string f(string right, string key); // 16次迭代的f函数，包括E运算、和子密钥的异或运算、S运算、P运算
    string itra16(string left, string right); // 16次迭代
    string *ope_pc_1(); // 使用初始64位密钥进行置换选择PC-1运算
    string ope_shift(string src, int index); // 密钥循环左移运算
    string ope_pc_2(string src); // 密钥置换选择PC-2运算，生成48位子密钥
    void genKey(); // 生成16次迭代所使用的子密钥
    void encry(); // 加密生成二进制字符串，每8个字节进行一次加密
    void decry(); // 解密二进制字符串生成明文，每8个字节进行一次解密


public:
    DES(string k);
    string getResult(string text, bool m); // 获取加密或解密的结果
    static string toCheckedBinary(int num); // 将一个int数字转换为二进制的字符串形式，最后一位为奇校验码
};

#endif
