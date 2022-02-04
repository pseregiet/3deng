#ifndef __animodel_h
#define __animodel_h
#include "md5loader.h"
#include "extrahmm.h"
#include "frameinfo.h"
#include <stdbool.h>

struct animodel {
    struct md5_model *model;
    struct md5_joint *interp;
    float *bonebuf;
    sg_image bonemap;
    int boneuv[4];
    int curranim;
    int currframe;
    int nextframe;
    float lasttime;
    bool pause;
};

int animodel_init(struct animodel *am, const char *modelname);
void animodel_kill(struct animodel *am);
void animodel_play(struct animodel *am, double delta);
void animodel_plain(struct animodel *am);
void animodel_interpolate(struct animodel *am);
void animodel_joint2matrix(struct animodel *am);
void animodel_render(struct animodel *am, struct frameinfo *fi, hmm_mat4 model);

void animodel_pipeline(struct pipelines *pipes);
void animodel_shadow_pipeline(struct pipelines *pipes);
void animodel_shadow_render(struct animodel *am, struct frameinfo *fi, hmm_mat4 model);
#endif

