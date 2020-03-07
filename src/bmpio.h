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

#define BMP_ID 0x4D42 // BMP filetype ID

typedef struct {
    uint8_t magic[2];
} bmpfile_magic_t;

typedef struct {
    uint32_t file_size; // size in bytes of bmp file
    uint16_t r1;
    uint16_t r2; // both reserved (0)
    uint32_t bmp_off; // offset in bytes from header to bitmap bits
} bmpfile_header_t;

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


/**
 * Read BMP file to pixel data, return pointer to pixel_t array (error handling within function)
*/
pixel_t *load_bmp(const char *filename, bitmap_info_header_t *bmp_info_head) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Couldn't open file");
        return NULL;
    }

    bmpfile_magic_t magic_info;
    if (fread(&magic_info, sizeof(bmpfile_magic_t), 1, file) != 1) {
        fclose(file);
        return NULL;
    }

    if (*((uint16_t *) magic_info.magic) != BMP_ID) {
        fprintf(stderr, "Not a BMP file: magic=%c%c\n", magic_info.magic[0], magic_info.magic[1]);
        fclose(file);
        return NULL;
    }

    bmpfile_header_t bmp_head;
    if (fread(&bmp_head, sizeof(bmpfile_header_t), 1, file) != 1) {
        fprintf(stderr, "Could not read BMP header");
        fclose(file);
        return NULL;
    }

    if (fread(bmp_info_head, sizeof(bitmap_info_header_t), 1, file) != 1) {
        fprintf(stderr, "Could not read BMP info header");
        fclose(file);
        return NULL;
    }

    if (bmp_info_head->compress != 0) {
        fprintf(stderr, "Compression not supported");
    }

    if (fseek(file, bmp_head.bmp_off, SEEK_SET)) {
        fprintf(stderr, "Failure while seeking start of BMP bits");
        fclose(file);
        return NULL;
    }

    pixel_t *bitmap = (pixel_t *) malloc(bmp_info_head->bmp_size * sizeof(pixel_t));

    if (bitmap == NULL) {
        fprintf(stderr, "Failure allocating memory for bitmap");
        fclose(file);
        return NULL;
    }

    size_t pad, count = 0;
    unsigned char c;
    pad = 4*ceil(bmp_info_head->nbytes*bmp_info_head->width/32.) - bmp_info_head->width;
    for (size_t i = 0; i < bmp_info_head->height; i++) {
        for (size_t j = 0; j < bmp_info_head->width; j++) {
            if (fread(&c, sizeof(unsigned char), 1, file) != 1) {
                fprintf(stderr, "Failure reading pixel data");
                fclose(file);
                return NULL;
            }
        }
        fseek(file, pad, SEEK_CUR);
    }

    fclose(file);
    return bitmap;
}

/**
 * Write pixel data out to a BMP, true on success
*/
bool save_bmp(const char *filename, const bitmap_info_header_t *bmp_ih, const pixel_t *data) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        return false;
    }
    bmpfile_magic_t magic_info = {{0x42, 0x4D}};
    if (fwrite(&magic_info, sizeof(bmpfile_magic_t), 1, file) != 1) {
        fclose(file);
        return false;
    }
    const uint32_t offset = sizeof(bmpfile_magic_t) + sizeof(bmpfile_header_t) + sizeof(bitmap_info_header_t) +
            ((1U << bmp_ih->nbytes) * 4);
    bmpfile_header_t bmp_h;
    bmp_h.file_size = offset + bmp_ih->bmp_size;
    bmp_h.r1 = 0;
    bmp_h.r2 = 0;
    bmp_h.bmp_off = offset;

    // write initial header
    if (fwrite(&bmp_h, sizeof(bmpfile_header_t), 1, file) != 1) {
        fclose(file);
        return false;
    }

    // write info header
    if (fwrite(bmp_ih, sizeof(bitmap_info_header_t), 1, file) != 1) {
        fclose(file);
        return false;
    }

    // generate color palette for 8-bit per RGB channel
    for (size_t i = 0; i < (1U << bmp_ih->bmp_size); i++) {
        const rgb_t color = {(uint8_t) i, (uint8_t) i, (uint8_t) i};
        if (fwrite(&color, sizeof(rgb_t), 1, file) != 1) {
            fclose(file);
            return false;
        }
    }

    size_t pad = 4*ceil(bmp_ih->bmp_size*bmp_ih->width/32.) - bmp_ih->width;
    unsigned char c;
    for (size_t i = 0; i < bmp_ih->height; i++) {
        for (size_t j = 0; j < bmp_ih->width; j++) {
            c = (unsigned char) data[j + bmp_ih->width*i];
            if (fwrite(&c, sizeof(char), 1, file) != 1) {
                fclose(file);
                return false;
            }
        }

        // padding
        c = 0;
        for(size_t j = 0; j < pad; j++) {
            if(fwrite(&c, sizeof(char), 1, file) != 1) {
                fclose(file);
                return true;
            }
        }
    }

    fclose(file);
    return true;
}