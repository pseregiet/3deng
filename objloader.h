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

int obj_load(struct model *model, const char *fn);
void obj_bind(struct model *model, sg_bindings *bind);

int vmodel_init();
void vmodel_kill();
int vmodel_get_key_value(int idx, struct model const **model, char const **name);
struct model *vmodel_find(const char *key);

#endif

