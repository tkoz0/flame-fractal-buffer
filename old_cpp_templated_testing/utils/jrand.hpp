#pragma once

#define likely(x)   __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>

template <
        int64_t multiplier = 0x5DEECE66DL,
        int64_t addend = 0xBL,
        size_t state_size = 48,
        int64_t seed_uniquifier_init = 8682522807148012L,
        int64_t seed_uniquifier_mult = 181783497276652981L>
class JavaRandomGeneric
{
private:
    static int64_t seed_uniquifier;
    int64_t state;
    bool has_g;
    double next_g;
    inline int32_t _next(size_t bits)
    {
        state = (state*multiplier + addend) & ((1L << state_size) - 1);
        return state >> (state_size - bits);
    }
public:
    JavaRandomGeneric()
    {
        setSeed();
    }
    JavaRandomGeneric(int64_t seed)
    {
        setSeed(seed);
    }
    inline void setSeed()
    {
        setSeed((seed_uniquifier *= seed_uniquifier_mult)
            ^ (time(NULL)*multiplier
                + clock()*((seed_uniquifier_init&(-4L))+1)));
    }
    inline void setSeed(int64_t seed)
    {
        state = (seed ^ multiplier) & ((1L << state_size) - 1);
        has_g = false;
    }
    void nextBytes(int8_t *arr, size_t len)
    {
        size_t mult4len = len & (-4uL);
        int32_t *ptr = (int32_t*) arr;
        int32_t *mult4end = (int32_t*) (arr + mult4len);
        while (ptr < mult4end)
            *(ptr++) = _next(32);
        int8_t *ptre = (int8_t*) mult4end;
        if (ptre < arr + len)
        {
            int32_t last_bytes = _next(32);
            while (ptre < arr + len)
            {
                *(ptre++) = (last_bytes & 0xFF);
                last_bytes >>= 8;
            }
        }
    }
    inline int32_t nextInt()
    {
        return _next(32);
    }
    int32_t nextInt(int32_t n)
    {
        if (unlikely(n <= 0))
            throw "bound must be positive";
        if ((n & -n) == n)
            return (int32_t)((n * (int64_t)_next(31)) >> 31);
        int32_t bits,val;
        do
        {
            bits = _next(31);
            val = bits % n;
        }
        while (unlikely(bits - val + (n - 1) < 0));
        return val;
    }
    inline int64_t nextLong()
    {
        int32_t hi = _next(32);
        int32_t lo = _next(32);
        return ((int64_t)hi << 32) + lo;
    }
    inline bool nextBool()
    {
        return _next(1);
    }
    inline float nextFloat()
    {
        return _next(24) / (float) 0x1000000;
    }
    inline double nextDouble()
    {
        int32_t hi = _next(26);
        int32_t lo = _next(27);
        return (((int64_t)hi << 27) + lo) / (double) 0x20000000000000L;
    }
    // does not match java exactly, observed to be up to 8 ulps off
    // https://developer.classpath.org/doc/java/lang/StrictMath-source.html
    double nextGaussian()
    {
        if (has_g)
        {
            has_g = false;
            return next_g;
        }
        double v1, v2, s;
        do
        {
            v1 = 2.0*nextDouble() - 1.0;
            v2 = 2.0*nextDouble() - 1.0;
            s = v1*v1 + v2*v2;
        }
        while (s >= 1.0);
        double norm = sqrt(-2.0*log(s)/s);
        next_g = v2*norm;
        has_g = true;
        return v1*norm;
    }
};

template <
        int64_t multiplier,
        int64_t addend,
        size_t state_size,
        int64_t seed_uniquifier_init,
        int64_t seed_uniquifier_mult>
int64_t JavaRandomGeneric<multiplier,addend,state_size,seed_uniquifier_init,
    seed_uniquifier_mult>::seed_uniquifier = seed_uniquifier_init;

typedef JavaRandomGeneric<> JavaRandom;