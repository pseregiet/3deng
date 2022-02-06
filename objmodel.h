#ifndef __objmodel_h
#define __objmodel_h
#include "sokolgl.h"
#include "pipelines.h"

struct obj_model {
//TODO:
//hitbox_ibuf is common (just a simple cube)
//so it could be taken out of here
    sg_buffer hitbox_vbuf;
    sg_buffer hitbox_ibuf;

    sg_buffer vbuf;
    sg_buffer ibuf;
    sg_image imgdiff;
    sg_image imgspec;
    sg_image imgbump;
    int vcount;
    int tcount;
};

void cube_index_buffer_init();
void cube_index_buffer_kill();

int objmodel_open(const char *fn, struct obj_model *mdl);
void objmodel_kill(struct obj_model *mdl);

void objmodel_pipeline(struct pipelines *pipes);
#endif
