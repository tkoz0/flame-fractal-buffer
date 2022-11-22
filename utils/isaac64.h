// based on https://www.burtleburtle.net/bob/rand/isaacafa.html
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define likely(x)   __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)

#ifdef __cplusplus
extern "C"
{
#endif

// constant determining the size of the internal state
#define ISAAC64_RANDSIZL 8
//#define ISAAC64_RANDSIZL 4

#if (ISAAC64_RANDSIZL < 3) || (ISAAC64_RANDSIZL > 16)
#error "need ISAAC64_RANDSIZL between 3 and 16"
#endif

//#define RANDSIZL ISAAC64_RANDSIZL
//#define RANDSIZ (1<<RANDSIZL)
#define ISAAC64_RANDSIZ (1<<ISAAC64_RANDSIZL)

typedef struct
{
    size_t randcnt; // number of unused values in randrsl
    uint64_t randrsl[ISAAC64_RANDSIZ]; // randomly numbers generated
    uint64_t randmem[ISAAC64_RANDSIZ]; // secret internal state
    uint64_t randa,randb,randc;
}
isaac64_ctx;

// initialize rng state, flag = use isaac->randrsl as a seed
void isaac64_init(isaac64_ctx *isaac, bool flag);

// regenerate isaac->randrsl with new values
void isaac64_gen(isaac64_ctx *isaac);

static inline uint64_t isaac64_rand(isaac64_ctx *isaac)
{
    if (unlikely(isaac->randcnt-- == 0))
    {
        isaac64_gen(isaac);
        isaac->randcnt = ISAAC64_RANDSIZ-1;
    }
    return isaac->randrsl[isaac->randcnt];
}

#ifdef __cplusplus
}
#endif
