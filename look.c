//------------------------------------------------------------------------------
//  super fucking amazing openGL3.3 engine
//  super quallity shit code by Patryk Seregiet (c) 2021-2022
//------------------------------------------------------------------------------
#include "sokolgl.h"
#include <SDL2/SDL.h>
#include "hmm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sdl2_stuff.h"
#include "frameinfo.h"
#include "event.h"
#include "md5loader.h"
#include "objloader.h"
#include "terrain.h"
#include "worldedit.h"
#include "shadow.h"
#include "hitbox.h"
#include "animodel_mngr.h"
#include "objloader.h"
#include "animatmapobj.h"
#include "staticmapobj.h"
#include "texloader.h"
#include "genshd_combo.h"
#include "sdl2_imgui.h"

#define u16 uint16_t

#define WW 1280
#define WH 720

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

sg_image imgdummy = {0};
struct sdlobjs sdl = {0};

bool gquit = 0;
bool playanim = true;

struct frameinfo fi = {
    .cam = (struct camera) {
        .yaw = -90.f,
        .pitch = 0,
        .pos = {0.0f, 0.0f, 3.0f},
        .front = { 0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .dir = {0.0f, 0.0f, 0.0f},
    },
    .dirlight = (struct dirlight) {
        .dir =  {0.5f, 1.0f, 0.3f},
        .diff = {0.4f, 0.4f, 0.4f},
        .ambi = {0.05f, 0.05f, 0.05f},
        .spec = {0.5f, 0.5f, 0.5f},
    },
    .pointlight[0] = (struct pointlight) {
        .pos =  {0.7f, 0.2f, 2.0f},
        .ambi = {0.05f, 0.05f, 0.05f},
        .diff = {0.8f, 0.8f, 0.8f},
        .spec = {1.0f, 1.0f, 1.0f},
        .atte = {1.0f, 0.0014f, 0.000007f}
        //.atte = {1.0f, 0.09f, 0.032f},
    },
    .pointlight[1] = (struct pointlight) {
        .pos =  {2.3f, -3.3f, -4.0f},
        .ambi = {0.05f, 0.05f, 0.05f},
        .diff = {0.8f, 0.8f, 0.8f},
        .spec = {1.0f, 1.0f, 1.0f},
        .atte = {1.0f, 0.09f, 0.032f},
    },
    .pointlight[2] = (struct pointlight) {
        .pos =  {-4.0f, 2.0f, 12.0f},
        .ambi = {0.05f, 0.05f, 0.05f},
        .diff = {0.8f, 0.8f, 0.8f},
        .spec = {1.0f, 1.0f, 1.0f},
        .atte = {1.0f, 0.09f, 0.032f},
    },
    .pointlight[3] = (struct pointlight) {
        .pos =  {0.0f, 0.0f, -3.0f},
        .ambi = {0.05f, 0.05f, 0.05f},
        .diff = {0.8f, 0.8f, 0.8f},
        .spec = {1.0f, 1.0f, 1.0f},
        .atte = {1.0f, 0.09f, 0.032f},
    },
    .spotlight  = (struct spotlight) {
        //.pos = cam.pos,
        //.dir = cam.front,
        .ambi = {0.0f, 0.0f, 0.0f},
        .diff = {1.0f, 1.0f, 1.0f},
        .spec = {1.0f, 1.0f, 1.0f},
        .atte = {1.0f, 0.09f, 0.032f},
        //.atte = {1.0f, 0.0014f, 0.000007f}
        //.cutoff;
        //.outcutoff;
    },
};

static void sdl_video_driver()
{
    const char *vd = SDL_GetCurrentVideoDriver();
    printf("SDL Video driver: %s\n", vd);

    if (!strncmp(vd, "x11", 3))
        fi.vd = VD_X11;
    else if (!strncmp(vd, "wayland", 7))
        fi.vd = VD_WAYLAND;
}
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

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        fatalerror("gladLoadGLLoader failed\n");
    }

    sdl_video_driver();

    sg_setup(&(sg_desc) {0});
    if (!sg_isvalid()) {
        fatalerror("sg_setup failed\n");
        return -1;
    }

    //SDL_GL_SetSwapInterval(0);

    igsdl2_Init();

    gquit = 0;

    return 0;
}

