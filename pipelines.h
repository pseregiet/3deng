#ifndef __pipelines_h
#define __pipelines_h

#include "sokolgl.h"

struct pipelines {
    sg_pipeline terrain;
    sg_pipeline animodel;
    sg_pipeline animodel_shadow;
    sg_pipeline objmodel;
    sg_pipeline particle;
    sg_pipeline wirebox;
    sg_pipeline objmodel_shadow;
    sg_pipeline terrain_shadow;

    sg_shader terrain_shd;
    sg_shader lightcube_shd;
    sg_shader animodel_shd;
    sg_shader animodel_shadow_shd;
    sg_shader objmodel_shd;
    sg_shader particle_shd;
    sg_shader wirebox_shd;
    sg_shader objmodel_shadow_shd;
    sg_shader terrain_shadow_shd;
};

int pipelines_init(struct pipelines *pipes);
void pipelines_kill(struct pipelines *pipes);
#endif

