#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

#include "jpeglib.h"
#include "jerror.h"
#include "zlib.h"
#include "png.h"

#define RGB_SIZE 3
/* #define FULL_WHITE (255*1000) */
#define FULL_WHITE (255*10000)

enum { R=0, G=1, B=2 };

static void usage(FILE *output) {
    fputs("Usage: dejpeg [-white=percents] iname.jpeg outname.png\n",
          output);
}

static void about(FILE *output) {
    struct jpeg_common_struct cinfo;
    struct jpeg_error_mgr err;
    char buf[JMSG_LENGTH_MAX];
    (void)memset(&err, 0, sizeof(err));
    cinfo.err = jpeg_std_error(&err);

    buf[0] = '\0';
    cinfo.err->msg_code = JMSG_VERSION;
    cinfo.err->format_message(&cinfo, buf);

    fprintf(output, "\nIJG JPEG LIBRARY version %s\n", buf);
    fprintf(output, "The Independent JPEG Group\'s JPEG software\n");

    buf[0] = '\0';
    cinfo.err->msg_code = JMSG_COPYRIGHT;
    cinfo.err->format_message(&cinfo, buf);

    fprintf(output, "%s\n", buf);

    fprintf(output,
"\nLIBPNG version %s - PNG reference library\n"
"Copyright (c) 1995-2024 The PNG Reference Library Authors\n",
        PNG_LIBPNG_VER_STRING);

    fprintf(output,
"\nZLIB version %s - general purpose compression library\n"
"Copyright (c) 1995-2022 Jean-loup Gailly and Mark Adler\n",
        ZLIB_VERSION);
}

static const char *scanword(const char *string, const char *pattern) {
    int n = 0;
    while (pattern[n] != '\0' && pattern[n] == string[n]) ++n;
    return string + n;
}

int main(int argc, char *argv[]) {
    char *iname = NULL, *outname = NULL;
    FILE *infile, *outfile;
    int fd;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW input_row;
    png_bytep output_row;
    png_structp png_ptr;
    png_infop info_ptr;
    uint32_t black = FULL_WHITE / 2; /* 50% */
    unsigned bits_per_pixel = 1;
    size_t output_row_size;

    if (argc <= 1) {
        usage(stdout);
        about(stdout);
        return 0;
    }
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            const char *p, *option = argv[i] + 1;

            if (*option == 'w' && *(p = scanword(option, "white")) == '=') {
                char *end = NULL;
                double d = strtod(p+1, &end);
                if (0.0 >= d || d > 100.0) {
                    fprintf(stderr,
                        "White value should be between 0 and 100\n");
                    return 1;
                }
                if (end == NULL ||
                    !(end[0] == '\0' || (end[0] == '%' && end[1] == '\0'))) {
                    fprintf(stderr, "White value should be percentage\n");
                    return 1;
                }
		black = (uint32_t)(FULL_WHITE * d / 100);
            } else {
                fprintf(stderr, "Unknown option: %s\n", option);
                return 1;
            }
        } else if (!iname) {
            iname = argv[i];
        } else if (!outname) {
            outname = argv[i];
        } else {
            usage(stderr);
            return 1;
        }
    }
    if (!(iname && outname)) {
        usage(stderr);
        return 1;
    }
    infile = fopen(iname, "rb");
    if (NULL == infile) {
        perror(iname);
        return 1;
    }
    fd = open(outname, O_CREAT|O_WRONLY|O_BINARY|O_EXCL,
	      S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        perror(outname);
        return 1;
    }
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB; /* always require RGB */
    jpeg_calc_output_dimensions(&cinfo);

    jpeg_start_decompress(&cinfo);

    if (cinfo.output_components != RGB_SIZE) {
        fputs("Cannot convert input image to RGB\n", stderr);
        return 1;
    }

    printf("%u x %u\n", cinfo.output_width, cinfo.output_height);

    input_row = (JSAMPROW)malloc(
        cinfo.output_width * RGB_SIZE * sizeof(JSAMPLE));
    if (NULL == input_row) {
        fputs("Not enough memory\n", stderr);
        return 1;
    }
    output_row_size = (cinfo.output_width * bits_per_pixel + 7) / 8;
    output_row = (png_bytep)malloc(output_row_size * sizeof(png_byte));
    if (NULL == output_row) {
        fputs("Not enough memory\n", stderr);
        return 1;
    }
    outfile = fdopen(fd, "w");
    if (NULL == outfile) {
        perror(outname);
        return 1;
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		                      NULL, NULL, NULL);
    if (!png_ptr) {
	fputs("Unable to create PNG write struct\n", stderr);
	return 1;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	fputs("Unable to create PNG info struct\n", stderr);
	return 1;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
	fputs("Write error\n", stderr);
	return 1;
    }
    png_init_io(png_ptr, outfile);
    png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);
    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    png_set_IHDR(png_ptr, info_ptr,
	cinfo.output_width, cinfo.output_height, bits_per_pixel,
	PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
	PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_invert_mono(png_ptr);
    png_write_info(png_ptr, info_ptr);

    while (cinfo.output_scanline < cinfo.output_height &&
           jpeg_read_scanlines(&cinfo, &input_row, 1) == 1) {
	uint32_t x;

	memset(output_row, 0, output_row_size * sizeof(png_byte));
	for (x=0; x < cinfo.output_width; ++x) {
	    JSAMPROW sample = &(input_row[RGB_SIZE * x]);
	    uint32_t bright =
		/* 299*sample[R] + 587*sample[G] + 114*sample[B]; */
		2126 * sample[R] + 7152 * sample[G] + 722 * sample[B];
	    if (black >= bright) {
		output_row[x >> 3] |= 0x80 >> (x & 7);
	    }
	}
        png_write_row(png_ptr, output_row);
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;
}
