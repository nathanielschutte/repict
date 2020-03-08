
#include <windows.h>
#include "bmpio.h"

#define MAX_FUNCTIONS 3             // number of functions implemented
#define MAX_FORMATS 2               // number of image formats supported
#define DEFAULT_OUT_FILE "out.bmp"  // default output file path

#define DEFAULT_USAGE "-f <function>"         // default console usage
#define DEFAULT_OUT "-o <out.[bmp/png/...]>"    // default console output usage

typedef enum {
    DEFAULT = 0,        // do nothing
    RESIZE = 1,         // resize image
    CONVERT = 2,        // convert image
    HELP = 3,           // print help
} FUNCTION;

typedef enum {F_BMP, F_PNG} FORMAT; // supported I/O formats

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

bool verbose;           // print verbose
bool function_def;      // make sure a function has been given
bool usage_req;

file_path_t file_in;    // file to read from
file_path_t file_out;   // file to output to (default to DEFAULT_OUT)

function_t function;    // function to be executed
int f_argc;             // internal arg count
char **f_argv;          // internal args

pixel_t *pixels;        // image data
pixel_t *pixels_out;    // output image data
int32_t width, height;  // dimensions
int bpp;


/* Get file format from input path */
FORMAT match_file_format(char *file);

/* Get function from input */
function_t *match_function(char *in);

/* Handle -f function select and args, call function */
bool handle_function(const int argc, const char **argv);

/* Open file for use */
bool open_file(char *file, FORMAT format);

/* Write new pixels to file */
bool write_file(char *file, FORMAT format);

/* Handle flags */
bool handle_flags(const int argc, const char **argv);

/* Print commandline usage of repict for specific function */
void print_usage_f(const char *arg0, function_t f, bool omit_out);

/* Print commandline usage of repict generally */
void print_usage(const char *arg0, bool omit_out);

/* Print a verbose only message */
void print_verbose(const char *msg);


// ========== FUNCTION SETUP AND METHOD SIGNATURES ==========
/* Just return data */
pixel_t *default_op(pixel_t *data, int argc, char **argv);

/* Resize to width: args[0] height: args[1] */
pixel_t *resize_op(pixel_t *data, int argc, char **argv);

/* Print help menu */
pixel_t *print_help(pixel_t *data, int argc, char **argv);

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
        "<width> <height> <optional: color mode>",
        "resize"
    },
    {
        HELP,
        print_help,
        0,
        0,
        "",
        "help"
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