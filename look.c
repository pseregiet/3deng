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
    bool quit;
} sdl;

struct matrix_unis {
    hmm_mat4 model;
    hmm_mat4 view;
    hmm_mat4 projection;
};

struct camera {
    float yaw;
    float pitch;
    hmm_vec3 pos;
    hmm_vec3 front;
    hmm_vec3 up;
    hmm_vec3 dir;
};

char vs[0x1000];
char fs[0x1000];
char *load_shader(const char *fn, char *dst)
{
    FILE *f = fopen(fn, "r");
    if (!f) {
        fatalerror("cannot open %s shader\n", fn);
        return 0;
    }

    int size;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size >= 0x1000) {
        fatalerror("shader %s larger than expected %x > %x\n", fn, size, 0x1000);
        return 0;
    }
    
    fread(dst, 1, size, f);
    dst[size] = 0;
    
    return dst;
}

struct camera cam = {
    .yaw = 0,
    .pitch = 0,
    .pos = {0.0f, 0.0f, 3.0f},
    .front = { 0.0f, 0.0f, 0.0f},
    .up = {0.0f, 1.0f, 0.0f},
    .dir = {0.0f, 0.0f, 0.0f},
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

    sdl.quit = 0;

    return 0;
}

hmm_vec3 cubespos[10] = {
   (hmm_vec3){.X= 0.0f,.Y=  0.0f,.Z=  0.0f}, 
   (hmm_vec3){.X= 2.0f,.Y=  5.0f,.Z= -15.0f}, 
   (hmm_vec3){.X=-1.5f,.Y= -2.2f,.Z= -2.5f},  
   (hmm_vec3){.X=-3.8f,.Y= -2.0f,.Z= -12.3f},  
   (hmm_vec3){.X= 2.4f,.Y= -0.4f,.Z= -3.5f},  
   (hmm_vec3){.X=-1.7f,.Y=  3.0f,.Z= -7.5f},  
   (hmm_vec3){.X= 1.3f,.Y= -2.0f,.Z= -2.5f},  
   (hmm_vec3){.X= 1.5f,.Y=  2.0f,.Z= -2.5f}, 
   (hmm_vec3){.X= 1.5f,.Y=  0.2f,.Z= -1.5f}, 
   (hmm_vec3){.X=-1.3f,.Y=  1.0f,.Z= -1.5f} 
};


static void move_camera(int key)
{
    const float camspeed = 0.05f;
    hmm_vec3 off;
    switch (key) {
        case SDLK_w:
            off = HMM_MultiplyVec3f(cam.front, camspeed);
            cam.pos = HMM_AddVec3(cam.pos, off);
            break;
        case SDLK_s:
            off = HMM_MultiplyVec3f(cam.front, camspeed);
            cam.pos = HMM_SubtractVec3(cam.pos, off);
            break;
        case SDLK_a:
            off = HMM_MultiplyVec3f(HMM_NormalizeVec3(HMM_Cross(cam.front, cam.up)), camspeed);
            cam.pos = HMM_SubtractVec3(cam.pos, off);
            break;
        case SDLK_d:
            off = HMM_MultiplyVec3f(HMM_NormalizeVec3(HMM_Cross(cam.front, cam.up)), camspeed);
            cam.pos = HMM_AddVec3(cam.pos, off);
            break;
        case SDLK_z:
            printf("%f, %f, %f\n", cam.pos.X, cam.pos.Y, cam.pos.Z);
            cam.pos.Y += camspeed;
            break;
        case SDLK_x:
    }
}

static void rot_camera(int mx, int my)
{
    const float sens = 0.01f;
    float xoff = (float)mx * sens;
    float yoff = (float)my * sens;

    cam.yaw += xoff;
    cam.pitch += yoff;

    if (cam.pitch > 89.0f)
        cam.pitch = 89.0f;
    else if (cam.pitch < -89.0f)
        cam.pitch = -89.0f;

    hmm_vec3 dir = HMM_Vec3(cosf(cam.yaw) * cosf(cam.pitch),
                            sinf(cam.pitch),
                            sinf(cam.yaw) * cosf(cam.pitch));
    cam.front = HMM_NormalizeVec3(dir);
}

static void do_input()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            sdl.quit = 1;
            return;
        }

        else if (e.type == SDL_KEYDOWN) {
            move_camera(e.key.keysym.sym);
        }

        else if (e.type == SDL_MOUSEMOTION) {
            rot_camera(e.motion.xrel, e.motion.yrel);
        }
    }
}

int main(int argc, char **argv)
{
    if (sdl_init()) {
        fatalerror("sdl_init() failed\n");
        return -1;
    }

    float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(vertices),
        .data.ptr = vertices,
    });

/*
    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(indices),
        .data.ptr = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
    });
*/
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
        .vs.source = load_shader("vs.glsl", vs),
        .fs.source = load_shader("fs.glsl", fs),
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
        //.index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [0] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [1] = {.format = SG_VERTEXFORMAT_FLOAT2},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });

    sg_bindings bind = {
        .vertex_buffers[0] = vbuf,
        //.index_buffer = ibuf,
        .fs_images[0] = img
    };

    sg_pass_action pass_action = {0};

    hmm_mat4 projection = HMM_Perspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);


    while (!sdl.quit) {

        do_input();

        int w, h;
        SDL_GL_GetDrawableSize(sdl.win, &w, &h);
        sg_begin_default_pass(&pass_action, w, h);
        sg_apply_pipeline(pip);
        sg_apply_bindings(&bind);

        hmm_mat4 view = HMM_LookAt(cam.pos, HMM_AddVec3(cam.pos, cam.front), cam.up);

        for (int i = 0; i < 10; ++i) {
            hmm_mat4 model = HMM_Translate(cubespos[i]);
            model = HMM_MultiplyMat4(model, HMM_Rotate((float)SDL_GetTicks()/10 + (i*50), HMM_Vec3(1.0f, 0.3f, 0.5f)));
            struct matrix_unis munis = {
                .model = model,
                .view = view,
                .projection = projection,
            };
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){&munis, sizeof(munis)});

            sg_draw(0, 36, 1);
        }

        sg_end_pass();
        sg_commit();
        SDL_GL_SwapWindow(sdl.win);
        //SDL_Delay(16);
    }

    sg_shutdown();
    SDL_GL_DeleteContext(sdl.ctx);
    SDL_DestroyWindow(sdl.win);
    SDL_Quit();

    return 0;

}
