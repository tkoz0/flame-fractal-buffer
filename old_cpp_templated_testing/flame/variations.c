#define _GNU_SOURCE
#include <math.h>
#include <string.h>

#include "../utils/jrand.h"
#include "types.h"
#include "variations.h"

// squared 2-norm (precalc_sumsq)
static inline num_t _r2(num_t x, num_t y)
{
    return x*x + y*y;
}

// 2-norm (precalc_sqrt)
static inline num_t _r(num_t x, num_t y)
{
    return hypot(x,y);
}

// angles

// atan2(x,y) (precalc_atan)
static inline num_t _theta(num_t x, num_t y)
{
    return atan2(x,y);
}

// atan2(y,x) (precalc_atanyx)
static inline num_t _phi(num_t x, num_t y)
{
    return atan2(y,x);
}

// tables for random variables
static const num_t _OMEGA_TABLE[2] = { 0, 3.141592653589793 };
static const num_t _LAMBDA_TABLE[2] = { 1.0, -1.0 };

// random variables

// random in [0,1)
static inline num_t _psi(jrand_t *j)
{
    return jrand_next_float(j);
}

// 0 or pi
static inline num_t _omega(jrand_t *j)
{
    return _OMEGA_TABLE[jrand_next_bool(j)];
}

// 1 or -1
static inline num_t _lambda(jrand_t *j)
{
    return _LAMBDA_TABLE[jrand_next_bool(j)];
}

// helper macros (prefix _C_ for compute)
#define _X (S->tx)
#define _Y (S->ty)
#define _C_R2 _r2(S->tx,S->ty)
#define _C_R _r(S->tx,S->ty)
#define _C_ATAN _theta(S->tx,S->ty)
#define _C_ATANYX _phi(S->tx,S->ty)
#define _PARAMS (S->xf->var_params)
// define this since the sincos depends on the type of num_t
// regular sin/cos can use the type generic macros
#define _SINCOS(x,y,z) sincosf(x,y,z)
// random values
#define _R_UNIF _psi(&S->rand)
#define _R_0_PI _omega(&S->rand)
#define _R_1_NEG1 _lambda(&S->rand)
// pre affine constants
#define _PRE_A (S->xf->pre_affine.a)
#define _PRE_B (S->xf->pre_affine.b)
#define _PRE_C (S->xf->pre_affine.c)
#define _PRE_D (S->xf->pre_affine.d)
#define _PRE_E (S->xf->pre_affine.e)
#define _PRE_F (S->xf->pre_affine.f)

// TODO for efficiency, some of the following should be precalculated
// more values may be good to precalculate, and some variation specific ones
// all these depend on tx,ty but some may only depend on the xform
// atan2(x,y) (theta)
// atan2(y,x) (phi)
// sin(theta), cos(theta)
// r = sqrt(x*x + y*y) (and r^2)
// sin(x), sin(y), cos(x), cos(y), sin(r), cos(r)

void var0_linear(iter_state_t *S, num_t W)
{
    S->vx += W * _X;
    S->vy += W * _Y;
}

void var1_sinusoidal(iter_state_t *S, num_t W)
{
    S->vx += W * sin(_X);
    S->vy += W * sin(_Y);
}

void var2_spherical(iter_state_t *S, num_t W)
{
    num_t r = W / (_C_R2 + _EPS);
    S->vx += r * _X;
    S->vy += r * _Y;
}

void var3_swirl(iter_state_t *S, num_t W)
{
    num_t sr,cr;
    _SINCOS(_C_R2,&sr,&cr);
    S->vx += W * (sr*_X - cr*_Y);
    S->vy += W * (cr*_X + sr*_Y);
}

void var4_horseshoe(iter_state_t *S, num_t W)
{
    num_t r = W / (_C_R + _EPS);
    S->vx += (_X-_Y) * (_X+_Y) * r;
    S->vy += 2.0*_X*_Y * r;
}

