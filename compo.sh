#!/bin/bash

GITH=\"$(git rev-parse HEAD)\"

COMP="gcc"

OPTS="\
    -I./3rdparty/cJSON \
    -I./3rdparty/hmm \
    -I./3rdparty/klib \
    -I./3rdparty/sokol \
    -I./3rdparty/qoi \
    -I./3rdparty/glad \
    -DGIT_HASH=${GITH} \
    -Wno-missing-braces \
    -Wall \
    -g \
"

LIBS="\
    -lcimgui \
    -L../cimgui/ \
    -lSDL2 \
    -lm \
    -ldl \
"

FILE="\
    3rdparty/glad/glad.c \
    3rdparty/cJSON/cJSON.c \
    qoi.c \
    animatmapobj.c \
    animodel.c \
    animodel_mngr.c \
    camera.c \
    event.c \
    extrahmm.c \
    fileops.c \
    growing_allocator.c \
    heightmap.c \
    hitbox.c \
    lightcube.c \
    look.c \
    md5anim.c \
    md5loader.c \
    md5model.c \
    mouse2world.c \
    worldedit.c \
    objloader.c \
    objmodel.c \
    pipelines.c \
    sdl2_imgui.c \
    shadow.c \
    sokol_gfx.c \
    sokol_imgui.c \
    staticmapobj.c \
    stb_image.c \
    terrain.c \
    texloader.c \
"

echo "Compile shd_combo.glsl"
sokol-shdc --input shaders/shd_combo.glsl --output genshd_combo.h --slang glsl330
echo "Compile shd_terrain.glsl"
sokol-shdc --input shaders/shd_terrain.glsl --output genshd_terrain.h --slang glsl330
echo "Compile shd_hitboxcube.glsl"
sokol-shdc --input shaders/shd_hitboxcube.glsl --output genshd_hitboxcube.h --slang glsl330
echo "Compile shd_md5.glsl"
sokol-shdc --input shaders/shd_md5.glsl --output genshd_md5.h --slang glsl330
echo "Build the engine"
$COMP $OPTS $FILE $LIBS
