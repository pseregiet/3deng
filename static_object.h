#ifndef __static_object_h
#define __static_object_h

#include "objloader.h"
#include "hmm.h"

struct static_object {
    struct model *model;
    hmm_mat4 matrix;
};

struct vector_static_object {
    struct static_object *data;
    int count;
    int cap;
};

extern struct vector_static_object static_objs;
int init_static_objects();

#endif

