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
 * 
 * There is also a persistent kernel stored in repict that can be set 
 * #################################################################################
 * 
 * 
 * ############## How To Use #######################################################
 * ====== BASIC : ======
 * repict_set_source(*input, width, height, channels);  --> must be set before use
 * ...                                                      ...
 * repict_bw(args, ... );                               --> pass filter arguments
 * ...                                                      ...
 * pixel_t *result = repict_get_result();               --> pointer to working image
 * repict_clean()                                       --> clean internal memory
 * 
 * 
 * ====== MORE : ======
 * repict_get_working_channels]();                      --> get working image channels
 * repict_get_result_as_copy();                         --> get copy of working image
 * 
 * 
 * ====== UTILITY : ======
 * repict_alloc_image(width, height, channels)          --> alloc image sized chunk
 * repict_copy_image(image, width, height, channels)    --> return copy of image
 * 
 * #################################################################################
 * 
 * 
 * 
 * TODO:
 * - handle I/O
 * - canny
 * - optimize convolution
 * - parse txt into kernel_t array
*/

#ifndef REPICT_IMPLEMENTATION
#define REPICT_IMPLEMENTATION

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef M_PI // make sure to define pi
#define M_PI 3.1415926535
#endif

// algorithm default constants
#define GAUSS_SIG_DEFAULT 0.8f
#define GAUSS_LOW_THRESHOLD 2.5f
#define GAUSS_HIGH_THRESHOLD 7.5f
#define GAUSS_CUT_OFF 0.005f
#define MAGNITUDE_SCALE 100.0f
#define MAGNITUDE_LIMIT 1000.0f
#define MAGNITUDE_MAX(s, l) (int)(s * l)

// data constants
#define KERNEL_MAX 100
#define PIXEL_MAX 255
#define TRASH_VALUE 120

// UI
#define ERROR_MSG "Error:"

// kernel edge method
#define REPICT_EDGE_ALL 0
#define REPICT_EDGE_TRASH 1

#ifndef REPICT_EDGE_STRATEGY
#define REPICT_EDGE_STRATEGY REPICT_EDGE_ALL
#endif


typedef unsigned char pixel_t;      // 8-bit format for a pixel channel type
typedef float kernel_t;             // kernel unit type

// internal store
size_t kernel_n         = 1;        // dimensions of kernel (kernel dim -> 2*kernel_n + 1)
size_t kernel_n_store   = 0;        // store old dimensions to minimize realloc
kernel_t *kernel        = NULL;     // pointer to kernel matrix
pixel_t *working_img    = NULL;     // current working copy of output image

// image dimensions
static unsigned int r_channels   = 3;    // channels of source image (can be changed)
static int32_t r_width           = 0;    // dimensions of source image (can be changed)
static int32_t r_height          = 0;    // ...


// ======== Internal functions ========
static void m_set_kernel_size(int c);
static kernel_t *m_generate_kernel_space(int c);
static void m_generate_kernel_internal(int c);
static void m_convolve(pixel_t *input, pixel_t *output);                                // internal convolution using kernel, result -> output
static void m_convolve_kernel(pixel_t *input, pixel_t *output, kernel_t *ker, int kn);  // convolution using specified kernel, result -> output
static void m_alloc_working(int32_t w, int32_t h, int bpp);     // allocate the working image
static void m_swap_working(pixel_t *output);                    // place output in working image

// ======== Repict functions ========
int repict_convolve(kernel_t *ker, int kn);                     // convolution with input kernel (doesn't change internal)
int repict_gaussian_filter(float sig, int n, bool keep);        // compute gaussian
int repict_bw(bool keep);                                       // apply B&W filter, keep all channels or output to 1 channel
int repict_average_filter(float width, int n, bool keep);

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
static float gaussian(float x, float y, float sig2);


static void m_set_kernel_size(int c) {
    if (c < 0 || c > KERNEL_MAX || (c % 2 == 0)) {
        error("kernel cannot be set to this size");
        return;
    }
    kernel_n = c;
}

static kernel_t *m_generate_kernel_space(int c) {
    if (c < 0 || c > KERNEL_MAX || (c % 2 == 0)) {
        error("kernel cannot be set to this size");
        return;
    }
    kernel_t *k;
    int size_k = c * c;
    k = (kernel_t *) malloc(size_k * sizeof(kernel_t));
    return k;
}

static void m_generate_kernel_internal(int c) {
    m_set_kernel_size(c);
    if (kernel_n == kernel_n_store) {
        return;
    }
    int size_k = kernel_n * kernel_n;
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

    // working_img holds obsolete data, free
    free(working_img);

    // output is the allocation from a repict function that is current
    working_img = output;
}

/* Convolution of working image and kernel, result placed in */
static void m_convolve(pixel_t *input, pixel_t *output) {
    if (kernel == NULL) {
        error("no kernel for convolution");
        return;
    }
    m_convolve_kernel(input, output, kernel, kernel_n);
}

