#include <cstdarg>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/program_options.hpp>

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

#define MAX_DIM 100000
#define INDEX(x,y) (X*(y)+(x))

// render image into a newly allocated buffer
uint8_t *render_gray8(const uint32_t *buf, size_t X, size_t Y,
        bool scale_zero, num_t (*scale_func)(uint32_t))
{
    size_t sample_count = 0;
    uint32_t sample_min = -1;
    uint32_t sample_max = 0;
    for (size_t i = 0; i < X*Y; ++i) // find range of buffer values
    {
        sample_count += buf[i];
        sample_min = std::min(sample_min,buf[i]);
        sample_max = std::max(sample_max,buf[i]);
    }
    fprintf(stderr,"samples in rectangle: %lu\n",sample_count);
    fprintf(stderr,"min sample = %u\n",sample_min);
    fprintf(stderr,"max sample = %u\n",sample_max);
    uint8_t *img = (uint8_t*) malloc(X*Y*sizeof(uint8_t));
    num_t scale_min = INFINITY;
    num_t scale_max = -INFINITY;
    for (size_t r = Y; r--;) // calculate scale range
        for (size_t c = 0; c < X; ++c)
        {
            uint32_t buf_val = buf[INDEX(r,c)];
            if (!scale_zero)
                buf_val -= sample_min;
            num_t scale_val = scale_func(buf_val);
            scale_min = std::min(scale_min,scale_val);
            scale_max = std::max(scale_max,scale_val);
        }
    fprintf(stderr,"scaled min = %f\n",scale_min);
    fprintf(stderr,"scaled max = %f\n",scale_max);
    uint8_t *img_ptr = img;
    num_t mult = _IMG8_MULT / scale_max;
    for (size_t r = Y; r--;) // compute scaled pixel values
        for (size_t c = 0; c < X; ++c)
        {
            uint32_t buf_val = buf[INDEX(r,c)];
            if (!scale_zero)
                buf_val -= sample_min;
            num_t scale_val = scale_func(buf_val);
            *(img_ptr++) = (uint8_t)(scale_val*mult);
        }
    return img;
}

// render image into a newly allocated buffer
uint16_t *render_gray16(const uint32_t *buf, size_t X, size_t Y,
        bool scale_zero, num_t (*scale_func)(uint32_t))
{
    size_t sample_count = 0;
    uint32_t sample_min = -1;
    uint32_t sample_max = 0;
    for (size_t i = 0; i < X*Y; ++i) // find range of buffer values
    {
        sample_count += buf[i];
        sample_min = std::min(sample_min,buf[i]);
        sample_max = std::max(sample_max,buf[i]);
    }
    fprintf(stderr,"samples in rectangle: %lu\n",sample_count);
    fprintf(stderr,"min sample = %u\n",sample_min);
    fprintf(stderr,"max sample = %u\n",sample_max);
    uint16_t *img = (uint16_t*) malloc(X*Y*sizeof(uint16_t));
    num_t scale_min = INFINITY;
    num_t scale_max = -INFINITY;
    for (size_t r = Y; r--;) // calculate scale range
        for (size_t c = 0; c < X; ++c)
        {
            uint32_t buf_val = buf[INDEX(r,c)];
            if (!scale_zero)
                buf_val -= sample_min;
            num_t scale_val = scale_func(buf_val);
            scale_min = std::min(scale_min,scale_val);
            scale_max = std::max(scale_max,scale_val);
        }
    fprintf(stderr,"scaled min = %f\n",scale_min);
    fprintf(stderr,"scaled max = %f\n",scale_max);
    uint16_t *img_ptr = img;
    num_t mult = _IMG16_MULT / scale_max;
    for (size_t r = Y; r--;) // compute scaled pixel values
        for (size_t c = 0; c < X; ++c)
        {
            uint32_t buf_val = buf[INDEX(r,c)];
            if (!scale_zero)
                buf_val -= sample_min;
            num_t scale_val = scale_func(buf_val);
            // byte order must be big endian for output images
            //*(img_ptr++) = (uint16_t)(scale_val*mult);
            *img_ptr = (uint16_t)(scale_val*mult);
            *img_ptr = (*img_ptr >> 8) | (*img_ptr << 8);
            ++img_ptr;
        }
    return img;
}

bool write_pgm8(std::ostream& os, size_t size_x, size_t size_y, uint8_t *img)
{
    os << "P5" << std::endl;
    os << size_x << " " << size_y << std::endl;
    os << "255" << std::endl;
    os.write((char*)img,size_x*size_y*sizeof(uint8_t));
    return os.good();
}

