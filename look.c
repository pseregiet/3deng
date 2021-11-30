//------------------------------------------------------------------------------
//  1-9-3-look
//------------------------------------------------------------------------------
#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

#include <GL/gl.h>
#include <SDL2/SDL.h>
#include "stb_image.h"
#include "hmm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdl2_stuff.h"
#include "shaderloader.h"
#include "event.h"
#include "genshader.h"
#include "sdl2_imgui.h"

#define u16 uint16_t

#define WW 1280
#define WH 720

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

struct sdlobjs sdl = {0};

bool gquit = 0;
struct matrix_unis {
    hmm_mat4 model;
    hmm_mat4 view;
    hmm_mat4 projection;
};


char vs[0x1000];
char fs[0x1000];

struct camera cam = {
    .yaw = -90.f,
    .pitch = 0,
    .pos = {0.0f, 0.0f, 3.0f},
    .front = { 0.0f, 0.0f, -1.0f},
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

    SDL_GL_SetSwapInterval(0);

    igsdl2_Init();

    gquit = 0;

    return 0;
}

hmm_vec3 cubespos[10] = {

    {.X= 0.0f, .Y= 0.0f, .Z= 0.0f},
    {.X= 2.0f, .Y= 5.0f, .Z=-15.0f},
    {.X=-1.5f, .Y=-2.2f, .Z=-2.5f},
    {.X=-3.8f, .Y=-2.0f, .Z=-12.3f},
    {.X= 2.4f, .Y=-0.4f, .Z=-3.5f},
    {.X=-1.7f, .Y= 3.0f, .Z=-7.5f},
    {.X= 1.3f, .Y=-2.0f, .Z=-2.5f},
    {.X= 1.5f, .Y= 2.0f, .Z=-2.5f},
    {.X= 1.5f, .Y= 0.2f, .Z=-1.5f},
    {.X=-1.3f, .Y= 1.0f, .Z=-1.5f},

};

hmm_vec4 lightspos[4] = {
    {0.7f, 0.2f, 2.0f, 1.0f},
    {2.3f, -3.3f, -4.0f, 1.0f},
    {-4.0f, 2.0f, 12.0f, 1.0f},
    {0.0f, 0.0f, -3.0f, 1.0f}
};