static void do_imgui_frame(int w, int h, double delta)
{
    simgui_new_frame(w, h, delta);

    igBegin("Hello world! window", NULL, 0);

    igText("Camera pos: %f, %f, %f", fi.cam.pos.X, fi.cam.pos.Y, fi.cam.pos.Z);
    igText("Camera dir: %f, %f, %f", fi.cam.front.X, fi.cam.front.Y, fi.cam.front.Z);
    igText("Light dir: %f, %f, %f", fi.dirlight.dir.X, fi.dirlight.dir.Y, fi.dirlight.dir.Z);
    igColorEdit3("directional light diffuse", fi.dirlight.diff.Elements, ImGuiColorEditFlags_DefaultOptions_);
    igColorEdit3("directional light ambient", fi.dirlight.ambi.Elements, ImGuiColorEditFlags_DefaultOptions_);

    if (igCheckbox("Light enable1", &fi.lightsenable[0]))
        fi.pointlight[0].pos = fi.cam.pos;

    if (igCheckbox("Light enable2", &fi.lightsenable[1]))
        fi.pointlight[1].pos = fi.cam.pos;

    if (igCheckbox("Light enable3", &fi.lightsenable[2]))
        fi.pointlight[2].pos = fi.cam.pos;

    if (igCheckbox("Light enable4", &fi.lightsenable[3]))
        fi.pointlight[3].pos = fi.cam.pos;

    igCheckbox("Enable anims", &playanim);

    if (igButton("Set lightdir", (ImVec2) {100.0f, 30.0f})) {
        fi.dirlight.dir = fi.cam.front;
    }


    igText("App average %.3f ms/frame (%.1f FPS)", delta, 1000.0f / delta);
    igEnd();
}

static void do_update_animated(double delta)
{
    int end = animatmapobj_mngr_end();
    struct animodel *mdls[end];

    //here we would do sorting, culling etc.
    for (int i = 0; i < end; ++i) {
        struct animatmapobj *obj = animatmapobj_mngr_get(i);
        mdls[i] = &obj->am;
    }
    animodel_mngr_calc_boneuv(mdls, end);

    for (int i = 0; i < end; ++i) {
        animodel_play(mdls[i], delta);
        animodel_plain(mdls[i]);
        //animodel_interpolate(mdls[i]);
        animodel_joint2matrix(mdls[i]);
    }
}

static void do_update(double delta)
{
    double smalldelta = delta / 1000.f;
    worldedit_update(smalldelta);
    if (playanim)
        do_update_animated(smalldelta);
    //rotate static objects...very fucking static, lol
    /*
    static float rottt = 0.0f;
    for (int i = 0; i < static_objs.count; ++i) {
        static_objs.data[i].matrix = HMM_MultiplyMat4(static_objs.data[i].matrix,
                (HMM_Rotate(HMM_ToRadians(rottt), HMM_Vec3(1.0f, 1.0f, 1.0f))));
        rottt += 0.01;
    }
    if (rottt >= 360.f)
        rottt=0.f;
    */
}

static void do_render_static_objs(struct frameinfo *fi, double delta)
{
    sg_apply_pipeline(fi->pipes.objmodel);
    objmodel_fraguniforms_slow(fi);
    objmodel_vertuniforms_slow(fi);

    const int end = staticmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        const struct staticmapobj *obj = staticmapobj_get(i);
        const struct obj_model *mdl = obj->om;
        objmodel_render(mdl, fi, obj->matrix);
    }

    //simpleobj_hitbox_render(fi, munis->vp);
}

    /*
static void do_render_lightcubes(struct frameinfo *fi, double delta, vs_params_t *munis)
{
    sg_apply_pipeline(fi->pipes.lightcube);
    sg_apply_bindings(&fi->lightbind);
    hmm_mat4 scale = HMM_Scale(HMM_Vec3(0.1, 0.1, 0.1));
    for (int i = 0; i < 4; ++i) {

        if (!fi->lightsenable[i])
            continue;

        hmm_vec3 pos = {
            lightspos[i].X,
            lightspos[i].Y,
            lightspos[i].Z
        };
        hmm_mat4 model = HMM_Translate(pos);
        model = HMM_MultiplyMat4(model, scale);
        model = HMM_MultiplyMat4(munis->vp, model);
        vs_paramsl_t lvs = {model};

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(lvs));
        sg_draw(0, 36, 1);
    }
}
*/

static void do_render_animat_objs(struct frameinfo *fi, double delta)
{
    animodel_mngr_upload_bones();
    sg_apply_pipeline(fi->pipes.animodel);

    animodel_vertuniforms_slow(fi);
    animodel_fraguniforms_slow(fi);
    int end = animatmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        struct animatmapobj *obj = animatmapobj_mngr_get(i);
        animodel_render(&obj->am, fi, obj->matrix);
    }
}

