#ifndef __camera_h
#define __camera_h
#include "hmm.h"

struct camera {
    float yaw;
    float pitch;
    hmm_vec3 pos;
    hmm_vec3 front;
    hmm_vec3 up;
    hmm_vec3 dir;
};

#endif
