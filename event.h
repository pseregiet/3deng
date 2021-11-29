#ifndef __event_h
#define __event_h

struct camera {
    float yaw;
    float pitch;
    hmm_vec3 pos;
    hmm_vec3 front;
    hmm_vec3 up;
    hmm_vec3 dir;
};

void do_input();
void move_camera();

#endif
