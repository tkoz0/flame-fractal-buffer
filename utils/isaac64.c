// based on https://www.burtleburtle.net/bob/rand/isaacafa.html
#include "isaac64.h"

#define ind(mm,x) (*(uint64_t*)((uint8_t*)(mm)+((x)&((ISAAC64_RANDSIZ-1)<<3))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
    x = *(m); \
    a = (mix) + *(m2++); \
    *(m++) = y = ind(mm,x) + (a) + (b); \
    *(r++) = b = ind(mm,(y)>>ISAAC64_RANDSIZL) + (x); \
}

void isaac64_gen(isaac64_ctx *isaac)
{
    uint64_t a,b,x,y,*m,*mm,*m2,*r,*mend;
    mm = isaac->randmem;
    r = isaac->randrsl;
    a = isaac->randa;
    b = isaac->randb + (++isaac->randc);
    for (m = mm, mend = m2 = m+(ISAAC64_RANDSIZ/2); m < mend;)
    {
        rngstep(~(a^(a<<21)),a,b,mm,m,m2,r,x);
        rngstep(  a^(a>>5 ) ,a,b,mm,m,m2,r,x);
        rngstep(  a^(a<<12) ,a,b,mm,m,m2,r,x);
        rngstep(  a^(a>>33) ,a,b,mm,m,m2,r,x);
    }
    for (m2 = mm; m2 < mend;)
    {
        rngstep(~(a^(a<<21)),a,b,mm,m,m2,r,x);
        rngstep(  a^(a>>5 ) ,a,b,mm,m,m2,r,x);
        rngstep(  a^(a<<12) ,a,b,mm,m,m2,r,x);
        rngstep(  a^(a>>33) ,a,b,mm,m,m2,r,x);
    }
    isaac->randa = a;
    isaac->randb = b;
}

#define mix(a,b,c,d,e,f,g,h) \
{ \
    a -= e; f ^= h >> 9;  h += a; \
    b -= f; g ^= a << 9;  a += b; \
    c -= g; h ^= b >> 23; b += c; \
    d -= h; a ^= c << 15; c += d; \
    e -= a; b ^= d >> 14; d += e; \
    f -= b; c ^= e << 20; e += f; \
    g -= c; d ^= f >> 17; f += g; \
    h -= d; e ^= g << 14; g += h; \
}

void isaac64_init(isaac64_ctx *isaac, bool flag)
{
    size_t i;
    uint64_t a,b,c,d,e,f,g,h;
    uint64_t *m = isaac->randmem;
    uint64_t *r = isaac->randrsl;
    isaac->randa = isaac->randb = isaac->randc = 0;
    a = b = c = d = e = f = g = h = 0x9e3779b97f4a7c13LL; // golden ratio
    mix(a,b,c,d,e,f,g,h); // scramble (4 times)
    mix(a,b,c,d,e,f,g,h);
    mix(a,b,c,d,e,f,g,h);
    mix(a,b,c,d,e,f,g,h);
    for (i = 0; i < ISAAC64_RANDSIZ; i += 8)
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
        for (i = 0; i < ISAAC64_RANDSIZ; i += 8)
        {
            a += m[i+0]; b += m[i+1]; c += m[i+2]; d += m[i+3];
            e += m[i+4]; f += m[i+5]; g += m[i+6]; h += m[i+7];
            mix(a,b,c,d,e,f,g,h);
            m[i+0] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
            m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
        }
    }
    isaac64_gen(isaac); // initial results
    isaac->randcnt = ISAAC64_RANDSIZ;
}
