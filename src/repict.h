/**
 * Repict - library of image manipulation functions
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

#define GAUSS_SIZE_DEFAULT 2
#define GAUSS_SIG_DEFAULT 1.4
#define GAUSS_LOW_THRESHOLD 2.5
#define GAUSS_HIGH_THRESHOLD 7.5
#define KERNEL_MAX 100
#define ERROR_MSG "Error:"

#define M_PI 3.1415926535
#define MAX_VAL 255

#define KERNEL(c) (2*c + 1)

typedef unsigned char pixel_t;
typedef float kernel_t;

size_t kernel_n = 1;            // dimensions of kernel (kernel dim -> 2*kernel_n + 1)
size_t kernel_n_store = 0;      // store old dimensions to minimize realloc
kernel_t *kernel = NULL;        // pointer to kernel matrix

unsigned int channels = 3;


// ======== Internal functions ========
static void m_set_kernel_size(int c, bool raw);
static void m_generate_kernel_space(int c);
static void m_convolve();                           // internal convolution using kernel


// ======== Repict functions ========
int repict_gaussian_filter(const pixel_t *in_data, pixel_t *out_data, int32_t width, int32_t height, int r, float sig);
int repict_convolve(const pixel_t *in_data, pixel_t *out_data, int32_t width, int32_t height);
int repict_bw(const pixel_t *in_data, pixel_t *outdata, int32_t width, int32_t height, bool keep);

pixel_t *repict_alloc_image(int32_t width, int32_t height, int bpp);
void repict_set_channels(const unsigned int c);


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
    if (c < 0 || c > raw ? KERNEL_MAX : KERNEL_MAX / 2) {
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
        malloc(size_k * sizeof(kernel_t));
    }
    else {
        realloc(kernel, size_k * sizeof(kernel_t));
    }
    kernel_n_store = kernel_n;
}


pixel_t *repict_alloc_image(int32_t width, int32_t height, int bpp) {
    pixel_t *p = (pixel_t *) malloc(bpp * width * height);
    if (p == NULL) {
        error("new image allocation failure");
    }
}


void repict_set_channels(const unsigned int c) {
    if (c < 1 || c > 4) {
        return;
    }
    channels = c;
}

/**
 * Convert image to black and white
 * in_data can have variable channels, out_data is one channel only unless keep is true
*/
int repict_bw(const pixel_t *in_data, pixel_t *out_data, int32_t width, int32_t height, bool keep) {
    if (in_data == NULL) {
        error("BW received bad image data");
        return -1;
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

    return 1;
}

int repict_convolve(const pixel_t *in_data, pixel_t *out_data, int32_t width, int32_t height) {


    return 1;
}


int repict_gaussian_filter(const pixel_t *in_data, pixel_t *out_data, int32_t width, int32_t height, 
        int r, float sig) {
    m_generate_kernel_space(r);
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