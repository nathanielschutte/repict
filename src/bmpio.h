#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/**
 * http://www.vbforums.com/showthread.php?t=261522
 * http://en.wikipedia.org/wiki/BMP_file_format
 * 
 * Manage reading and writing BMPs.  Use STB library for format conversion
*/

typedef struct {
    uint8_t magic[2];
} bmpfile_magic_t;

// use BMP format wiki
typedef struct {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t nplanes; // color planes = 1
    uint16_t nbytes; // bytes per pixel
    uint32_t compress; // compress type
    uint32_t bmp_size; // size of bmp in bytes
    int32_t ppm_x; // pixels per meter x-axis
    int32_t ppm_y; // pixels per meter y-axis
    uint32_t ncolors; // number of colors
    uint32_t ncolors_imp; // number of colors important
} bitmap_info_header_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} rgb_t;

typedef short int pixel_t;