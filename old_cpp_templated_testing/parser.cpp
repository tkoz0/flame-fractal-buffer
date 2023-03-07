#include <cstdarg>
#include <cstring>

#include "parser.hpp"
#include "flame/variations.h"

// print to stderr and exit
static void _err_exit(const char *f, ...)
{
    va_list args;
    va_start(args,f);
    vfprintf(stderr,f,args);
    va_end(args);
    exit(1);
}

static char *_copy_cstr(const char *s)
{
    char *ret = (char*) malloc(strlen(s)+1);
    if (!ret)
        _err_exit("memory allocation failure\n");
    strcpy(ret,s);
    return ret;
}

// some reasonable limits
#define MAX_DIM 100000
#define MAX_RECT 1e5

void _int_from_obj(const JsonObject& data, int64_t& jint, const char *key)
{
    auto kv = data.find(key);
    if (kv == data.end())
        _err_exit("error: \"%s\" not found\n",key);
    if (!kv->second.intValue(jint))
        _err_exit("error: \"%s\" not an integer\n",key);
}

void _float_from_obj(const JsonObject& data, double& jfloat, const char *key)
{
    auto kv = data.find(key);
    if (kv == data.end())
        _err_exit("error: \"%s\" not found\n",key);
    if (!kv->second.floatValue(jfloat))
        _err_exit("error: \"%s\" not a float\n",key);
}

