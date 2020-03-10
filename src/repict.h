/**
 * 
 * REPICT - library of image manipulation functions
 *
 * ############## Notes ############## #############################################
 * Repict will do all of the allocation for output images and store processing 
 * results, which can be retrieved using repict_get_result() or 
 * repict_get_result_as_copy()
 * 
 * Multiple repict functions can be called in a row after setting the source image
 * and dimensions, and the results of each function/filter will carry over to the
 * next as well as current channels working in the image (can change depending on
 * the function/filter).
 * 
 * I/O currently must be handled externally - repict only deals with pixel matrices
 * #################################################################################
 * 
 * 
 * ############## How To Use #######################################################
 * ====== BASIC : ======
 * repict_set_source(*input, width, height, channels);  --> must be set before use
 * ...
 * repict_bw(args, ... );                               --> pass filter arguments
 * ...
 * pixel_t *result = repict_get_result();               --> pointer to working image
 * repict_clean()                                       --> clean internal memory
 * 
 * 
 * ====== MORE : ======
 * repict_get_working_channels]();  --> get working image channels
 * repict_get_result_as_copy();     --> get copy of working image
 * 
 * 
 * ====== UTILITY : ======
 * repict_alloc_image(width, height, channels)
 * 
 * #################################################################################
 * 
 * 
 * 
 * TODO:
 * - handle I/O
 * - canny
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
#define M_PI 3.1415926535

#define KERNEL_MAX 100
#define PIXEL_MAX 255

#define ERROR_MSG "Error:"

#define KERNEL(c) (2*c + 1)

typedef unsigned char pixel_t;      // 8-bit format for a pixel channel type
typedef float kernel_t;             // kernel unit type

size_t kernel_n         = 1;        // dimensions of kernel (kernel dim -> 2*kernel_n + 1)
size_t kernel_n_store   = 0;        // store old dimensions to minimize realloc
kernel_t *kernel        = NULL;     // pointer to kernel matrix
pixel_t *working_img    = NULL;     // current working copy of output image

static unsigned int r_channels   = 3;    // channels of source image (can be changed)
static int32_t r_width           = 0;    // dimensions of source image (can be changed)
static int32_t r_height          = 0;    // ...


// ======== Internal functions ========
static void m_set_kernel_size(int c, bool raw);
static void m_generate_kernel_space(int c);
static void m_convolve(void);                                   // internal convolution using kernel
static void m_alloc_working(int32_t w, int32_t h, int bpp);     // allocate the working image
static void m_swap_working(pixel_t *output);                    // place output in working image

// ======== Repict functions ========
int repict_gaussian_filter(int rad, float sig);             // compute gaussian
int repict_convolve(const kernel_t *ker);                   // convolution with input kernel (doesn't change internal)
int repict_bw(bool keep);                                   // apply B&W filter, keep all channels or output to 1 channel

void repict_set_source(pixel_t *in, const int32_t w, const int32_t h, 
        const unsigned int c, bool copy);                                                  // set source image properties
pixel_t *repict_get_result(void);                                               // get pointer to working image
pixel_t *repict_get_result_as_copy(void);                                       // get pointer to copy of working image
int repict_get_working_channels(void);                                          // get number of channels in working image
pixel_t *repict_copy_image(const pixel_t *in, int32_t w, int32_t h, int bpp);   // copy an image
pixel_t *repict_alloc_image(int32_t w, int32_t h, int bpp);                     // malloc image of dimensions
void repict_clean(void);                                                        // free internal memory

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
        p = (pixel_t *) malloc(bpp * r_width * r_height);
    }
    else {
        p = (pixel_t *) realloc(working_img, bpp * r_width * r_height);
    }
    if (p == NULL) {
        error("working image (re)allocation failure");
        return;
    }
    working_img = p;
}

static void m_swap_working(pixel_t *output) {
    //working_img = repict_copy_image(output, width, height, channels);
    working_img = output;
}



pixel_t *repict_alloc_image(int32_t w, int32_t h, int bpp) {
    pixel_t *p;
    p = (pixel_t *) malloc(bpp * r_width * r_height);
    if (p == NULL) {
        error("new image allocation failure");
        return NULL;
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

void repict_set_source(pixel_t *in, const int32_t w, const int32_t h, const unsigned int c, bool copy) {
    if (w < 1 || h < 1) {
        error("dimensions must be postitive non-zero");
        return;
    }
    if (in == NULL) {
        error("input image null");
    }
    if (c < 1 || c > 4) {
        error("channels must be 1-4");
        return;
    }
    r_width = w;
    r_height = h;
    r_channels = c;
    if (copy) { // copy input image instead of just setting the pointer
        working_img = repict_copy_image(in, w, h, c);
    }
    else {
        working_img = in;
    }
}

pixel_t *repict_get_result(void) {
    return working_img;
}

pixel_t *repict_get_result_as_copy(void) {
    return repict_copy_image(working_img, r_width, r_height, r_channels);
}

int repict_get_working_channels(void) {
    return r_channels;
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
 * Convert image to black and white.  keep = true: image channels remain the same
 * keep = false: image downgrades to a single channel
*/
int repict_bw(bool keep) {
    if (working_img == NULL) {
        error("image not initialized");
        return -1;
    }

    pixel_t *new_img;
    if (keep) {
        new_img = repict_alloc_image(r_width, r_height, r_channels);
    }
    else {
        new_img = repict_alloc_image(r_width, r_height, 1);
    }

    int32_t range = r_width * r_height * r_channels;
    unsigned int single_channel = 0;
    for (unsigned int i = 0; i < range; i += r_channels) {
        int32_t avg = 0;
        for (unsigned int k = 0; k < r_channels; k++) {
            avg += working_img[i + k];
        }
        avg /= r_channels;

        if (keep) { // write average to all channels
            for (unsigned int k = 0; k < r_channels; k++) {
                new_img[i + k] = avg;
            }
        }
        else { // single channel output
            new_img[single_channel] = avg;
            single_channel++;
        }
    }

    m_swap_working(new_img);
    if (! keep) {
        r_channels = 1;
    }
    return 1;
}

int repict_convolve(const kernel_t *ker) {

    return 1;
}


/* Outputs a single channel image to out_data */
int repict_gaussian_filter(int rad, float sig) {
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