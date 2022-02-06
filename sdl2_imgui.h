#ifndef __sdl2_imgui_h
#define __sdl2_imgui_h

#include "sokolgl.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "../cimgui/cimgui.h"
#define SOKOL_IMGUI_NO_SOKOL_APP
#include "../sokol/util/sokol_imgui.h"

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

bool igsdl2_ProcessEvent(SDL_Event *e);
void igsdl2_Init(void);
void igsdl2_Shutdown(void);
void igsdl2_UpdateMousePosAndButtons(void);
void igsdl2_UpdateMouseCursor(void);

#endif
