#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "hmm.h"
#include "event.h"

enum keys {
    KEYW,
    KEYS,
    KEYA,
    KEYD,
    KEY_COUNT,
};

extern struct camera cam;
extern bool gquit;
char keymap[KEY_COUNT] = {0};

void move_camera()
{
    const float camspeed = 0.05f;
    hmm_vec3 off;
    if (keymap[KEYW]) {
        off = HMM_MultiplyVec3f(cam.front, camspeed);
        cam.pos = HMM_AddVec3(cam.pos, off);
    }
    else if (keymap[KEYS]) {
        off = HMM_MultiplyVec3f(cam.front, camspeed);
        cam.pos = HMM_SubtractVec3(cam.pos, off);
    }

    if (keymap[KEYA]) {
        off = HMM_MultiplyVec3f(HMM_NormalizeVec3(HMM_Cross(cam.front, cam.up)), camspeed);
        cam.pos = HMM_SubtractVec3(cam.pos, off);
    }
    else if (keymap[KEYD]) {
        off = HMM_MultiplyVec3f(HMM_NormalizeVec3(HMM_Cross(cam.front, cam.up)), camspeed);
        cam.pos = HMM_AddVec3(cam.pos, off);
    }
    /*
        case SDLK_z:
            printf("%f, %f, %f\n", cam.pos.X, cam.pos.Y, cam.pos.Z);
            cam.pos.Y += camspeed;
            break;
        case SDLK_x:
    }
    */
}

static void rot_camera(int mx, int my)
{
    my = -my;
    const float sens = 0.01f;
    float xoff = (float)mx * sens;
    float yoff = (float)my * sens;

    cam.yaw += xoff;
    cam.pitch += yoff;

    if (cam.pitch > 0.99f)
        cam.pitch = 0.99f;
    else if (cam.pitch < -0.99)
        cam.pitch = -0.99;

    hmm_vec3 dir = HMM_Vec3(cosf(cam.yaw) * cosf(cam.pitch),
                            sinf(cam.pitch),
                            sinf(cam.yaw) * cosf(cam.pitch));
    cam.front = HMM_NormalizeVec3(dir);
}

void do_input()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            gquit = 1;
            return;
        }

        else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            char val = e.type == SDL_KEYDOWN;

            if (e.key.keysym.sym == SDLK_w)
                keymap[KEYW] = val;
            else if (e.key.keysym.sym == SDLK_s)
                keymap[KEYS] = val;
            else if (e.key.keysym.sym == SDLK_a)
                keymap[KEYA] = val;
            else if (e.key.keysym.sym == SDLK_d)
                keymap[KEYD] = val;
        }

        else if (e.type == SDL_MOUSEMOTION) {
            rot_camera(e.motion.xrel, e.motion.yrel);
        }
    }
}
