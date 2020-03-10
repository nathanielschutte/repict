/**
 * Repict - library of image manipulation functions
 *
 * ========== Notes ========== 
 * Repict will do all of the allocation for output images.  All image processing
 * functions take an out_data pointer, which is where repict will allocate the output
 * image data.
 * 
 * ========== How To Use ==========
 * 
 * === BASIC : ===
 * repict_set_dimensions(width, height);    --> must be set before use
 * repict_set_channels(channels);           --> default 3
 * ...
 * repict_bw(in_data ... );                 --> pass input image and arguments
 * ...
 * repict_clean()                           --> clean internal memory
 * 
 * 
 * === CHAIN FUNCTIONS : ===
 * repict_keep_working(true);               --> will save copy of every output
 * ...
 * repict_bw(NULL ... );                    --> set input image to NULL; repict will use working copy
 * ...
 * repict_keep_working(false);              --> keep off if not using to minimize overhead of copy
 * 
*/

#ifndef REPICT_IMPLEMENTATION
#define REPICT_IMPLEMENTATION

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define GAUSS_SIZE_DEFAULT 2f
#define GAUSS_SIG_DEFAULT 1.4f
#define GAUSS_LOW_THRESHOLD 2.5f
#define GAUSS_HIGH_THRESHOLD 7.5f
#define KERNEL_MAX 100
#define ERROR_MSG "Error:"

#define M_PI 3.1415926535
#define MAX_PIXEL 255

#define KERNEL(c) (2*c + 1)

typedef unsigned char pixel_t;  // 8-bit format for a pixel channel type
typedef float kernel_t;         // kernel unit type

size_t kernel_n         = 1;        // dimensions of kernel (kernel dim -> 2*kernel_n + 1)
size_t kernel_n_store   = 0;        // store old dimensions to minimize realloc
kernel_t *kernel        = NULL;     // pointer to kernel matrix
pixel_t *working_img    = NULL;     // current working copy of output image
bool keep_working       = false;    // copy result of each function to working copy

unsigned int channels   = 3;    // channels of in_data
int32_t width           = 0;    // dimensions of in_data image
int32_t height          = 0;


// ======== Internal functions ========
static void m_set_kernel_size(int c, bool raw);
static void m_generate_kernel_space(int c);
static void m_convolve();                                       // internal convolution using kernel
static void m_alloc_working(int32_t w, int32_t h, int bpp);     // allocate the working image
static void m_make_copy();

// ======== Repict functions ========
int repict_gaussian_filter(const pixel_t *in_data, pixel_t *out_data, int rad, float sig);
int repict_convolve(const pixel_t *in_data, pixel_t *out_data);
int repict_bw(pixel_t *in_data, pixel_t *out_data, bool keep);

void repict_set_source(pixel_t *in, const int32_t width, const int32_t height, const unsigned int c);
void repict_keep_working(bool working);
pixel_t *repict_copy_image(const pixel_t *in, int32_t w, int32_t h, int bpp);
pixel_t *repict_alloc_image(int32_t w, int32_t h, int bpp);
void repict_clean(void);

// ======== Utility functions ========
static void error(const char *err);

/*
 * Kernel set table:
 * c   kernel_n
 * 1 =   3
 * 2 =   5
 * 3 =   7
 * 4 =   9
 * 5 =   11
 * ...
*/
static void m_set_kernel_size(int c, bool raw) {
    if (c < 0 || c > (raw ? KERNEL_MAX : KERNEL_MAX / 2)) {
        error("kernel cannot be set to this size");
        return;
        }
    if (raw) {
        kernel_n = c;
    }
    else {
        kernel_n = KERNEL(c);
    }
}


static void m_generate_kernel_space(int c) {
    m_set_kernel_size(c, true);
    if (kernel_n == kernel_n_store) {
        return;
    }
    int size_k = KERNEL(kernel_n) * KERNEL(kernel_n);
    if (kernel == NULL) {
        kernel = (kernel_t *) malloc(size_k * sizeof(kernel_t));
    }
    else {
        kernel = (kernel_t *) realloc(kernel, size_k * sizeof(kernel_t));
    }
    kernel_n_store = kernel_n;
}


static void m_alloc_working(int32_t w, int32_t h, int bpp) {
    pixel_t *p;
    if (working_img == NULL) {
        p = (pixel_t *) malloc(bpp * width * height);
    }
    else {
        p = (pixel_t *) realloc(working_img, bpp * width * height);
    }
    if (p == NULL) {
        error("working image (re)allocation failure");
        return;
    }
    working_img = p;
}

static void m_make_copy(pixel_t *output) {
    if (keep_working) {
        repict_copy_image(output, width, height, channels);
    }
}

pixel_t *repict_alloc_image(int32_t w, int32_t h, int bpp) {
    pixel_t *p;
    p = (pixel_t *) malloc(bpp * width * height);
    if (p == NULL) {
        error("new image allocation failure");
        return;
    }
    return p;
}

pixel_t *repict_copy_image(const pixel_t *in, int32_t w, int32_t h, int bpp) {
    pixel_t *new_img = repict_alloc_image(w, h, bpp);
    int64_t range = w * h * bpp;
    for (unsigned int i = 0; i < range; i++) {
        new_img[i] = in[i];
    }
    return new_img;
}


void repict_set_channels(const unsigned int c) {
    if (c < 1 || c > 4) {
        error("channels must be 1-4");
        return;
    }
    channels = c;
}


void repict_set_dimensions(const int32_t w, const int32_t h) {
    if (w < 1 || h < 1) {
        error("dimensions must be postitive non-zero");
        return;
    }
    width = w;
    height = h;
}

void repict_keep_working(bool working) {
    if (working) {
        if (! keep_working) {
            m_alloc_working(width, height, channels);
        }
    }
    keep_working = working;
}

void repict_clean(void) {
    if (kernel != NULL) {
        free(kernel);
        kernel = NULL;
    }
    if (working_img != NULL) {
        free(working_img);
        working_img = NULL;
    }
}

/**
 * Convert image to black and white
 * in_data can have variable channels, out_data is one channel only unless keep is true
*/
int repict_bw(pixel_t *in_data, pixel_t *out_data, bool keep) {
    if (keep_working && working_img != NULL) {
        in_data = working_img;
    }
    if (in_data == NULL) {
        error("BW received bad image data");
        return -1;
    }
    if (keep) {
        repict_alloc_image(width, height, 1);
    }
    int32_t range = width * height * channels;
    unsigned int single_channel = 0;
    for (unsigned int i = 0; i < range; i += channels) {
        int32_t avg = 0;
        for (unsigned int k = 0; k < channels; k++) {
            avg += in_data[i + k];
        }
        avg /= channels;

        if (keep) { // write average to all channels
            for (unsigned int k = 0; k < channels; k++) {
                out_data[i + k] = avg;
            }
        }
        else { // single channel output
            out_data[single_channel] = avg;
            single_channel++;
        }
    }

    m_make_copy();
    return 1;
}

int repict_convolve(const pixel_t *in_data, pixel_t *out_data) {


    return 1;
}


/* Outputs a single channel image to out_data */
int repict_gaussian_filter(const pixel_t *in_data, pixel_t *out_data, int rad, float sig) {
    m_generate_kernel_space(rad);

    float sigma;
    if (sig < 0) {
        sigma = GAUSS_SIG_DEFAULT;
    }
    else {
        sigma = sig;
    }

    float sig2 = sig * sig;
    int kns = KERNEL(kernel_n);


    return 1;
}


static void error(const char *err) {
    printf(ERROR_MSG);
    printf(" ");
    printf(err);
    printf("\n");
}



#endif