bool write_pgm16(std::ostream& os, size_t size_x, size_t size_y, uint16_t *img)
{
    os << "P5" << std::endl;
    os << size_x << " " << size_y << std::endl;
    os << "65535" << std::endl;
    os.write((char*)img,size_x*size_y*sizeof(uint16_t));
    return os.good();
}

bool write_png8(std::ostream& os, size_t size_x, size_t size_y, uint8_t *img)
{
    boost::gil::gray8_image_t image(size_x,size_y);
    auto image_view = boost::gil::view(image);
    for (size_t y = 0; y < size_y; ++y)
    {
        auto ptr = image_view.row_begin(y);
        for (size_t x = 0; x < size_x; ++x)
            ptr[x] = *(img++);
    }
    boost::gil::write_view(os,image_view,boost::gil::png_tag());
    return os.good();
}

bool write_png16(std::ostream& os, size_t size_x, size_t size_y, uint16_t *img)
{
    boost::gil::gray16_image_t image(size_x,size_y);
    auto image_view = boost::gil::view(image);
    for (size_t y = 0; y < size_y; ++y)
    {
        auto ptr = image_view.row_begin(y);
        for (size_t x = 0; x < size_x; ++x)
            ptr[x] = *(img++);
    }
    boost::gil::write_view(os,image_view,boost::gil::png_tag());
    return os.good();
}

bool string_ends_with(const std::string& string, const std::string& suffix)
{
    size_t l1 = string.length();
    size_t l2 = suffix.length();
    return l1 >= l2 && string.substr(l1-l2) == suffix;
}

namespace bpo = boost::program_options;

