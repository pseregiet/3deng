#include "texloader.h"
#include "fileops.h"
#include "qoi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

void make_sg_image_16f(float *ptr, int size, int *w, int *h, sg_image *img)
{
    int hi = 0;
    int wi = 0;

    if (size < 4096) {
        wi = size;
        hi = 1;
    }
    else {
        hi = size/4096;
        wi = 4096;
    }

    int bytesize = wi * hi * 8;
    *img = sg_make_image(&(sg_image_desc) {
        .width = wi,
        .height = hi,
        .pixel_format = SG_PIXELFORMAT_RG32F,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .data.subimage[0][0] = {
            .ptr = ptr,
            .size = bytesize,
        },
    });

    *w = wi;
    *h = hi;
}

int load_sg_image(const char *fn, sg_image *img)
{
    struct file f;
    qoi_desc d1;
    int ret = -1;

    if (openfile(&f, fn))
        return -1;

    if (qoi_parse_header(f.udata, f.usize, &d1)) {
        printf("qoi_parse_header failed %s\n", fn);
        goto freefile;
    }

    const int imgsize = d1.width * d1.height * 4;
    char *buf = malloc(imgsize);
    if (qoi_decode(f.udata, buf, f.usize, imgsize, &d1)) {
        printf("qoi_decode failed %s\n", fn);
        goto freebuf;
    }

    *img = sg_make_image(&(sg_image_desc) {
        .width = d1.width,
        .height = d1.height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .data.subimage[0][0] = {
            .ptr = buf,
            .size = imgsize,
        },
    });
    return 0;

freebuf:
    free(buf);
freefile:
    closefile(&f);
    return ret;
}

int load_sg_image_array(const char **fn, sg_image *img, int count)
{
    struct file f;
    qoi_desc d1;
    qoi_desc d2;

    if (openfile(&f, fn[0]))
        return -1;

    if (qoi_parse_header(f.udata, f.usize, &d1)) {
        printf("qoi_parse_header failed %s\n", fn[0]);
        goto freefile;
    }

    const int imgsize = d1.width * d1.height * 4;
    char *buf = malloc(imgsize * count);
    if (qoi_decode(f.udata, buf, f.usize, imgsize, &d1)) {
        printf("qoi_decode failed %s\n", fn[0]);
        goto freebuf;
    }
    closefile(&f);

    for (int i = 1; i < count; ++i) {
        if (openfile(&f, fn[i])) {
            free(buf);
            return -1;
        }

        if (qoi_parse_header(f.udata, f.usize, &d2)) {
            printf("qoi_parse_header failed %s\n", fn[i]);
            goto freebuf;
        }

        if (d1.width != d2.width || d1.height != d2.height) {
            printf("Image has bad image for array %s\n", fn[i]);
            goto freebuf;
        }

        if (qoi_decode(f.udata, &buf[imgsize * i], f.usize, imgsize, &d2)) {
            printf("qoi_decode failed %s\n", fn[0]);
            goto freebuf;
        }
        closefile(&f);
    }
    
    *img = sg_make_image_with_mipmaps(&(sg_image_desc) {
        .type = SG_IMAGETYPE_ARRAY,
        .width = d1.width,
        .height = d1.height,
        .num_slices = count,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST_MIPMAP_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .data.subimage[0][0] = {
            .ptr = buf,
            .size = imgsize * count,
        },
    });

    free(buf);
    return 0;
//-------------
freebuf:
    free(buf);
freefile:
    closefile(&f);
    return -1;
}

