
#include <windows.h>
#include "bmpio.h"

#define MAX_FUNCTIONS 3         // number of functions implemented
#define DEFAULT_OUT_FILE "out.bmp"   // default output file path

#define DEFAULT_USAGE = "-f <function>"         // default console usage
#define DEFAULT_OUT "-o <out.[bmp/png/...]>"    // default console output usage

enum FUNCTION {
    DEFAULT = 0,        // do nothing
    RESIZE = 1,         // resize image
    CONVERT = 2,        // convert image
};

enum FORMAT {BMP, PNG}; // supported I/O formats

typedef char *file_path_t;
typedef pixel_t * (*RepictFunction) (pixel_t *data, char **args);

typedef struct {
    FUNCTION func;          // function identifier
    RepictFunction exec;    // function call (RepictFunction signature)
    unsigned int arg_min;   // expected argument count
    unsigned int arg_max;   // max arguments, some optional
    const char *usage;      // usage string, printed to console
} function_t;

const function_t functions[MAX_FUNCTIONS] = {
    {DEFAULT,
    default_op,
    

    }
};

typedef struct {
    FORMAT format;          // format identifier
    char *ext;              // format extension
} format_t;

const char *usage[MAX_FUNCTIONS] = {"", "<width> <height>", ""};

function_t func; // function to be performed
file_path_t file_in; // file to read from
file_path_t file_out; // file to output to (default to DEFAULT_OUT)


/* Print commandline usage of repict function */
void printUsage(const char *arg0, function_t f, bool omit_out);

pixel_t *default_op(pixel_t *data, char **args);