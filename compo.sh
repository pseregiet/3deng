#!/bin/bash

sokol-shdc --input combo.glsl --output genshader.h --slang glsl330
gcc look.c stb_image.c sokol_gfx.c shaderloader.c event.c -lSDL2 -lm -lGL && ./a.out
