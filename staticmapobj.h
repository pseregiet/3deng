#ifndef __staticmapobj_h
#define __staticmapobj_h
#include "objloader.h"
#include "hmm.h"

struct staticmapobj {
    const struct obj_model *om;
    hmm_mat4 matrix;
};

void staticmapobj_setpos(struct staticmapobj *swo, hmm_vec3 pos);

//--- object so simple it doesn't need its own file...
int staticmapobj_mngr_init();
void staticmapobj_mngr_kill();
int staticmapobj_mngr_end();
struct staticmapobj *staticmapobj_get(int idx);
#endif