// parse flame from JSON
void flame_from_json(const Json& input, flame_t *flame)
{
    fprintf(stderr,"parsing flame...\n");
    // store values retrieved from json
    int64_t jint;
    double jfloat;
    std::string jstr;
    // parse flame
    JsonObject fdata;
    auto kv = fdata.end();
    if (!input.objectValue(fdata))
        _err_exit("error: input is not a json object\n");
    // "name"
    kv = fdata.find("name");
    if (kv == fdata.end())
        _err_exit("error: \"name\" not found\n");
    if (!kv->second.stringValue(jstr))
        _err_exit("error: \"name\" not a string\n");
    flame->name = _copy_cstr(jstr.c_str());
    // other fields
    _int_from_obj(fdata,jint,"size_x");
    flame->size_x = jint;
    _int_from_obj(fdata,jint,"size_y");
    flame->size_y = jint;
    _int_from_obj(fdata,jint,"samples");
    flame->samples = jint;
    _float_from_obj(fdata,jfloat,"xmin");
    flame->xmin = jfloat;
    _float_from_obj(fdata,jfloat,"xmax");
    flame->xmax = jfloat;
    _float_from_obj(fdata,jfloat,"ymin");
    flame->ymin = jfloat;
    _float_from_obj(fdata,jfloat,"ymax");
    flame->ymax = jfloat;
    // check parameters
    if (flame->size_x <= 0 || flame->size_x >= MAX_DIM)
        _err_exit("error: \"size_x\" should be between 0 and %lu\n",MAX_DIM);
    if (flame->size_y <= 0 || flame->size_y >= MAX_DIM)
        _err_exit("error: \"size_y\" should be between 0 and %lu\n",MAX_DIM);
    if (flame->xmin <= -MAX_RECT || flame->xmin >= MAX_RECT)
        _err_exit("error: \"xmin\" should be between %+f and %+f\n",-MAX_RECT,MAX_RECT);
    if (flame->xmax <= -MAX_RECT || flame->xmax >= MAX_RECT)
        _err_exit("error: \"xmax\" should be between %+f and %+f\n",-MAX_RECT,MAX_RECT);
    if (flame->ymin <= -MAX_RECT || flame->ymin >= MAX_RECT)
        _err_exit("error: \"ymin\" should be between %+f and %+f\n",-MAX_RECT,MAX_RECT);
    if (flame->ymax <= -MAX_RECT || flame->ymax >= MAX_RECT)
        _err_exit("error: \"ymax\" should be between %+f and %+f\n",-MAX_RECT,MAX_RECT);
    // "xforms" (list)
    JsonArray xflist;
    kv = fdata.find("xforms");
    if (kv == fdata.end())
        _err_exit("error: \"xforms\" not found\n");
    if (!kv->second.arrayValue(xflist))
        _err_exit("error: \"xforms\" not a list\n");
    if (xflist.empty())
        _err_exit("error: \"xforms\" is empty\n");
    flame->xforms_len = xflist.size();
    flame->xforms = (xform_t*) malloc(xflist.size() * sizeof(xform_t));
    // "xforms" loop
    fprintf(stderr,"parsing xforms...\n");
    JsonObject xform;
    JsonArray vars;
    JsonObject var;
    JsonArray aff;
    for (size_t xfi = 0; xfi < xflist.size(); ++xfi)
    {
        fprintf(stderr,"parsing xforms[%lu]\n",xfi);
        if (!xflist[xfi].objectValue(xform))
            _err_exit("error: not an object\n");
        // "weight"
        _float_from_obj(xform,jfloat,"weight");
        flame->xforms[xfi].weight = jfloat;
        if (flame->xforms[xfi].weight <= 0.0)
            _err_exit("error: \"weight\" not positive\n");
        // "variations" (list)
        kv = xform.find("variations");
        if (kv == xform.end())
            _err_exit("error: \"variations\" not found\n");
        if (!kv->second.arrayValue(vars))
            _err_exit("error: \"variations\" not a list\n");
        if (vars.empty())
            _err_exit("error: \"variations\" is empty\n");
        // "variations" loop
        flame->xforms[xfi].var_len = vars.size();
        flame->xforms[xfi].varw = (num_t*) malloc(vars.size() * sizeof(num_t));
        flame->xforms[xfi].vars = (var_func_t*) malloc(vars.size() * sizeof(var_func_t));
        for (size_t vari = 0; vari < vars.size(); ++vari)
        {
            if (!vars[vari].objectValue(var))
                _err_exit("error: \"variations\"[%lu] not an object\n",vari);
            kv = var.find("name");
            if (kv == var.end())
                _err_exit("error: \"variations\"[%lu] \"name\" not found\n",vari);
            if (!kv->second.stringValue(jstr))
                _err_exit("error: \"variations\"[%lu] \"name\" not a string\n",vari);
            _float_from_obj(var,jfloat,"weight");
            flame->xforms[xfi].varw[vari] = jfloat;
            const var_info_t *var_info = find_variation(jstr.c_str());
            if (!var_info)
                _err_exit("error: \"variations\"[%lu] \"name\" is invalid\n",vari);
            flame->xforms[xfi].vars[vari] = var_info->func;
        }
        // pre affine
        kv = xform.find("pre_affine");
        if (kv == xform.end())
            _err_exit("error: \"pre_affine\" not found\n");
        if (!kv->second.arrayValue(aff))
            _err_exit("error: \"pre_affine\" not a list\n");
        if (aff.size() != 6)
            _err_exit("error: \"pre_affine\" length != 6\n");
        for (size_t affi = 0; affi < 6; ++affi)
        {
            if (!aff[affi].floatValue(jfloat))
                _err_exit("error: \"pre_affine\"[%lu] not a float\n",affi);
            switch (affi)
            {
            case 0: flame->xforms[xfi].pre_affine.a = jfloat; break;
            case 1: flame->xforms[xfi].pre_affine.b = jfloat; break;
            case 2: flame->xforms[xfi].pre_affine.c = jfloat; break;
            case 3: flame->xforms[xfi].pre_affine.d = jfloat; break;
            case 4: flame->xforms[xfi].pre_affine.e = jfloat; break;
            case 5: flame->xforms[xfi].pre_affine.f = jfloat; break;
            }
        }
        // post affine
        kv = xform.find("post_affine");
        if (kv == xform.end())
            _err_exit("error: \"post_affine\" not found\n");
        if (!kv->second.arrayValue(aff))
            _err_exit("error: \"post_affine\" not a list\n");
        if (aff.size() != 6)
            _err_exit("error: \"post_affine\" length != 6\n");
        for (size_t affi = 0; affi < 6; ++affi)
        {
            if (!aff[affi].floatValue(jfloat))
                _err_exit("error: \"post_affine\"[%lu] not a float\n",affi);
            switch (affi)
            {
            case 0: flame->xforms[xfi].post_affine.a = jfloat; break;
            case 1: flame->xforms[xfi].post_affine.b = jfloat; break;
            case 2: flame->xforms[xfi].post_affine.c = jfloat; break;
            case 3: flame->xforms[xfi].post_affine.d = jfloat; break;
            case 4: flame->xforms[xfi].post_affine.e = jfloat; break;
            case 5: flame->xforms[xfi].post_affine.f = jfloat; break;
            }
        }
    }
}
