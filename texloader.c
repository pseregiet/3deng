#include "texloader.h"
#include "stb_image.h"
#include <string.h>
#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

int load_sg_image(const char *fn, sg_image *img)
{
    int w,h,n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(fn, &w, &h, &n, 4);
    if (!data) {
        fatalerror("stbi_load(%s) failed\n", fn);
        return -1;
    }

    printf("%s:%d:%d:%d\n", fn, w, h, n);

    *img = sg_make_image(&(sg_image_desc) {
        .width = w,
        .height = h,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .data.subimage[0][0] = {
            .ptr = data,
            .size = (w*h*4),
        },
    });

    stbi_image_free(data);
}

struct imgarray {
    unsigned char *data;
    int w;
    int h;
};

int load_sg_image_array(const char **fn, sg_image *img, int count)
{
    int w,h,n;
    int ret = -1;
    stbi_set_flip_vertically_on_load(true);

    if (!fn) {
        printf("load_sg_image_array fn is 0\n");
        return ret;
    }

    struct imgarray *darr = calloc(count, sizeof(*darr));
    if (!darr) {
        fatalerror("load_sg_image_array malloc failed\n");
        return ret;
    }

    for (int i = 0; i < count; ++i) {
        darr[i].data = stbi_load(fn[i], &w, &h, &n, 4);
        if (!darr[i].data) {
            fatalerror("stbi_load(%s) failed\n", fn);
            return ret;
        }
        darr[i].w = w;
        darr[i].h = h;
        printf("%s:%d:%d:%d\n", fn[i], w, h, n);
    }

    int lastw = darr[0].w;
    int lasth = darr[0].h;
    for (int i = 1; i < count; ++i) {
        if (darr[i].w != lastw || darr[i].h != lasth) {
            printf("load_sg_image_array images are of not equal size\n");
            goto freeimg;
        }
    }

    int newsize = lastw * lasth * 4 * count;
    unsigned char *newdata = malloc(newsize);
    if (!newdata) {
        fatalerror("load_sg_image_array malloc failed\n");
        goto freeimg;
    }

    for (int i = 0; i < count; ++i)
        memcpy(&newdata[lastw * lasth * 4 * i], darr[i].data, lastw * lasth * 4);

    *img = sg_make_image(&(sg_image_desc) {
        .type = SG_IMAGETYPE_ARRAY,
        .width = w,
        .height = h,
        .num_slices = count,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .data.subimage[0][0] = {
            .ptr = newdata,
            .size = newsize,
        },
    });
    
    ret = 0;
    free(newdata);
freeimg:
    for (int i = 0; i < count; ++i)
        stbi_image_free(darr[i].data);

    free(darr);

    return ret;
}

