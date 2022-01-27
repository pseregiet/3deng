#ifndef __animatmapobj_h
#define __animatmapobj_h
#include "animodel.h"
#include "extrahmm.h"

struct animatmapobj {
    struct animodel am;
    hmm_mat4 matrix;
};

void animatmapobj_setpos(struct animatmapobj *swo, hmm_vec3 pos);

//--- object so simple it doesn't need its own file...
int animatmapobj_mngr_init();
void animatmapobj_mngr_kill();
int animatmapobj_mngr_end();
struct animatmapobj *animatmapobj_mngr_get(int idx);
#endif

