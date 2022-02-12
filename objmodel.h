#ifndef __objmodel_h
#define __objmodel_h
#include "sokolgl.h"
#include "pipelines.h"
#include "frameinfo.h"
#include "hmm.h"

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
    sg_image imgnorm;
    int vcount;
    int icount;
};

void cube_index_buffer_init();
void cube_index_buffer_kill();

int objmodel_open(const char *fn, struct obj_model *mdl);
void objmodel_kill(struct obj_model *mdl);

void objmodel_pipeline(struct pipelines *pipes);
void objmodel_fraguniforms(struct frameinfo *fi);
void objmodel_render(const struct obj_model *mdl, struct frameinfo *fi, hmm_mat4 model);
#endif