int main(int argc, char **argv)
{
    bpo::options_description op_opt("options");
    bpo::positional_options_description op_pos;
    op_opt.add_options()
        ("help,h","help information")
        ("bits,b",bpo::value<size_t>()->default_value(8),"bit depth")
        ("format,f",bpo::value<std::string>()->default_value("png"),
            "image format")
        ("input_json",bpo::value<std::string>(),"(specify as positional arg)")
        ("input_buf",bpo::value<std::string>(),"(specify as positional arg)")
        ("output_file",bpo::value<std::string>(),"(specify as positional arg)");
    op_pos.add("input_json",1);
    op_pos.add("input_buf",1);
    op_pos.add("output_file",1);
    bpo::variables_map op_varmap;
    bpo::store(bpo::command_line_parser(argc,argv)
        .options(op_opt).positional(op_pos).run(),op_varmap);
    if (op_varmap.count("help") || op_varmap.empty())
    {
        std::cerr << "usage: ffgray [options] <flame json> <flame buffer>"
            " <output image>" << std::endl;
        std::cerr << op_opt;
        exit(1);
    }
    if (!op_varmap.count("input_json") || !op_varmap.count("input_buf")
        || !op_varmap.count("output_file"))
    {
        std::cerr << "usage: ffgray [options] <flame json> <flame buffer>"
            " <output image>" << std::endl;
        std::cerr << op_opt;
        _err_exit("error: missing positional argument\n");
    }
    std::string json_input = op_varmap["input_json"].as<std::string>();
    std::string buf_input = op_varmap["input_buf"].as<std::string>();
    std::string output_file = op_varmap["output_file"].as<std::string>();
    size_t bits = op_varmap["bits"].as<size_t>();
    std::string format = op_varmap["format"].as<std::string>();
    if (bits != 8 && bits != 16)
        _err_exit("error: bit depth must be 8 or 16\n");
    if (format != "pgm" && format != "png")
        _err_exit("error: format must be pgm or png\n");
    // read input json
    Json json_input_data;
    if (json_input == "-")
        json_input_data = Json(std::cin);
    else
        json_input_data = Json(read_text_file(json_input));
    std::cerr << "input flame: " << json_input_data << std::endl;
    // parse needed parts
    union
    {
        int64_t size_x_i64;
        uint64_t size_x;
    };
    union
    {
        int64_t size_y_i64;
        uint64_t size_y;
    };
    bool scale_zero;
    std::string scale_func_name;
    JsonObject fdata;
    auto kv = fdata.end();
    if (!json_input_data.objectValue(fdata))
        _err_exit("error: input is not a json object\n");
    kv = fdata.find("size_x");
    if (kv == fdata.end())
        _err_exit("error: \"size_x\" not found\n");
    if (!kv->second.intValue(size_x_i64))
        _err_exit("error: \"size_x\" not an integer\n");
    kv = fdata.find("size_y");
    if (kv == fdata.end())
        _err_exit("error: \"size_y\" not found\n");
    if (!kv->second.intValue(size_y_i64))
        _err_exit("error: \"size_y\" not an integer\n");
    kv = fdata.find("scale_zero");
    if (kv == fdata.end())
        _err_exit("error: \"scale_zero\" not found\n");
    if (!kv->second.boolValue(scale_zero))
        _err_exit("error: \"scale_zero\" not a boolean\n");
    kv = fdata.find("scale_func");
    if (kv == fdata.end())
        _err_exit("error: \"scale_func\" not found\n");
    if (!kv->second.stringValue(scale_func_name))
        _err_exit("error: \"scale_func\" not a string\n");
    // check size
    if (size_x <= 0 || size_x >= MAX_DIM)
        _err_exit("error: \"size_x\" should be between 0 and %lu\n",MAX_DIM);
    if (size_y <= 0 || size_y >= MAX_DIM)
        _err_exit("error: \"size_y\" should be between 0 and %lu\n",MAX_DIM);
    // select scaling function: [0,inf) -> [0,inf) (monotone increasing)
    num_t (*scale_func)(uint32_t);
    if (scale_func_name == "binary") // TODO binary with threshold
        scale_func = [](uint32_t x)
            { return (num_t)(x != 0); };
    else if (scale_func_name == "linear")
        scale_func = [](uint32_t x)
            { return (num_t)x; };
    else if (scale_func_name == "log")
        scale_func = [](uint32_t x)
            { return (num_t)log(1.0+(num_t)x); };
    else if (scale_func_name == "loglog")
        scale_func = [](uint32_t x)
            { return (num_t)log(1.0+log(1.0+(num_t)x)); };
    else if (scale_func_name == "log^2")
        scale_func = [](uint32_t x)
            { num_t t = log(1.0+(num_t)x); return t*t; };
    else if (scale_func_name == "log^3") // TODO power as arg
        scale_func = [](uint32_t x)
            { num_t t = log(1.0+(num_t)x); return t*t*t; };
    else if (scale_func_name == "sqrt")
        scale_func = [](uint32_t x)
            { return (num_t)sqrt(1.0+(num_t)x)-(num_t)1.0; };
    else if (scale_func_name == "cbrt") // TODO nth root
        scale_func = [](uint32_t x)
            { return (num_t)cbrt(1.0+(num_t)x)-(num_t)1.0; };
    else if (scale_func_name == "arctan")
        scale_func = [](uint32_t x)
            { return (num_t)atan(1.0+(num_t)x)-_PI_4; };
    else if (scale_func_name == "recip")
        scale_func = [](uint32_t x)
            { return (num_t)x/((num_t)x+(num_t)1.0); };
    else
        _err_exit("error: invalid scaling function\n");
    // read input buffer
    uint32_t *buf = (uint32_t*)malloc(size_x*size_y*sizeof(uint32_t));
    if (buf_input == "-")
    {
        if (!std::cin.read((char*)buf,size_x*size_y*sizeof(uint32_t)))
            _err_exit("error: stdin may not have enough data\n");
    }
    else
    {
        std::ifstream buf_input_file(buf_input,std::ios::in|std::ios::binary);
        if (!buf_input_file.read((char*)buf,size_x*size_y*sizeof(uint32_t)))
            _err_exit("error: cannot read file or not enough data\n");
    }
    // render
    uint8_t *img8 = nullptr;
    uint16_t *img16 = nullptr;
    if (bits == 8)
        img8 = render_gray8(buf,size_x,size_y,scale_zero,scale_func);
    if (bits == 16)
        img16 = render_gray16(buf,size_x,size_y,scale_zero,scale_func);
    // write output
    if (output_file == "-")
    {
        if (bits == 8)
        {
            if (!write_pgm8(std::cout,size_x,size_y,img8))
                _err_exit("error: failure writing to stdout\n");
        }
        if (bits == 16)
        {
            if (!write_pgm16(std::cout,size_x,size_y,img16))
                _err_exit("error: failure writing to stdout\n");
        }
    }
    else if (format == "png")
    {
        std::ofstream os(output_file);
        if (!os.good())
            _err_exit("error: failure opening output file\n");
        if (bits == 8)
        {
            if (!write_png8(os,size_x,size_y,img8))
                _err_exit("error: failure writing to file\n");
        }
        if (bits == 16)
        {
            if (!write_png16(os,size_x,size_y,img16))
                _err_exit("error: failure writing to file\n");
        }
        os.close();
    }
    else // pgm
    {
        std::ofstream os(output_file);
        if (!os.good())
            _err_exit("error: failure opening output file\n");
        if (bits == 8)
        {
            if (!write_pgm8(os,size_x,size_y,img8))
                _err_exit("error: failure writing to file\n");
        }
        if (bits == 16)
        {
            if (!write_pgm16(os,size_x,size_y,img16))
                _err_exit("error: failure writing to file\n");
        }
        os.close();
    }
    free(buf);
    if (img8) free(img8);
    if (img16) free(img16);
    return 0;
}