static int load_sg_image(const char *fn, sg_image *img)
{
    int w,h,n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(fn, &w, &h, &n, 0);
    if (!data) {
        fatalerror("stbi_load(%s) failed\n", fn);
        return -1;
    }

    printf("%d:%d:%d\n", w, h, n);

    *img = sg_make_image(&(sg_image_desc) {
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

    stbi_image_free(data);
}

struct frameinfo {
    sg_pipeline mainpip;
    sg_bindings mainbind;

    sg_pipeline lightpip;
    sg_bindings lightbind;

    sg_pass_action pass_action;
};

static void do_imgui_frame(int w, int h, double delta)
{
    simgui_new_frame(w, h, delta);
    static float f = 0.0f;

    igBegin("Hello world! window", NULL, 0);
    igText("Useful text");
    igText("Hello World!");
    igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 1.0f);
    igText("App average %.3f ms/frame (%.1f FPS)", delta, 1000.0f / delta);
    igEnd();
}

static void do_frame(struct frameinfo *fi, double delta)
{
    hmm_mat4 projection = HMM_Perspective(45.0f, (float)WW/(float)WH, 0.1f, 100.0f);

    int w, h;
    SDL_GL_GetDrawableSize(sdl.win, &w, &h);
    
    if (sdl.imguifocus)
        do_imgui_frame(w, h, delta);

    sg_begin_default_pass(&fi->pass_action, w, h);
    sg_apply_pipeline(fi->mainpip);
    sg_apply_bindings(&fi->mainbind);

    //fs uniforms
    fs_params_t fs_params = {
        .viewpos = cam.pos,
        .matshine = 32.0f,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &SG_RANGE(fs_params));

    fs_dir_light_t fs_dir_light = {
        .direction = HMM_Vec3(-0.2f, -1.0f, -0.3f),
        .ambient = HMM_Vec3(0.05f, 0.05f, 0.05f),
        .diffuse = HMM_Vec3(0.4f, 0.4f, 0.4f),
        .specular = HMM_Vec3(0.5f, 0.5f, 0.5f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_dir_light, &SG_RANGE(fs_dir_light));

    fs_point_lights_t fs_point_lights = {
        .position[0]    = lightspos[0],
        .ambient[0]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[0]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[0]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[0] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .position[1]    = lightspos[1],
        .ambient[1]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[1]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[1]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[1] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .position[2]    = lightspos[2],
        .ambient[2]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[2]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[2]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[2] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .position[3]    = lightspos[3],
        .ambient[3]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[3]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[3]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[3] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_point_lights, &SG_RANGE(fs_point_lights));

    fs_spot_light_t fs_spot_light = {
        .position = cam.pos,
        .direction = cam.front,//HMM_AddVec3(cam.pos, cam.front),
        .cutoff = HMM_COSF(HMM_ToRadians(12.5f)),//cosf(12.5f),
        .outcutoff = HMM_COSF(HMM_ToRadians(15.0f)),//cosf(15.0f),
        .attenuation = HMM_Vec3(1.0f, 0.09f, 0.032f),
        .ambient = HMM_Vec3(0.0f, 0.0f, 0.0f),
        .diffuse = HMM_Vec3(1.0f, 1.0f, 1.0f),
        .specular = HMM_Vec3(1.0f, 1.0f, 1.0f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_spot_light, &SG_RANGE(fs_spot_light));

    hmm_mat4 view = HMM_LookAt(cam.pos, HMM_AddVec3(cam.pos, cam.front), cam.up);

    for (int i = 0; i < 10; ++i) {
        hmm_mat4 model = HMM_Translate(cubespos[i]);
        model = HMM_MultiplyMat4(model, HMM_Rotate((float)SDL_GetTicks()/10 + (i*50), HMM_Vec3(1.0f, 0.3f, 0.5f)));
        vs_params_t munis = {
            .model = model,
            .view = view,
            .projection = projection,
        };
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(munis));

        sg_draw(0, 36, 1);
    }

    sg_apply_pipeline(fi->lightpip);
    sg_apply_bindings(&fi->lightbind);
    vs_params_t lvs = {
        .view = view,
        .projection = projection,
    };

    hmm_mat4 scale = HMM_Scale(HMM_Vec3(0.2f, 0.2f, 0.2f));
    for (int i = 0; i < 4; ++i) {
        hmm_vec3 pos = {
            lightspos[i].X,
            lightspos[i].Y,
            lightspos[i].Z
        };
        lvs.model = HMM_Translate(pos);
        lvs.model = HMM_MultiplyMat4(lvs.model, scale);

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(lvs));
        sg_draw(0, 36, 1);
    }

    if (sdl.imguifocus)
        simgui_render();

    sg_end_pass();
    sg_commit();
    SDL_GL_SwapWindow(sdl.win);
}

int main(int argc, char **argv)
{
    if (sdl_init()) {
        fatalerror("sdl_init() failed\n");
        return -1;
    }

    struct frameinfo fi;

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
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

    sg_image img;
    sg_image img2;
    if (load_sg_image("container2.png", &img)) {
        fatalerror("cannot load image\n");
        return -1;
    }

    if (load_sg_image("container2_specular.png", &img2)) {
        fatalerror("cannot load image\n");
        return -1;
    }


    sg_shader shd = sg_make_shader(comboshader_shader_desc(SG_BACKEND_GLCORE33));
    sg_shader lightshd = sg_make_shader(light_cube_shader_desc(SG_BACKEND_GLCORE33));

    fi.mainpip = sg_make_pipeline(&(sg_pipeline_desc) {
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
                [ATTR_vs_position] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_normal] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_texcoord] = {.format = SG_VERTEXFORMAT_FLOAT2},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });

    fi.mainbind = (sg_bindings){
        .vertex_buffers[0] = vbuf,
        //.index_buffer = ibuf,
        .fs_images[SLOT_diffuse_tex] = img,
        .fs_images[SLOT_specular_tex] = img2,
    };
    
    fi.lightpip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = lightshd,
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
                [ATTR_light_cube_vs_pos] = {.format = SG_VERTEXFORMAT_FLOAT3},
            },
            .buffers[0].stride = 32,
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });

    fi.lightbind = (sg_bindings){
        .vertex_buffers[0] = vbuf,
    };

    fi.pass_action = (sg_pass_action){
        .colors[0] = {.action = SG_ACTION_CLEAR, .value = {0.0, 0.0, 0.0, 1.0 }},
    };


    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = 0;
    double delta = 0;
    while (!gquit) {
        last = now;
        now = SDL_GetPerformanceCounter();
        delta = ((now - last)*1000 / (double)SDL_GetPerformanceFrequency());

        do_input(delta);
        do_frame(&fi, delta);

    }

    igsdl2_Shutdown();
    sg_shutdown();
    SDL_GL_DeleteContext(sdl.ctx);
    SDL_DestroyWindow(sdl.win);
    SDL_Quit();

    return 0;

}