static void do_render(struct frameinfo *fi, double delta)
{
    hmm_mat4 projection = HMM_Perspective(75.0f, (float)WW/(float)WH, 0.1f, 2000.0f);

    int w, h;
    SDL_GL_GetDrawableSize(sdl.win, &w, &h);
   
    shadowmap_draw();

    sg_begin_default_pass(&fi->pass_action, w, h);
    hmm_mat4 view = HMM_LookAt(fi->cam.pos, HMM_AddVec3(fi->cam.pos, fi->cam.front), fi->cam.up);
    hmm_mat4 vp = HMM_MultiplyMat4(projection, view);

    fi->vp = vp;
    fi->projection = projection;
    fi->view = view;

    draw_terrain(fi, vp, fi->dirlight.dir, fi->cam.pos, fi->shadow.lightspace);
 
    do_render_static_objs(fi, delta);
    //do_render_lightcubes(fi, delta, &munis);
    do_render_animat_objs(fi, delta);
    worldedit_render();


    if (sdl.imguifocus) {
        do_imgui_frame(w, h, delta);
        worldedit_imgui();
        simgui_render();
    }

    sg_end_pass();
    sg_commit();
    SDL_GL_SwapWindow(sdl.win);
}

static int imgdummy_init()
{
    char data[4] = {0,0,0,0};

    imgdummy = sg_make_image(&(sg_image_desc) {
        .width = 1,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .data.subimage[0][0] = {
            .ptr = data,
            .size = 4,
        },
    });

    //TODO: move this somewhere sensible
    fi.spotlight.cutoff = HMM_COSF(HMM_ToRadians(12.5f));
    fi.spotlight.outcutoff = HMM_COSF(HMM_ToRadians(15.0f));

    return 0;
}
static void imgdummy_kill()
{
    sg_destroy_image(imgdummy);
}

int main(int argc, char **argv)
{
#ifdef GIT_HASH
    printf("Welcome to 3deng. Git: %s\n", GIT_HASH);
#endif
    if (sdl_init()) {
        fatalerror("sdl_init() failed\n");
        return -1;
    }

    uint32_t init_start = SDL_GetTicks();
    assert(!imgdummy_init());
    assert(!texloader_init());
    assert(!pipelines_init(&fi.pipes));
    assert(!init_terrain());
    assert(!shadowmap_init());
    assert(!md5loader_init());
    assert(!objloader_init());
    assert(!animodel_mngr_init());
    assert(!animatmapobj_mngr_init());
    assert(!staticmapobj_mngr_init());
    assert(!worldedit_init());

    float vertices[36*3] =
    {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f, 
        0.5f,  0.5f, -0.5f, 
        0.5f,  0.5f, -0.5f, 
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        0.5f,  0.5f,  0.5f, 
        0.5f,  0.5f, -0.5f, 
        0.5f, -0.5f, -0.5f, 
        0.5f, -0.5f, -0.5f, 
        0.5f, -0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 

        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f, 
        0.5f, -0.5f,  0.5f, 
        0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f, 
        0.5f,  0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,

    };

    sg_buffer lvbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(vertices),
        .data.ptr = vertices,
    });
    
    fi.lightbind = (sg_bindings){
        .vertex_buffers[0] = lvbuf,
    };

    fi.pass_action = (sg_pass_action){
        .colors[0] = {.action = SG_ACTION_CLEAR, .value = {0.5, 0.5, 0.5, 1.0 }},
    };

    terrain_set_shadowmap(fi.shadow.depthmap);

    double init_done = (double)(SDL_GetTicks() - init_start);
    printf("%.3fs init\n", init_done / 1000.0f);

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = 0;
    double delta = 0;
    while (!gquit) {
        last = now;
        now = SDL_GetPerformanceCounter();
        delta = ((now - last)*1000 / (double)SDL_GetPerformanceFrequency());

        do_input(delta);
        do_update(delta);
        do_render(&fi, delta);

    }

    worldedit_kill();
    staticmapobj_mngr_kill();
    animatmapobj_mngr_kill();
    animodel_mngr_kill();
    objloader_kill();
    md5loader_kill();
    shadowmap_kill();
    //kill_terrain();
    pipelines_kill(&fi.pipes);
    texloader_kill();
    imgdummy_kill();
    

    igsdl2_Shutdown();
    sg_shutdown();
    SDL_GL_DeleteContext(sdl.ctx);
    SDL_DestroyWindow(sdl.win);
    SDL_Quit();

    return 0;
}

