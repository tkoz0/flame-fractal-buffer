/*
Variations
*/

#pragma once

#include "types.h"

extern const var_info_t VARIATIONS[];

// find a variation by name, returns NULL if not found
#ifdef __cplusplus
extern "C"
#endif
const var_info_t *find_variation(const char *name);

// precalc flags
#define PC_THETA (1 << 0)
#define PC_PHI   (1 << 1)
#define PC_SINT  (1 << 2)
#define PC_COST  (1 << 3)
#define PC_R     (1 << 4)
#define PC_R2    (1 << 5)
