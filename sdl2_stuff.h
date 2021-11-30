#ifndef __sdl2_stuff_h
#define __sdl2_stuff_h

#include <SDL2/SDL.h>
#include <stdbool.h>

//ImGuiMouseCursor_COUNT = 10
struct sdlobjs {
    SDL_Window *win;
    SDL_GLContext ctx;
    SDL_Cursor *cursor[10*2];
    char *clipboard;
    bool mouse[3];
    char imguifocus;
};

#endif
