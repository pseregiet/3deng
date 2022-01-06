#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "hmm.h"
#include "event.h"
#include "sdl2_stuff.h"
#include "sdl2_imgui.h"

enum keys {
    KEYW,
    KEYS,
    KEYA,
    KEYD,
    KEY_COUNT,
};

extern struct sdlobjs sdl;
extern struct camera cam;
extern bool gquit;
char keymap[KEY_COUNT] = {0};

void move_camera(double delta)
{
    float camspeed = 0.5f * delta;
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
    const float sens = 0.1f;
    float xoff = (float)mx * sens;
    float yoff = (float)my * sens;

    cam.yaw += xoff;
    cam.pitch += yoff;

    if (cam.pitch > 89.0f)
        cam.pitch = 89.0f;
    else if (cam.pitch < -89.0f)
        cam.pitch = -89.0f;

    float ryaw = HMM_ToRadians(cam.yaw);
    float rpitch = HMM_ToRadians(cam.pitch);

    hmm_vec3 dir = HMM_Vec3(cosf(ryaw) * cosf(rpitch),
                            sinf(rpitch),
                            sinf(ryaw) * cosf(rpitch));
    cam.front = HMM_NormalizeVec3(dir);
}

void do_input(double delta)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            gquit = 1;
            return;
        }

        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_BACKQUOTE) {
            sdl.imguifocus++;
            if (sdl.imguifocus > 2)
                sdl.imguifocus = 0;

            bool lockmouse = sdl.imguifocus == 1 ? false : true;
            SDL_SetRelativeMouseMode(lockmouse);
        }

        if (sdl.imguifocus == 1) {
            igsdl2_ProcessEvent(&e);
            continue;
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

    if (sdl.imguifocus == 1) {
        igsdl2_UpdateMousePosAndButtons();
        igsdl2_UpdateMouseCursor();
        return;
    }

    move_camera(delta);
}
