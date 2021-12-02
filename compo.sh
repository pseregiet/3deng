#!/bin/bash

sokol-shdc --input combo.glsl --output genshader.h --slang glsl330
gcc -g look.c stb_image.c sokol_gfx.c shaderloader.c event.c objloader.c fast_obj.c mouse2world.c sdl2_imgui.c sokol_imgui.c -lcimgui -L../cimgui/ -lSDL2 -lm -lGL && ./a.out
