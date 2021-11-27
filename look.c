//------------------------------------------------------------------------------
//  1-9-3-look
//------------------------------------------------------------------------------
#include "../sokol/sokol_gfx.h"

#include <GL/gl.h>
#include <SDL2/SDL.h>
#include "stb_image.h"
#include "hmm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define u16 uint16_t

#define WW 800
#define WH 600

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

struct sdlobjs {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_GLContext ctx;
} sdl;

struct matrix_unis {
    hmm_mat4 model;
    hmm_mat4 view;
    hmm_mat4 projection;
};

static int sdl_init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        fatalerror("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdl.win = SDL_CreateWindow("SDL2 x sokol_gfx", 500, 500, WW, WH, SDL_WINDOW_OPENGL);
    if (!sdl.win) {
        fatalerror("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    sdl.ctx = SDL_GL_CreateContext(sdl.win);
    if (!sdl.ctx) {
        fatalerror("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return -1;
    }

    sg_setup(&(sg_desc) {0});
    if (!sg_isvalid()) {
        fatalerror("sg_setup failed\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (sdl_init()) {
        fatalerror("sdl_init() failed\n");
        return -1;
    }

    float vertices[] = {
        -0.5f,  0.5f, 0.0, 0.0, 1.0,
        0.5f,   0.5f, 0.0, 1.0, 1.0,
        0.5f,  -0.5f, 0.0, 1.0, 0.0,
        -0.5f, -0.5f, 0.0, 0.0, 0.0,
    };

    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(vertices),
        .data.ptr = vertices,
    });

    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(indices),
        .data.ptr = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
    });

    int w,h,n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("sprites.png", &w, &h, &n, 0);
    if (!data) {
        fatalerror("stbi_load(%s) failed\n", "sprites.png");
        return -1;
    }

    printf("%d:%d:%d\n", w, h, n);

    sg_image img = sg_make_image(&(sg_image_desc) {
        .width = w,
        .height = h,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .data.subimage[0][0] = {
            .ptr = data,
            .size = (w*h*n),
        },
    });

    printf("sg_image\n");

    sg_shader shd = sg_make_shader(&(sg_shader_desc) {
        .fs.images[0] = {
            .name = "tex",
            .image_type = SG_IMAGETYPE_2D,
        },
        .vs.uniform_blocks[0] = {
            .size = sizeof(struct matrix_unis),
            .uniforms = {
                [0] = {.name="model", .type = SG_UNIFORMTYPE_MAT4},
                [1] = {.name="view", .type = SG_UNIFORMTYPE_MAT4},
                [2] = {.name="projection", .type = SG_UNIFORMTYPE_MAT4},
            },
        },
        .vs.source =
            "#version 330\n"
            "layout(location = 0) in vec3 position;\n"
            "layout(location = 1) in vec2 texcoord0;\n"
            "out vec2 uv;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "   gl_Position = projection * view * model * vec4(position, 1.0);\n"
            "   uv = texcoord0;\n"
            "}\n"
        ,
        .fs.source = 
            "#version 330\n"
            "uniform sampler2D tex;\n"
            "in vec2 uv;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "   frag_color = texture(tex, uv);\n"
            "}\n"
        ,
    });

    printf("shd = %p\n", &shd);

    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = shd,
        .color_count = 1,
        .colors[0] = {
            .write_mask = SG_COLORMASK_RGB,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [0] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [1] = {.format = SG_VERTEXFORMAT_FLOAT2},
            },
        },
    });

    sg_bindings bind = {
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf,
        .fs_images[0] = img
    };

    sg_pass_action pass_action = {0};

    hmm_mat4 model = HMM_Rotate(-55.0f, HMM_Vec3(1.0f, 0.0f, 0.0f));
    hmm_mat4 view = HMM_Translate(HMM_Vec3(0.0f, 0.0f, -3.0f));
    hmm_mat4 projection = HMM_Perspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);

    while (!SDL_QuitRequested()) {
        int w, h;
        SDL_GL_GetDrawableSize(sdl.win, &w, &h);
        sg_begin_default_pass(&pass_action, w, h);
        
        /*
        hmm_mat4 rotate = HMM_Rotate(90.0f, HMM_Vec3(0.0f, 0.0f, 1.0f));
        hmm_mat4 scale = HMM_Scale(HMM_Vec3(0.5f, 0.5f, 0.5f));
        hmm_mat4 trans = HMM_MultiplyMat4(rotate, scale);
        */

        hmm_mat4 translate = HMM_Translate(HMM_Vec3(0.5f, -0.5f, 0.0f));
        hmm_mat4 rotate = HMM_Rotate((float)SDL_GetTicks() * 1,
                HMM_Vec3(0.0f, 0.0f, 1.0f));
        hmm_mat4 trans = HMM_MultiplyMat4(translate, rotate);
        sg_apply_pipeline(pip);
        sg_apply_bindings(&bind);

        struct matrix_unis munis = {
            .model = model,
            .view = view,
            .projection = projection,
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){&munis, sizeof(munis)});

        sg_draw(0, 6, 1);
        sg_end_pass();
        sg_commit();
        SDL_GL_SwapWindow(sdl.win);
        SDL_Delay(10);
    }

    sg_shutdown();
    SDL_GL_DeleteContext(sdl.ctx);
    SDL_DestroyWindow(sdl.win);
    SDL_Quit();

    return 0;

}