void var5_polar(iter_state_t *S, num_t W)
{
    S->vx += W * _C_ATAN * _1_PI;
    S->vy += W * (_C_R - 1.0);
}

void var6_handkerchief(iter_state_t *S, num_t W)
{
    num_t a = _C_ATAN;
    num_t r = _C_R;
    num_t rw = W * r;
    S->vx += rw * sin(a+r);
    S->vy += rw * cos(a-r);
}

void var7_heart(iter_state_t *S, num_t W)
{
    num_t r = _C_R;
    num_t sin_a,cos_a;
    _SINCOS(r*_C_ATAN,&sin_a,&cos_a);
    r *= W;
    S->vx += r * sin_a;
    S->vy += (-r) * cos_a;
}

void var8_disc(iter_state_t *S, num_t W)
{
    num_t a = _C_ATAN * _1_PI * W;
    num_t sr,cr;
    _SINCOS(_PI*_C_R,&sr,&cr);
    S->vx += sr * a;
    S->vy += cr * a;
}

void var9_spiral(iter_state_t *S, num_t W)
{
    num_t a = _C_ATAN;
    num_t r = _C_R + _EPS;
    num_t sr,cr;
    _SINCOS(r,&sr,&cr);
    num_t r1 = W/r;
    S->vx += r1 * (cos(a) + sr);
    S->vy += r1 * (sin(a) - cr);
}

void var10_hyperbolic(iter_state_t *S, num_t W)
{
    num_t r = _C_R + _EPS;
    num_t a = _C_ATAN;
    S->vx += W * sin(a) / r;
    S->vy += W * cos(a) * r;
}

void var11_diamond(iter_state_t *S, num_t W)
{
    num_t a = _C_ATAN;
    num_t sr,cr;
    _SINCOS(_C_R,&sr,&cr);
    S->vx += W * sin(a) * cr;
    S->vy += W * cos(a) * sr;
}

void var12_ex(iter_state_t *S, num_t W)
{
    num_t a = _C_ATAN;
    num_t r = _C_R;
    num_t n0 = sin(a+r);
    num_t n1 = cos(a-r);
    num_t m0 = n0*n0*n0 * r;
    num_t m1 = n1*n1*n1 * r;
    S->vx += W * (m0 + m1);
    S->vy += W * (m0 - m1);
}

void var13_julia(iter_state_t *S, num_t W)
{
    num_t r = _C_R * W;
    num_t sa,ca;
    _SINCOS(0.5*_C_ATAN + _R_0_PI,&sa,&ca);
    S->vx += r * ca;
    S->vy += r * sa;
}

void var14_bent(iter_state_t *S, num_t W)
{
    num_t x = _X;
    num_t y = _Y;
    // operations without branching
    x *= (num_t[]){1.0,2.0}[x < 0.0];
    y *= (num_t[]){1.0,0.5}[y < 0.0];
    S->vx += W * x;
    S->vy += W * y;
}

void var15_waves(iter_state_t *S, num_t W)
{
    // TODO precalculate dx2,dy2 for efficiency (dependent only on xform)
    num_t dx2 = 1.0 / (_PRE_C*_PRE_C + _EPS);
    num_t dy2 = 1.0 / (_PRE_F*_PRE_F + _EPS);
    num_t x = _X * _PRE_B * sin(_Y * dx2);
    num_t y = _Y * _PRE_E * sin(_X * dy2);
    S->vx += W * x;
    S->vy += W * y;
}

void var16_fisheye(iter_state_t *S, num_t W)
{
    num_t r = 2.0 * W / (_C_R + 1.0);
    S->vx += r * _Y;
    S->vy += r * _X;
}

void var17_popcorn(iter_state_t *S, num_t W)
{
    S->vx += W * (_X + _PRE_C * sin(tan(3*_Y)));
    S->vy += W * (_Y + _PRE_F * sin(tan(3*_X)));
}

