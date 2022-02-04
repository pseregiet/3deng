#ifndef __texloader_h
#define __texloader_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

int load_sg_image(const char *fn, sg_image *img);
int load_sg_image_array(const char **fn, sg_image *img, int count);
void make_sg_image_16f(float *ptr, int size, int *w, int *h, sg_image *img);

#endif
