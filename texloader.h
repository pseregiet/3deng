#ifndef __texloader_h
#define __texloader_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

int texloader_init();
void texloader_kill();
sg_image texloader_find(const char *name);

void make_sg_image_16f(float *ptr, int size, int *w, int *h, sg_image *img);

#endif
