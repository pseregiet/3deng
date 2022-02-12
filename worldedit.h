#ifndef __worldedit_h
#define __worldedit_h
#include <SDL2/SDL.h>

void worldedit_imgui();
void worldedit_render();
void worldedit_update(double delta);
bool worldedit_input(SDL_Event *e);

int worldedit_init();
void worldedit_kill();

#endif

