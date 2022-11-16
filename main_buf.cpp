#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>

#include "parser.hpp"
#include "flame/renderer.h"
#include "flame/types.h"
#include "utils/json_small.hpp"

// print to stderr and exit
static void _err_exit(const char *f, ...)
{
    va_list args;
    va_start(args,f);
    vfprintf(stderr,f,args);
    va_end(args);
    exit(1);
}

// read entire contents of file into a string
std::string read_text_file(const std::string name)
{
    std::ifstream input;
    input.open(name);
    std::stringstream ss;
    ss << input.rdbuf();
    input.close();
    return ss.str();
}

// frees dynamic memory of flame, not the memory pointed to by f
void destroy_flame(flame_t *f)
{
    if (f->name)
        free(f->name);
    for (size_t i = 0; i < f->xforms_len; ++i)
    {
        free(f->xforms[i].vars);
        free(f->xforms[i].varw);
    }
    if (f->xforms)
        free(f->xforms);
}

// returns a newly allocated rendering buffer
uint32_t *render_flame(flame_t *f)
{
    fprintf(stderr,"rendering flame...\n");
    jrand_t j;
    jrand_init(&j);
    uint32_t *buf = (uint32_t*) calloc(f->size_x*f->size_y,sizeof(uint32_t));
    clock_t r_start = clock();
    render_basic(f,buf,&j);
    clock_t r_end = clock();
    float r_secs = (r_end-r_start)/(float)CLOCKS_PER_SEC;
    fprintf(stderr,"done\n");
    fprintf(stderr,"time = %f sec\n",r_secs);
    fprintf(stderr,"samples/sec = %f\n",f->samples/r_secs);
    size_t sample_count = 0;
    uint32_t sample_min = -1;
    uint32_t sample_max = 0;
    for (size_t i = 0; i < f->size_x*f->size_y; ++i)
    {
        sample_count += buf[i];
        sample_min = std::min(sample_min,buf[i]);
        sample_max = std::max(sample_max,buf[i]);
    }
    float percent = (float)sample_count/(float)f->samples * 100.0;
    fprintf(stderr,"samples in rectangle = %lu (%f%%)\n",sample_count,percent);
    fprintf(stderr,"min sample = %u\n",sample_min);
    fprintf(stderr,"max sample = %u\n",sample_max);
    return buf;
}

namespace bpo = boost::program_options;

int main(int argc, char **argv)
{
    bpo::options_description op_opt("options");
    bpo::positional_options_description op_pos;
    op_opt.add_options()
        ("help,h","help information")
        ("input","(specify as positional arg)")
        ("output","(specify as positional arg)");
    op_pos.add("input",1);
    op_pos.add("output",1);
    bpo::variables_map op_varmap;
    bpo::store(bpo::command_line_parser(argc,argv)
        .options(op_opt).positional(op_pos).run(),op_varmap);
    if (op_varmap.count("help") || op_varmap.empty())
    {
        std::cerr << "usage: ffbuf <input json> <output buffer>" << std::endl;
        std::cerr << op_opt;
        exit(1);
    }
    if (!op_varmap.count("input") || !op_varmap.count("output"))
    {
        std::cerr << "usage: ffbuf <input json> <output buffer>" << std::endl;
        std::cerr << op_opt;
        _err_exit("error: missing positional argument\n");
    }
    std::string input_file = op_varmap["input"].as<std::string>();
    std::string output_file = op_varmap["output"].as<std::string>();
    Json input_data;
    if (input_file == "-") // input from stdin
        input_data = Json(std::cin);
    else // input from file
        input_data = Json(read_text_file(input_file));
    // test
    std::cerr << "input flame: " << input_data << std::endl;
    flame_t flame;
    flame.name = nullptr;
    flame.xforms = nullptr;
    flame.xforms_len = 0;
    flame_from_json(input_data,&flame);
    uint32_t *buf = render_flame(&flame);
    if (output_file == "-") // write to stdout
        std::cout.write((char*)buf,flame.size_x*flame.size_y*sizeof(*buf));
    else // write to file
    {
        std::ofstream out_file(output_file,std::ios::out|std::ios::binary);
        out_file.write((char*)buf,flame.size_x*flame.size_y*sizeof(*buf));
        out_file.close();
    }
    destroy_flame(&flame);
    free(buf);
    return 0;
}
