/*
Types
*/

#pragma once

#include <stdint.h>

#include "../utils/jrand.h"

// machine epsilon
#define _EMACH32 (1.0F / (float)(1  << 23)) // 1.1920928955078125e-07
#define _EMACH64 (1.0 / (double)(1L << 52)) // 2.220446049250313e-16

// floating point type for calculations
typedef float num_t;
//typedef double num_t;

// multiplier for scaling to 8 bit pixel values
#define _IMG256_MULT (256.0 * (1.0F - 8.0*_EMACH32))
//#define _IMG256_MULT (256.0 * (1.0  - 8.0*_EMACH64))

#define _EMACH _EMACH32
//#define _EMACH _EMACH64

// pi based constants
#define _PI    ((num_t) 3.141592653589793)
#define _2PI   ((num_t) 6.283185307179586)
#define _1_PI  ((num_t) 0.3183098861837907)
#define _2_PI  ((num_t) 0.6366197723675814)
#define _PI_2  ((num_t) 1.5707963267948966)
#define _PI_3  ((num_t) 1.0471975511965976)
#define _PI_4  ((num_t) 0.7853981633974483)
#define _PI_6  ((num_t) 0.5235987755982988)
#define _PI_12 ((num_t) 0.2617993877991494)

// e based constants
#define _E    ((num_t) 2.718281828459045)
#define _1_E  ((num_t) 0.36787944117144233)
#define _LOG2 ((num_t) 0.6931471805599453)

// sqrt based constants
#define _SQRT2   ((num_t) 1.4142135623730951)
#define _SQRT3   ((num_t) 1.7320508075688772)
#define _SQRT5   ((num_t) 2.23606797749979)
#define _1_SQRT2 ((num_t) 0.7071067811865475)
#define _1_SQRT3 ((num_t) 0.5773502691896258)
#define _1_SQRT5 ((num_t) 0.4472135954999579)
#define _PHI     ((num_t) 1.618033988749895)
#define _1_PHI   ((num_t) 0.6180339887498948)

// small number for avoiding division by zero
#define _EPS ((num_t) 1e-10)

// identity affine transformation (x,y) -> (x,y)
#define NULL_AFFINE ((affine_params){1.0,0.0,0.0,0.0,1.0,0.0})

// parameters for affine transformation
typedef struct
{
    // (x,y) -> (a*x+b*y+c,d*x+e*y+f)
    num_t a, b, c, d, e, f;
}
affine_params;

// affine identity transformation (x,y) -> (x,y)
extern const affine_params null_affine;

// some helper types, currently unused
typedef struct { uint8_t r, g, b; } rgb_t;
typedef struct { uint8_t r, g, b, a; } rgba_t;
typedef struct { num_t x, y; } point_t;

// parameters for variations
// TODO add to this for more variations
// TODO may make sense to limit size of this if variations are not needed
typedef struct
{
    num_t blob_high;
    num_t blob_low;
    num_t blob_waves;
}
var_params_t;

typedef struct iter_state_t iter_state_t; // forward declare

// variation function type
typedef void (*var_func_t)(iter_state_t*,num_t);

// xform
typedef struct
{
    num_t weight; // probability to select (will be normalized)
    var_func_t *vars; // function pointers to variations
    num_t *varw; // variation wewights
    size_t var_len; // number of variations
    affine_params pre_affine;
    affine_params post_affine;
    var_params_t var_params; // other variation parameters
    //uint32_t pc_flags; // which values to precalculate
}
xform_t;

// flame
typedef struct
{
    char *name;
    size_t size_x, size_y; // dimensions for histogram
    uint64_t samples; // number of iterations
    num_t xmin, xmax, ymin, ymax; // bounds for rectangle to render
    xform_t *xforms;
    size_t xforms_len;
}
flame_t;

// iteration state variables
struct iter_state_t
{
    num_t x, y; // current point
    num_t tx, ty; // pre affine transform applied
    num_t vx, vy; // variation sum
    jrand_t rand; // RNG state
    xform_t *xf; // xform selected (contains params)
    // precalculated variables (TODO enable)
    // num_t pc_theta, pc_phi;
    // num_t pc_sint, pc_cost;
    // num_t pc_r, pc_r2;
};

// basic information about variations
typedef struct
{
    const char *name;
    const var_func_t func;
    const uint32_t flags;
}
var_info_t;
