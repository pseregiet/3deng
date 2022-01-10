#ifndef __hitbox_h
#define __hitbox_h
#include "pipelines.h"
#include "frameinfo.h"

void simpleobj_hitbox_pipeline(struct pipelines *pipes);
void simpleobj_hitbox_render(struct frameinfo *fi, hmm_mat4 vp);

#endif

