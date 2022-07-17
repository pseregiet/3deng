#ifndef __objmodel_h
#define __objmodel_h
#include "sokolgl.h"
#include "pipelines.h"
#include "frameinfo.h"
#include "hmm.h"

struct obj_model {
    sg_buffer vbuf;
    sg_buffer ibuf;
    int vcount;
    int icount;
    struct material material;
};

int objmodel_open(const char *fn, struct obj_model *mdl);
void objmodel_kill(struct obj_model *mdl);

void objmodel_pipeline(struct pipelines *pipes);
void objmodel_shadow_pipeline(struct pipelines *pipes);
void objmodel_fraguniforms_slow(struct frameinfo *fi);
void objmodel_vertuniforms_slow(struct frameinfo *fi);
void objmodel_render(const struct obj_model *mdl, struct frameinfo *fi, hmm_mat4 model);
void objmodel_shadow_render(const struct obj_model *mdl, struct frameinfo *fi, hmm_mat4 model);
#endif
