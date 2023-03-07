#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils/jrand.h"
#include "types.h"

// iterations from the start that are not plotted for the IFS to "settle"
// the papers suggest using 20 for this value
#define SETTLE_ITERS 50

// write extra stats to stderr
#define STDERR_RENDER_STATS

// whether to run the post affine transform
// this may make sense to disable when it is not used
//#define DISABLE_POST_AFFINE

// maximum number of bad value messages to output
// only applies if STDERR_RENDER_STATS is enabled
#define BAD_VALUE_LIMIT 10

// absolute value of numbers to trigger bad value (non contractive system)
#define BAD_VALUE_THRESHOLD 1e10

// force equal probability selection for all xforms, regardless of input
//#define FORCE_EQUAL_XFORM_SELECTION

// check for NaN and very large/small values
// TODO make this faster by checking only for +-inf and NaN
static inline bool bad_value(num_t n)
{
    return (fabs(n) > BAD_VALUE_THRESHOLD) || isnan(n);
}

// adjustments that may help increase performance
void optimize_flame(flame_t *flame)
{
    // insertion sort xforms in order of decreasing weight so
    // cumulative weight selection loop ends sooner on average
    for (size_t i = 1; i < flame->xforms_len; ++i)
    {
        size_t j = i;
        while (j && flame->xforms[j-1].weight < flame->xforms[j].weight)
        {
            xform_t tmp = flame->xforms[j-1];
            flame->xforms[j-1] = flame->xforms[j];
            flame->xforms[j] = tmp;
            --j;
        }
    }
    // remove variations with weight 0
    for (size_t i = 0; i < flame->xforms_len; ++i)
    {
        xform_t *xf = flame->xforms+i;
        size_t var_count = 0;
        for (size_t j = 0; j < xf->var_len; ++j)
            if (xf->varw[j] != 0.0)
                ++var_count;
        // reallocate and store nonzero variations
        // if var_count == 0, leave memory allocated
        if (var_count > 0 && var_count < xf->var_len)
        {
            num_t *varw = malloc(var_count*sizeof(*varw));
            var_func_t *vars = malloc(var_count*sizeof(*vars));
            assert(varw);
            assert(vars);
            size_t k = 0;
            for (size_t j = 0; j < xf->var_len; ++j)
            {
                if (xf->varw[j] == 0.0)
                    continue;
                varw[k] = xf->varw[j];
                vars[k] = xf->vars[j];
                ++k;
            }
            free(xf->vars);
            free(xf->varw);
            xf->vars = vars;
            xf->varw = varw;
        }
        xf->var_len = var_count;
    }
}

// random point in [-s,s]x[-s,s]
static inline void _biunit_rand(num_t s, jrand_t *j, num_t *x, num_t *y)
{
    *x = s*(jrand_next_float(j)*2.0 - 1.0);
    *y = s*(jrand_next_float(j)*2.0 - 1.0);
}

// (x,y) -> (*xn,*yn)
static inline void _apply_affine(affine_params *af, num_t *xn, num_t *yn,
                                num_t x, num_t y)
{
    *xn = af->a*x + af->b*y + af->c;
    *yn = af->d*x + af->e*y + af->f;
}

// transforms the state with a chosen xform
static inline void _apply_xform_basic(iter_state_t *state, xform_t *xf)
{
    // transform point
    _apply_affine(&(xf->pre_affine),&(state->tx),&(state->ty),
                    state->x,state->y);
    state->vx = 0.0;
    state->vy = 0.0;
    state->xf = xf;
    for (uint32_t i = 0; i < xf->var_len; ++i) // sum variations
        (xf->vars[i])(state,xf->varw[i]);
    // update point
#ifndef DISABLE_POST_AFFINE
    _apply_affine(&(xf->post_affine),&(state->x),&(state->y),
                    state->vx,state->vy);
#else
    state->x = state->vx;
    state->y = state->vy;
#endif
}

#ifndef FORCE_EQUAL_XFORM_SELECTION
// normalize weights to sum to 1 for probability selection algorithm
static void _normalize_xform_weights(xform_t *xforms, uint32_t len)
{
    assert(len > 0);
    num_t wsum = 0.0;
    xform_t *xf, *xf2;
    for (xf = xforms; len--; ++xf)
        wsum += xf->weight;
    for (xf2 = xforms; xf2 != xf; ++xf2)
        xf2->weight /= wsum;
}
#endif

// randomly select flame based on cumulative weights
// TODO support doing this with binary search for better efficiency
static inline uint32_t _pick_xform(num_t *cw, jrand_t *jrand, uint32_t xflen)
{
#ifndef FORCE_EQUAL_XFORM_SELECTION
    uint32_t ret = 0;
    num_t rand = jrand_next_float(jrand);
    while (cw[ret] < rand)
        ++ret;
    return ret;
#else
    return jrand_next_int_mod(jrand,xflen);
#endif
}

