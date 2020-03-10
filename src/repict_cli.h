
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repict.h"

#define MAX_FUNCTIONS 7             // number of functions implemented
#define MAX_FORMATS 2               // number of image formats supported
#define CHANNELS 3                           // color channels on input
#define DEFAULT_OUT_FILE "out/output.png"    // default output file path

#define DEFAULT_USAGE "<image.png> -f <function>"         // default console usage
#define DEFAULT_OUT "-o <out.[bmp/png/...]>"              // default console output usage
#define DEFAULT_ARG "repict"

typedef enum {
    DEFAULT = 0,        // do nothing
    RESIZE = 1,         // resize image
    GAUSS = 2,          // apply gaussian filter
    FAST = 3,           // apply fast average blur
    BW = 4,             // apply black and white filter
    CANNY = 5,          // find edges
    CUSTOM_KER = 6      // apply custom kernel from file
} FUNCTION;

typedef enum {NONE, F_BMP, F_PNG} FORMAT; // supported I/O formats

typedef char *file_path_t;
typedef pixel_t * (*RepictFunction) (pixel_t *data, int argc, char **argv);

typedef struct {
    FUNCTION func;          // function identifier
    RepictFunction exec;    // function call (RepictFunction signature)
    unsigned int arg_min;   // expected argument count
    unsigned int arg_max;   // max arguments, some optional
    const char *usage;      // usage string, printed to console
    const char *name;
} function_t;

typedef struct {
    FORMAT format;          // format identifier
    char *ext;              // format extension
} format_t;

static const format_t DEFAULT_OUT_FORMAT = {F_PNG, "png"};

bool verbose;           // print verbose
bool function_def;      // make sure a function has been given
bool out_def;           // output has been specified
bool usage_req;         // print usage on error

file_path_t file_in;    // file to read from
file_path_t file_out;   // file to output to (default to DEFAULT_OUT)
char *file_type;        // used for infile type and outfile type

function_t function;    // function to be executed
int f_argc;             // internal arg count
char **f_argv;          // internal args

pixel_t *pixels;        // image data
pixel_t *pixels_out;    // output image data
int32_t width, height;  // dimensionss
int bpp;                // bytes per pixel for png

int channels_out = CHANNELS;       // channels written to output image, default to same as input


/* Get file format from input path */
FORMAT match_file_format(char *file);

/* Get function from input */
function_t *match_function(char *in);

/* Handle -f function select and args, call function */
bool handle_function(const int argc, char **argv);

/* Open file for use */
bool open_file(char *file, FORMAT format);

/* Write new pixels to file */
bool write_file(char *file, FORMAT format);

/* Handle flags */
bool handle_flags(const int argc, char **argv);

/* Print commandline usage of repict for specific function */
void print_usage_f(function_t f, bool omit_out);

/* Print commandline usage of repict generally */
void print_usage(bool omit_out);

/* Print a verbose only message */
void print_verbose(const char *lbl, const char *msg);

void init_repict();


// ========== FUNCTION SETUP AND METHOD SIGNATURES ==========
/* Just return data */
pixel_t *default_op(pixel_t *data, int argc, char **argv);

/* Resize to width: args[0] height: args[1] */
pixel_t *resize_op(pixel_t *data, int argc, char **argv);

/* Apply gaussian filter kernel size: arg[0] (2n + 1) */
pixel_t *gauss_op(pixel_t *data, int argc, char **argv);

/* Apply fast blur filter kernel size: arg[0] (2n + 1) */
pixel_t *average_op(pixel_t *data, int argc, char **argv);

/* Apply B&W filter */
pixel_t *bw_op(pixel_t *data, int argc, char **argv);

/* Find edges */
pixel_t *canny_op(pixel_t *data, int argc, char **argv);

/* Apply custom kernal to image from input kernel.txt file */
pixel_t *custom_kernel_op(pixel_t *data, int argc, char **argv);

/* Print help menu */
void print_help();

// TODO read from file
/* Implemented function parameter setup */
const function_t functions[MAX_FUNCTIONS] = {
    {
        DEFAULT,
        default_op,
        0,
        0,
        "",
        "def"
    },
    {
        RESIZE,
        resize_op,
        2, // need width and height
        3, // optional select color correction mode
        "<width> <height> <optl: color mode>",
        "resize"
    },
    {
        GAUSS,
        gauss_op,
        1,
        2,
        "<kernel size> <optl: sigma>",
        "gauss"
    },
    {
        FAST,
        average_op,
        1,
        1,
        "<kernel size>",
        "average"
    },
    {
        BW,
        bw_op,
        0,
        0,
        "",
        "bw"
    },
    {
        CANNY,
        canny_op,
        0,
        3,
        "<optl: gauss size> <optl: min thresh> <optl: max thresh>",
        "canny"
    },
    {
        CUSTOM_KER,
        custom_kernel_op,
        1,
        1,
        "<kernel file>",
        "kernel"
    }
};

const format_t formats[MAX_FORMATS] = {
    {
        F_PNG,
        "png"
    },
    {
        F_BMP,
        "bmp"
    }
};
// ==========================================================