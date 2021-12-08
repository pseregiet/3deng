#!/bin/bash

#sokol-shdc --input combo.glsl --output genshader.h --slang glsl330
#gcc -g look.c stb_image.c sokol_gfx.c shaderloader.c event.c objloader.c fast_obj.c mouse2world.c sdl2_imgui.c sokol_imgui.c -lcimgui -L../cimgui/ -lSDL2 -lm -lGL && ./a.out

COMP="gcc"

OPTS="\
    -g\
"

LIBS="\
    -lcimgui
    -L../cimgui/
    -lSDL2 \
    -lm \
    -lGL \
"

FILE="\
    event.c \
    fast_obj.c \
    look.c \
    mouse2world.c \
    objloader.c \
    sdl2_imgui.c \
    shaderloader.c \
    sokol_gfx.c \
    sokol_imgui.c \
    stb_image.c \
    shadow.c \
    terrain.c \
    texloader.c \
    extrahmm.c \
"

echo "Compile combo.glsl"
sokol-shdc --input combo.glsl --output genshader.h --slang glsl330
echo "Compile shdterrain.glsl"
sokol-shdc --input shd_terrain.glsl --output genshd_terrain.h --slang glsl330
echo "Build the engine"
$COMP $OPTS $FILE $LIBS && ./a.out
