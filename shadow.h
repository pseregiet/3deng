#ifndef __shadow_h
#define __shadow_h

#include "sokolgl.h"
#include "hmm.h"
#include "pipelines.h"

int  shadowmap_init();
void shadowmap_kill();
void shadowmap_draw();
void shadow_pipeline(struct pipelines *pipes);

#endif

