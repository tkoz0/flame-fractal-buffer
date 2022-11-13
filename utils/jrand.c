/*
Implementation of java.util.Random
Tested to match exactly except for nextGaussian() which is approximate
*/

#define _GNU_SOURCE
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "jrand.h"

#define JRAND_MULTIPLIER 0x5DEECE66DL
#define JRAND_ADDEND 0xBL
#define JRAND_STATE_SIZE 48
#define JRAND_MASK ((1L << JRAND_STATE_SIZE) - 1)
#define JRAND_SEED_UNIQUIFIER_INIT 8682522807148012L

static int64_t _initial_scramble(int64_t s)
{
    return (s ^ JRAND_MULTIPLIER) & JRAND_MASK;
}

static int64_t _system_time()
{
    return time(NULL) + clock() * JRAND_MULTIPLIER;
}

static int64_t _SEED_UNIQUIFIER = 8682522807148012L;

static int64_t _seed_uniquifier()
{
    return (_SEED_UNIQUIFIER = _SEED_UNIQUIFIER * 181783497276652981L);
}

void jrand_init_seed(jrand_t *j, int64_t s)
{
    j->state = _initial_scramble(s);
    j->has_g = false;
}

void jrand_init(jrand_t *j)
{
    jrand_init_seed(j, _seed_uniquifier() ^ _system_time());
}

// extract some bits, 0 < b <= 32
static int32_t _jrand_next(jrand_t *j, size_t b)
{
    j->state = (j->state * JRAND_MULTIPLIER + JRAND_ADDEND) & JRAND_MASK;
    return j->state >> (JRAND_STATE_SIZE - b);
}

// fill array arr of length len
void jrand_next_bytes(jrand_t *j, int8_t *arr, size_t len)
{
    size_t mult4len = len & (-4uL);
    int32_t *ptr = (int32_t*) arr;
    int32_t *mult4end = (int32_t*) (arr + mult4len);
    while (ptr < mult4end)
        *(ptr++) = _jrand_next(j,32);
    int8_t *ptre = (int8_t*) mult4end;
    if (ptre < arr + len)
    {
        int32_t last_bytes = _jrand_next(j,32);
        while (ptre < arr + len)
        {
            *(ptre++) = (last_bytes & 0xFF);
            last_bytes >>= 8;
        }
    }
}

int32_t jrand_next_int(jrand_t *j)
{
    return _jrand_next(j,32);
}

// requires bound 0 < b < 2^31 (not checked)
int32_t jrand_next_int_mod(jrand_t *j, int32_t b)
{
    int32_t r = _jrand_next(j,31);
    int32_t m = b - 1;
    if ((b & m) == 0) // b is a power of 2
        return (b * r) >> 31;
    int32_t u = r;
    r = u % b;
    while (u - r + m < 0)
    {
        u = _jrand_next(j,31);
        r = u % b;
    }
    return r;
}

int64_t jrand_next_long(jrand_t *j)
{
    int32_t hi = _jrand_next(j,32);
    int32_t lo = _jrand_next(j,32);
    return ((int64_t) hi << 32) + lo;
}

bool jrand_next_bool(jrand_t *j)
{
    return _jrand_next(j,1);
}

float jrand_next_float(jrand_t *j)
{
    return _jrand_next(j,24) / (float) 0x1000000;
}

double jrand_next_double(jrand_t *j)
{
    int32_t hi = _jrand_next(j,26);
    int32_t lo = _jrand_next(j,27);
    return (((int64_t) hi << 27) + lo) / (double) 0x20000000000000L;
}

// TODO
// https://developer.classpath.org/doc/java/lang/StrictMath-source.html
// these implementations for sqrt and log may make it match java exactly
double jrand_next_gaussian(jrand_t *j)
{
    if (j->has_g)
    {
        double ret = j->next_g;
        j->has_g = false;
        return ret;
    }
    else
    {
        double s = 0.0, x, y, m;
        while (s >= 1.0 || s == 0.0)
        {
            x = 2.0 * jrand_next_double(j) - 1.0;
            y = 2.0 * jrand_next_double(j) - 1.0;
            s = x*x + y*y;
        }
        m = sqrt(-2.0 * log(s) / s);
        j->has_g = true;
        // GCC warns x and y may be uninitialized
        // this never will occur so ignore the warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        j->next_g = y * m;
        return x * m;
#pragma GCC diagnostic pop
    }
}
