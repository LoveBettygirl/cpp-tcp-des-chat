#include "RSA.h"
#include "utils.h"

RSA::RSA(int bitLen)
{
    r1 = mt19937((unsigned int)time(nullptr));
    r2 = mt11213b((unsigned int)time(nullptr));
    this->bitLen = bitLen;
    p = randomPrime();
    q = randomPrime();
    n = p * q;
    euler = (p - 1) * (q - 1);
    do
    {
        e = genRandom(1, euler);
    } while (gcd(e, euler) != 1);
    // 求e对euler的逆元d
    // 注意gcd()和exgcd()的计算必须是有符号的
    d = uint1024_t(calculateD(e, euler));
    publicKey = RSAPublicKey(e, n);
    privateKey = RSAPrivateKey(d, n);
}

uint1024_t RSA::genRandom(uint1024_t min, uint1024_t max)
{
    boost::random::uniform_int_distribution<uint1024_t> ran(min + 1, max - 1);
    uint1024_t num = ran(r1);
    return num;
}

string RSA::encry(string src, RSAPublicKey k)
{
    uint1024_t integer = string2BigInteger(src);
    if (integer >= k.getN())
    {
        perror("Data must not be larger than n");
        exit(1);
    }
    integer = modPow(integer, k.getE(), k.getN());
    return toBinaryString(integer);
}

string RSA::decry(string src, RSAPrivateKey k)
{
    uint1024_t integer = parseBinaryBigInt(src);
    integer = modPow(integer, k.getD(), k.getN());
    return bigInteger2String(integer);
}

int1024_t RSA::exgcd(int1024_t a, int1024_t b)
{
    if (b == 0)
    {
        x = 1;
        y = 0;
        return a;
    }
    int1024_t result = exgcd(b, a % b);
    int1024_t temp = x;
    x = y;
    y = temp - (a / b) * y;
    return result;
}

int1024_t RSA::calculateD(int1024_t a, int1024_t k)
{
    int1024_t d = exgcd(a, k);
    // 判断最大公约数是否为1，否则无解
    if (d == 1)
        return (x % k + k) % k; // 求出的x可能为负，需要转为最小正整数解
    else
        return -1;
}

int RSA::getLoop()
{
    int rounds;
    if (this->bitLen < 100)
        rounds = 50;
    else if (this->bitLen < 256)
        rounds = 27;
    else if (this->bitLen < 512)
        rounds = 15;
    else if (this->bitLen < 768)
        rounds = 8;
    else if (this->bitLen < 1024)
        rounds = 4;
    else
        rounds = 2;
    return rounds;
}

bool RSA::testPrime(uint1024_t n)
{
    // boost库实现了Miller-Rabin算法，不想写了
    if (miller_rabin_test(n, getLoop(), r2))
        return true;
    return false;
}

uint1024_t RSA::randomPrime()
{
    uint1024_t num;
    do
    {
        num = genRandom(1, uint1024_t(1) << this->bitLen);
        num |= (uint1024_t(1) << (this->bitLen - 1)); // 设置生成的随机数最高位为1，保证足够大
        num |= 1;                                     // 保证这个数为奇数
    } while (!testPrime(num));
    return num;
}

int1024_t RSA::gcd(int1024_t a, int1024_t b)
{
    if (b == 0)
        return a;
    int1024_t mod = a % b;
    while (mod != 0)
    {
        a = b;
        b = mod;
        mod = a % b;
    }
    return b;
}

uint1024_t RSA::modPow(uint1024_t base, uint1024_t index, uint1024_t mod)
{
    string indexBinary = toBinaryString(index);
    uint1024_t result = uint1024_t(base);
    for (int i = 1; i < indexBinary.length(); i++)
    {
        result = (result * result) % mod;
        if (indexBinary[i] == '1')
            result = (result * base) % mod;
    }
    return result;
}
