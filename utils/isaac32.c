// based on https://www.burtleburtle.net/bob/rand/isaacafa.html
#include "isaac32.h"

#define ind(mm,x) (*(uint32_t*)((uint8_t*)(mm)+((x)&((ISAAC32_RANDSIZ-1)<<2))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
    x = *(m); \
    a = (mix) + *(m2++); \
    *(m++) = y = ind(mm,x) + (a) + (b); \
    *(r++) = b = ind(mm,(y)>>ISAAC32_RANDSIZL) + (x); \
}

void isaac32_gen(isaac32_ctx *isaac)
{
    uint32_t a,b,x,y,*m,*mm,*m2,*r,*mend;
    mm = isaac->randmem;
    r = isaac->randrsl;
    a = isaac->randa;
    b = isaac->randb + (++isaac->randc);
    for (m = mm, mend = m2 = m+(ISAAC32_RANDSIZ/2); m < mend;)
    {
        rngstep(a^(a<<13),a,b,mm,m,m2,r,x);
        rngstep(a^(a>>6 ),a,b,mm,m,m2,r,x);
        rngstep(a^(a<<2 ),a,b,mm,m,m2,r,x);
        rngstep(a^(a>>16),a,b,mm,m,m2,r,x);
    }
    for (m2 = mm; m2 < mend;)
    {
        rngstep(a^(a<<13),a,b,mm,m,m2,r,x);
        rngstep(a^(a>>6 ),a,b,mm,m,m2,r,x);
        rngstep(a^(a<<2 ),a,b,mm,m,m2,r,x);
        rngstep(a^(a>>16),a,b,mm,m,m2,r,x);
    }
    isaac->randa = a;
    isaac->randb = b;
}

#define mix(a,b,c,d,e,f,g,h) \
{ \
    a ^= b << 11; d += a; b += c; \
    b ^= c >> 2 ; e += b; c += d; \
    c ^= d << 8 ; f += c; d += e; \
    d ^= e >> 16; g += d; e += f; \
    e ^= f << 10; h += e; f += g; \
    f ^= g >> 4 ; a += f; g += h; \
    g ^= h << 8 ; b += g; h += a; \
    h ^= a >> 9 ; c += h; a += b; \
}

void isaac32_init(isaac32_ctx *isaac, bool flag)
{
    size_t i;
    uint32_t a,b,c,d,e,f,g,h;
    uint32_t *m = isaac->randmem;
    uint32_t *r = isaac->randrsl;
    isaac->randa = isaac->randb = isaac->randc = 0;
    a = b = c = d = e = f = g = h = 0x9e3779b9; // golden ratio
    mix(a,b,c,d,e,f,g,h); // scramble (4 times)
    mix(a,b,c,d,e,f,g,h);
    mix(a,b,c,d,e,f,g,h);
    mix(a,b,c,d,e,f,g,h);
    for (i = 0; i < ISAAC32_RANDSIZ; i += 8)
    {
        if (flag)
        {
            a += r[i+0]; b += r[i+1]; c += r[i+2]; d += r[i+3];
            e += r[i+4]; f += r[i+5]; g += r[i+6]; h += r[i+7];
        }
        mix(a,b,c,d,e,f,g,h);
        m[i+0] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
        m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
    }
    if (flag) // 2nd pass to make seed affect all of randmem
    {
        for (i = 0; i < ISAAC32_RANDSIZ; i += 8)
        {
            a += m[i+0]; b += m[i+1]; c += m[i+2]; d += m[i+3];
            e += m[i+4]; f += m[i+5]; g += m[i+6]; h += m[i+7];
            mix(a,b,c,d,e,f,g,h);
            m[i+0] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
            m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
        }
    }
    isaac32_gen(isaac); // initial results
    isaac->randcnt = ISAAC32_RANDSIZ;
}
