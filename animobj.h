#ifndef __animobj_h
#define __animobj_h
#include "pipelines.h"
#include "md5model.h"

struct animobj {
    struct md5_model *model;
    struct md5_joint *interp;
    int curanim;
    int curframe;
    int nextframe;
    float lasttime;
};

int animobj_init();
void animobj_kill(struct animobj *obj);
void animobj_pipeline(struct pipelines *pipes);
void animobj_play(double delta);
void animobj_render(struct pipelines *pipes, hmm_mat4 vp);
void animobj_update_bonetex();

#endif