// histogram length == flame->size_x * flame->size_y
// histogram indexed by (flame->size_x * y_pos) + x_pos
// TODO support final xform
void render_basic(flame_t *flame, uint32_t *histogram, jrand_t *jrand)
{
    uint64_t bad_value_count = 0;
    num_t xmul = (num_t) flame->size_x / (flame->xmax - flame->xmin);
    num_t ymul = (num_t) flame->size_y / (flame->ymax - flame->ymin);
    // small correction factor to ensure indexing in bounds
    xmul *= (1.0 - 8.0*_EMACH);
    ymul *= (1.0 - 8.0*_EMACH);
    num_t *cw = NULL;
#ifndef FORCE_EQUAL_XFORM_SELECTION
    _normalize_xform_weights(flame->xforms,flame->xforms_len);
    // form cumulative weights array for random xform selection
    cw = malloc(sizeof(*cw)*flame->xforms_len);
    assert(cw);
    num_t s = 0.0;
    for (uint32_t i = 0; i < flame->xforms_len; ++i)
    {
        s += flame->xforms[i].weight;
        cw[i] = s;
    }
    cw[flame->xforms_len-1] = 1.0; // to correct for rounding error
#endif
    iter_state_t state;
    state.rand = *jrand;
    _biunit_rand(1.0,jrand,&(state.x),&(state.y));
    for (uint32_t i = 0; i < SETTLE_ITERS; ++i)
        _apply_xform_basic(&state,
            flame->xforms+_pick_xform(cw,jrand,flame->xforms_len));
#ifdef STDERR_RENDER_STATS
    uint32_t *xfdist = calloc(flame->xforms_len,sizeof(*xfdist));
    assert(xfdist);
    num_t xmin=INFINITY,xmax=-INFINITY,ymin=INFINITY,ymax=-INFINITY;
#endif
    uint64_t samples = flame->samples;
    while (samples--)
    {
        uint32_t xf_i = _pick_xform(cw,jrand,flame->xforms_len);
        _apply_xform_basic(&state,flame->xforms+xf_i);
#ifdef STDERR_RENDER_STATS
        ++xfdist[xf_i];
#endif
        if (bad_value(state.x) || bad_value(state.y))
        {
            ++bad_value_count;
            if (bad_value_count <= BAD_VALUE_LIMIT)
            {
                fprintf(stderr,"renderer_basic(): bad_value (x,y) = (%f,%f)"
                    " (sample number = %lu)\n",
                    state.x,state.y,flame->samples-1-samples);
                if (bad_value_count == BAD_VALUE_LIMIT)
                {
                    fprintf(stderr,"renderer_basic(): not showing more "
                        "bad value errors\nrender_basic(): "
                        "IFS probably not contractive on average\n");
                }
            }
            _biunit_rand(1.0,jrand,&(state.x),&(state.y));
            // get the new point to settle before adding to histogram again
            for (uint32_t i = 0; i < SETTLE_ITERS; ++i)
                _apply_xform_basic(&state,
                    flame->xforms+_pick_xform(cw,jrand,flame->xforms_len));
            // count re-settling against sample count so rendering does not
            // hang when rendering a system reaching bad values frequently
            if (samples >= SETTLE_ITERS)
                samples -= SETTLE_ITERS;
            else
                samples = 0;
            continue;
        }
#ifdef STDERR_RENDER_STATS
        xmin = fmin(xmin,state.x);
        xmax = fmax(xmax,state.x);
        ymin = fmin(ymin,state.y);
        ymax = fmax(ymax,state.y);
#endif
        if (state.x < flame->xmin || state.x >= flame->xmax
            || state.y < flame->ymin || state.y >= flame->ymax)
            continue;
        uint32_t x = (state.x - flame->xmin) * xmul;
        uint32_t y = (state.y - flame->ymin) * ymul;
        ++histogram[(flame->size_x*y)+x];
    }
#ifdef STDERR_RENDER_STATS
    fprintf(stderr,"  xform distribution");
    for (uint32_t i = 0; i < flame->xforms_len; ++i)
        fprintf(stderr," %u",xfdist[i]);
    fprintf(stderr,"\n");
    fprintf(stderr,"  x extremes %f %f\n",xmin,xmax);
    fprintf(stderr,"  y extremes %f %f\n",ymin,ymax);
    fprintf(stderr,"  bad values: %lu\n",bad_value_count);
    free(xfdist);
#endif
#ifndef FORCE_EQUAL_XFORM_SELECTION
    free(cw);
#endif
}
