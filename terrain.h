#ifndef __terrain_h
#define __terrain_h
#include "hmm.h"
#include "frameinfo.h"

int init_terrain();
void terrain_pipeline(struct pipelines *pipes);

void draw_terrain(struct frameinfo *fi, hmm_mat4 vp,
        hmm_vec3 lightpos, hmm_vec3 viewpos, hmm_mat4 lightmatrix);
void terrain_set_shadowmap(sg_image shadowmap);

#endif