/* Convolution using kernel 'ker' that ignores edges of size (kernel width/2) */
static void m_convolve_kernel(pixel_t *input, pixel_t *output, kernel_t *ker, int kn) {
    if (r_width < kn || r_height < kn) {
        error("cannot perform convolution - image too small for kernel size");
        return;
    }
    if (kn % 2 == 0) {
        error("kernel width must be odd");
        return;
    }
    if (output == NULL) {
        error("no output image provided for convolution");
        return;
    }
    

    // convolution, unoptimized
    const int khl = kn / 2;
    float ksum = 0;
    for (unsigned int i = 0; i < kn*kn; i++) {
        ksum += ker[i];
    }
    int stride = r_width * r_channels;
    int xn = stride - khl;
    int yn = r_height * r_channels - khl;

    for (unsigned int x = 0; x < r_width; x += r_channels) {
        for (unsigned int y = 0; y < r_height; y += r_channels) {

            if ((x >= khl && x < xn && y >= khl && y < yn) || REPICT_EDGE_STRATEGY == REPICT_EDGE_ALL) {
                float acc = 0.0;
                int c = 0;
                for (int i = -khl; i <= khl; i++) {
                    for (int j = -khl; j <= khl; j++) {
                        // for this strategy, kernel should operate even on edge pixels
                        if (REPICT_EDGE_STRATEGY == REPICT_EDGE_ALL) {
                            // don't overstep edges
                            if (y + j < 0 || y + j >= r_height || x + i < 0 || x + i >= r_width) {
                                continue;
                            }
                        }
                        acc += input[(y - j) * stride + x - i] * ker[c];
                        c++;
                    }
                }
                acc /= ksum;
                output[y * stride + x] = (pixel_t) acc;
            }
            else {
                switch (REPICT_EDGE_STRATEGY) {
                    case REPICT_EDGE_ALL:
                    output[y * stride + x] = (pixel_t) TRASH_VALUE;
                    break;

                    default:
                    output[y * stride + x] = (pixel_t) TRASH_VALUE;
                }
            }
        }
    }
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


/* Convolve using outside kernel on source image */
int repict_convolve(kernel_t *ker, int kn) {
    if (working_img == NULL) {
        error("image not initialized");
        return -1;
    }
    pixel_t *new_img = repict_alloc_image(r_width, r_height, r_channels);
    m_convolve_kernel(working_img, new_img, ker, kn);
    m_swap_working(new_img);
    return 1;
}


/* keep: all channels vs 1 channel.  sig = gaussian values and radius, n = convolutions */
int repict_gaussian_filter(float sig, int n, bool keep) {
    if (working_img == NULL) {
        error("image not initialized");
        return -1;
    }

    if (! keep) {
        repict_bw(false);
    }
    pixel_t *new_img = repict_alloc_image(r_width, r_height, r_channels);

    float sigma; // use this sigma
    if (sig < 0)
        sigma = GAUSS_SIG_DEFAULT;
    else
        sigma = sig;

    const int kw = (2 * (int)(2 * sigma)) + 3; // kernel dimension appropriate for value of sigma
    kernel_t *gauss_ker = m_generate_kernel_space(kw);

    const float sig2 = sigma * sigma;
    const float mean = (float) floor(kw / 2.0) + 1;

    // generate kernel values for gaussian filter, function of sigma
    size_t c = 0;
    for (unsigned int i = 1; i <= kw; i++) {
        for (unsigned int j = 1; j <= kw; j++) {
            gauss_ker[c] = (kernel_t) gaussian(i - mean, j - mean, sig2) / (2 * M_PI * sig2);
            c++;
        }
    }

    // convolution performed n times
    m_convolve_kernel(working_img, new_img, gauss_ker, kw);
    pixel_t *temp_img;
    if (n > 1) {
        temp_img = repict_alloc_image(r_width, r_height, r_channels);
    }
    for (unsigned int i = 1; i < n; i++) {
        printf("convolution #%d\n", i+1);
        m_convolve_kernel(new_img, temp_img, gauss_ker, kw);
        new_img = temp_img;
    }
    m_swap_working(new_img);
    return 1;
}


/* width = kernel width, n = convolutions, keep = all channels vs. 1 channel */
int repict_average_filter(float width, int n, bool keep) {
    if (working_img == NULL) {
        error("image not initialized");
        return -1;
    }
    if (! keep) {
        repict_bw(false);
    }
    pixel_t *new_img = repict_alloc_image(r_width, r_height, r_channels);
    kernel_t *avg_ker = m_generate_kernel_space(width);

    // generate kernel values for average filter
    size_t c = 0;
    for (unsigned int i = 1; i <= width; i++) {
        for (unsigned int j = 1; j <= width; j++) {
            avg_ker[c] = (kernel_t) 1;
        }
    }
    for (unsigned int i = 0; i < n; i++) {
        m_convolve_kernel(working_img, new_img, avg_ker, width);
    }
    m_swap_working(new_img);
    if (! keep) {
        r_channels = 1;
    }
    return 1;
}


static void error(const char *err) {
    printf(ERROR_MSG);
    printf(" ");
    printf(err);
    printf("\n");
}

/* x=(i-(k+1)), y=(j-(k+1)) sig2=sig^2 */
static float gaussian(float x, float y, float sig2) {
    return (float) exp(-(x*x + y*y) / (2.0 * sig2));
}


#endif