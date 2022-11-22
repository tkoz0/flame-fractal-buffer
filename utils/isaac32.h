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
#define ISAAC32_RANDSIZL 8
//#define ISAAC32_RANDSIZL 4

#if (ISAAC32_RANDSIZL < 3) || (ISAAC32_RANDSIZL > 16)
#error "need ISAAC32_RANDSIZL between 3 and 16"
#endif

//#define RANDSIZL ISAAC32_RANDSIZL
//#define RANDSIZ (1<<RANDSIZL)
#define ISAAC32_RANDSIZ (1<<ISAAC32_RANDSIZL)

typedef struct
{
    size_t randcnt; // number of unused values in randrsl
    uint32_t randrsl[ISAAC32_RANDSIZ]; // randomly numbers generated
    uint32_t randmem[ISAAC32_RANDSIZ]; // secret internal state
    uint32_t randa,randb,randc;
}
isaac32_ctx;

// initialize rng state, flag = use isaac->randrsl as a seed
void isaac32_init(isaac32_ctx *isaac, bool flag);

// regenerate isaac->randrsl with new values
void isaac32_gen(isaac32_ctx *isaac);

static inline uint32_t isaac32_rand(isaac32_ctx *isaac)
{
    if (unlikely(isaac->randcnt-- == 0))
    {
        isaac32_gen(isaac);
        isaac->randcnt = ISAAC32_RANDSIZ-1;
    }
    return isaac->randrsl[isaac->randcnt];
}

#ifdef __cplusplus
}
#endif
