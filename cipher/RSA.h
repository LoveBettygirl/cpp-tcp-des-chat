#ifndef RSA_H
#define RSA_H

#include <string>
#include <ctime>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <boost/multiprecision/miller_rabin.hpp>
using namespace std;
using namespace boost::multiprecision;
using namespace boost::random;

class RSAPublicKey {
private:
    uint1024_t e;
    uint1024_t n;
public:
    RSAPublicKey() {
        e = 0;
        n = 0;
    }
    RSAPublicKey(uint1024_t e, uint1024_t n) {
        this->e = e;
        this->n = n;
    }
    void setE(uint1024_t e) {
        this->e = e;
    }
    void setN(uint1024_t n) {
        this->n = n;
    }
    uint1024_t getE() {
        return e;
    }
    uint1024_t getN() {
        return n;
    }
};

class RSAPrivateKey {
private:
    uint1024_t d;
    uint1024_t n;
public:
    RSAPrivateKey() {
        d = 0;
        n = 0;
    }
    RSAPrivateKey(uint1024_t d, uint1024_t n) {
        this->d = d;
        this->n = n;
    }
    void setD(uint1024_t d) {
        this->d = d;
    }
    void setN(uint1024_t n) {
        this->n = n;
    }
    uint1024_t getD() {
        return d;
    }
    uint1024_t getN() {
        return n;
    }
};

class RSA {
private:
    uint1024_t p;
    uint1024_t q;
    uint1024_t n;
    uint1024_t euler;
    uint1024_t e;
    uint1024_t d;
    int bitLen; // p和q的长度
    int1024_t x, y; // 计算exgcd的临时变量
    RSAPublicKey publicKey;
    RSAPrivateKey privateKey;
    mt19937 r1; // 用于随机数生成的随机种子
    mt11213b r2; // 用于素性检测的随机种子

    uint1024_t genRandom(uint1024_t min, uint1024_t max); // 生成范围为(min, max)的随机整数
    int1024_t exgcd(int1024_t a, int1024_t b); // 欧几里得扩展算法
    int1024_t calculateD(int1024_t a, int1024_t k); // 求e的反模d（找到扩展欧几里得算法求出的最小正整数解x）
    int getLoop(); // 获取不同bitLen对应的应执行Miller-Rabin算法的次数
    bool testPrime(uint1024_t n); // 多次执行Miller-Rabin算法测试一个正奇数是不是素数
    uint1024_t randomPrime(); // 随机生成大素数
    int1024_t gcd(int1024_t a, int1024_t b); // 辗转相除法求最大公约数
    static uint1024_t modPow(uint1024_t base, uint1024_t index, uint1024_t mod); // 快速模幂运算
public:
    RSA(int bitLen);
    RSAPublicKey getPublicKey() { // 获取产生的RSA公钥
        return publicKey;
    }
    RSAPrivateKey getPrivateKey() { // 获取产生的RSA私钥
        return privateKey;
    }
    static string encry(string src, RSAPublicKey k); // 对明文进行一次加密
    static string decry(string src, RSAPrivateKey k); // 对密文进行一次解密
};

#endif
