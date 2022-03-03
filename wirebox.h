#ifndef __wirebox_h
#define __wirebox_h

#include "pipelines.h"
#include "frameinfo.h"
#include "hmm.h"

int wirebox_init();
void wirebox_kill();
void wirebox_pipeline(struct pipelines *pipes);

void wirebox_prepare_render(struct frameinfo *fi);
void wirebox_render(struct frameinfo *fi, hmm_mat4 model);

#endif

