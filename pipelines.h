#ifndef __pipelines_h
#define __pipelines_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

struct pipelines {
    sg_pipeline simpleobj_hitbox;
    sg_pipeline terrain;
    sg_pipeline lightcube;
    sg_pipeline animodel;
    sg_pipeline objmodel;

    sg_shader simpleobj_hitbox_shd;
    sg_shader terrain_shd;
    sg_shader lightcube_shd;
    sg_shader animodel_shd;
    sg_shader objmodel_shd;
};

int pipelines_init(struct pipelines *pipes);
void pipelines_kill(struct pipelines *pipes);
#endif

