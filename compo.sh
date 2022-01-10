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
    pipelines.c \
    lightcube.c \
    hitbox.c \
    mouse2world.c \
    objloader.c \
    static_object.c \
    growing_allocator.c \
    sdl2_imgui.c \
    shaderloader.c \
    sokol_gfx.c \
    sokol_imgui.c \
    stb_image.c \
    shadow.c \
    heightmap.c \
    terrain.c \
    texloader.c \
    extrahmm.c \
"

echo "Compile shd_combo.glsl"
sokol-shdc --input shd_combo.glsl --output genshd_combo.h --slang glsl330
echo "Compile shd_terrain.glsl"
sokol-shdc --input shd_terrain.glsl --output genshd_terrain.h --slang glsl330
echo "Compile shd_hitboxcube.glsl"
sokol-shdc --input shd_hitboxcube.glsl --output genshd_hitboxcube.h --slang glsl330
echo "Build the engine"
$COMP $OPTS $FILE $LIBS && ./a.out
