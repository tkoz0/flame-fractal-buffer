/*
Implementation of java.util.Random
Tested to match exactly except for nextGaussian() which is approximate
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

// java.util.Random state/context
typedef struct { int64_t state; bool has_g; double next_g; } jrand_t;

// initialize with seed
void jrand_init_seed(jrand_t *j, int64_t s);

// initialize with a random seed
void jrand_init(jrand_t *j);

// fill array arr of length len
void jrand_next_bytes(jrand_t *j, int8_t *arr, size_t len);

// random 32 bit integer
int32_t jrand_next_int(jrand_t *j);

// requires bound 0 < b < 2^31 (not checked)
int32_t jrand_next_int_mod(jrand_t *j, int32_t b);

// random 64 bit integer
int64_t jrand_next_long(jrand_t *j);

// random true/false
bool jrand_next_bool(jrand_t *j);

// random single precision float in [0,1)
float jrand_next_float(jrand_t *j);

// random double precision float in [0,1)
double jrand_next_double(jrand_t *j);

// random gaussian distributed double, mean 0, stddev 1
double jrand_next_gaussian(jrand_t *j);

#ifdef __cplusplus
}
#endif