void var18_exponential(iter_state_t *S, num_t W)
{
    num_t dx = W * exp(_X - 1.0);
    num_t sdy,cdy;
    _SINCOS(_PI*_Y,&sdy,&cdy);
    S->vx += dx * cdy;
    S->vy += dx * sdy;
}

void var19_power(iter_state_t *S, num_t W)
{
    num_t a = _C_ATAN;
    num_t sina = sin(a);
    num_t r = W * pow(_C_R,sina);
    S->vx += r * cos(a);
    S->vy += r * sina;
}

void var20_cosine(iter_state_t *S, num_t W)
{
    num_t a = _X * _PI;
    num_t sa,ca;
    _SINCOS(a,&sa,&ca);
    S->vx += W * (ca * cosh(_Y));
    S->vy += W * ((-sa) * sinh(_Y));
}

void var21_rings(iter_state_t *S, num_t W)
{
    // TODO precalculate dx (only depends on xform)
    num_t dx = _PRE_C*_PRE_C + _EPS;
    num_t r = _C_R;
    r = W * (fmod(r+dx,2.0*dx) - dx + r * (1.0 - dx));
    num_t a = _C_ATAN;
    S->vx += r * cos(a);
    S->vy += r * sin(a);
}

void var22_fan(iter_state_t *S, num_t W)
{
    // TODO precalculate dx,dy (only depends on xform)
    num_t dx = _PI * (_PRE_C*_PRE_C + _EPS);
    num_t dy = _PRE_F;
    num_t dx2 = dx * 0.5;
    num_t a = _C_ATAN;
    num_t r = W * _C_R;
    num_t sa,ca;
    // add to a without branching
    num_t m = (num_t[]){1.0,-1.0}[fmod(a+dy,dx) > dx2];
    a += m * dx2;
    _SINCOS(a,&sa,&ca);
    S->vx += r * ca;
    S->vy += r * sa;
}

void var23_blob(iter_state_t *S, num_t W)
{
    num_t r = _C_R * W;
    num_t a = _C_ATAN;
    // TODO precalculate bdiff
    //num_t bdiff = _PARAMS.blob_high - _PARAMS.blob_low;
    //r *= _PARAMS.blob_low + bdiff*(0.5 + 0.5*sin(_PARAMS.blob_waves*a));
    S->vx += r * sin(a);
    S->vy += r * cos(a);
}

const var_info_t VARIATIONS[] =
{
    {"linear", &var0_linear, 0},
    {"sinusoidal", &var1_sinusoidal, 0},
    {"spherical", &var2_spherical, 0},
    {"swirl", &var3_swirl, 0},
    {"horseshoe", &var4_horseshoe, 0},
    {"polar", &var5_polar, 0},
    {"handkerchief", &var6_handkerchief, 0},
    {"heart", &var7_heart, 0},
    {"disc", &var8_disc, 0},
    {"spiral", &var9_spiral, 0},
    {"hyperbolic", &var10_hyperbolic, 0},
    {"diamond", &var11_diamond, 0},
    {"ex", &var12_ex, 0},
    {"julia", &var13_julia, 0},
    {"bent", &var14_bent, 0},
    {"waves", &var15_waves, 0},
    {"fisheye", &var16_fisheye, 0},
    {"popcorn", &var17_popcorn, 0},
    {"exponential", &var18_exponential, 0},
    {"power", &var19_power, 0},
    {"cosine", &var20_cosine, 0},
    {"rings", &var21_rings, 0},
    {"fan", &var22_fan, 0},
    //{"blob", &var23_blob, 0},
    {NULL,NULL,0}
};

const var_info_t *find_variation(const char *name)
{
    const var_info_t *ptr = VARIATIONS;
    while (ptr->name)
    {
        if (!strcmp(ptr->name,name))
            return ptr;
        ++ptr;
    }
    return NULL;
}
