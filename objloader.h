#ifndef __obj_loader_h
#define __obj_loader_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

struct model {
    sg_buffer buffer;
    sg_image imgdiff;
    sg_image imgspec;
    sg_image imgbump;
    int vcount;
};

int load_sg_image(const char *fn, sg_image *img);
int load_sg_image_array(const char **fn, sg_image *img, int count);
int obj_load(struct model *model, const char *fn);
void obj_bind(struct model *model, sg_bindings *bind);

#endif

