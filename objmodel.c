#include "objmodel.h"
#include "texloader.h"
#include "fast_obj.h"
#include "hmm.h"
#include "genshd_combo.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static sg_buffer cube_index_buffer;
void cube_index_buffer_init()
{
    uint16_t indices[36] = {
        // front
		0, 1, 2,   2, 3, 0,
		// right
		1, 5, 6,   6, 2, 1,
		// back
		7, 6, 5,   5, 4, 7,
		// left
		4, 0, 3,   3, 7, 4,
		// bottom
		4, 5, 1,   1, 0, 4,
		// top
		3, 2, 6,   6, 7, 3,
    };

    cube_index_buffer = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(indices),
        .data.ptr = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
    });
}
void cube_index_buffer_kill()
{
    sg_destroy_buffer(cube_index_buffer);
}
extern sg_image imgdummy;

inline static sg_image get_material(const char *fn)
{
    sg_image img = imgdummy;
    if (!fn)
        return img;

    load_sg_image(fn, &img);
    return img;
}

struct vertex {
    hmm_vec3 pos;
    hmm_vec3 norm;
    hmm_vec2 uv;
};

struct hitbox {
    hmm_vec3 min;
    hmm_vec3 max;
};

inline static void find_hitbox(hmm_vec3 pos, struct hitbox *hb)
{
    if (pos.X < hb->min.X)
        hb->min.X = pos.X;
    if (pos.Y < hb->min.Y)
        hb->min.Y = pos.Y;
    if (pos.Z < hb->min.Z)
        hb->min.Z = pos.Z;

    if (pos.X > hb->max.X)
        hb->max.X = pos.X;
    if (pos.Y < hb->max.Y)
        hb->max.Y = pos.Y;
    if (pos.Z < hb->max.Z)
        hb->max.Z = pos.Z;
}

inline static void make_hitbox(struct obj_model *mdl, struct hitbox *hb)
{
    const float cube_vertices[24] = {
        // front
        hb->max.X, hb->min.Y, hb->min.Z,
        hb->max.X, hb->min.Y, hb->max.Z,
        hb->min.X, hb->min.Y, hb->max.Z,
        hb->min.X, hb->min.Y, hb->min.Z,
        // back
        hb->max.X, hb->max.Y, hb->min.Z,
        hb->max.X, hb->max.Y, hb->max.Z,
        hb->min.X, hb->max.Y, hb->max.Z,
        hb->min.X, hb->max.Y, hb->min.Z,
    };

    mdl->hitbox_vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(cube_vertices),
        .data.ptr = cube_vertices,
    });
    mdl->hitbox_ibuf = cube_index_buffer;
}

static int parse_objvertex(struct obj_model *mdl, fastObjMesh *mesh)
{
    const int fc = mesh->face_count;
    const int vs = sizeof(struct vertex);
    const int vc = fc * 3;
    const int ds = vc * vs;

    struct vertex *vbuf = calloc(vc, vs);
    assert(vbuf);

    struct hitbox hb = {
        .min = (hmm_vec3){-INFINITY, -INFINITY, -INFINITY},
        .max = (hmm_vec3){INFINITY, INFINITY, INFINITY},
    };

    for (int i = 0; i < vc; ++i) {
        fastObjIndex vert = mesh->indices[i];
        int vpos = vert.p * 3;
        int npos = vert.n * 3;
        int tpos = vert.t * 2;

        memcpy(&vbuf[i].pos,  &mesh->positions[vpos], sizeof(hmm_vec3));
        memcpy(&vbuf[i].norm, &mesh->normals[npos],   sizeof(hmm_vec3));
        memcpy(&vbuf[i].uv,   &mesh->texcoords[tpos], sizeof(hmm_vec2));

        find_hitbox(vbuf[i].pos, &hb);
    }
    make_hitbox(mdl, &hb);

    mdl->vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = ds,
        .data.ptr = (const void *)vbuf,
    });

    mdl->vcount = vc;
    mdl->tcount = fc;
    return 0;
}

static void parse_objmaterials(struct obj_model *mdl, fastObjMesh *mesh)
{
    sg_image imgs[3] = {
        get_material(mesh->materials[0].map_Kd.path),
        get_material(mesh->materials[0].map_Ks.path),
        get_material(mesh->materials[0].map_bump.path),
    };

    mdl->imgdiff = imgs[0];
    mdl->imgspec = imgs[1];
    mdl->imgbump = imgs[2];
}

int objmodel_open(const char *fn, struct obj_model *mdl)
{
    fastObjMesh *mesh = fast_obj_read(fn);
    if (!mesh) {
        printf("%s: fast_obj_read failed\n", fn);
        return -1;
    }

    if (parse_objvertex(mdl, mesh))
        return -1;

    parse_objmaterials(mdl, mesh);
    return 0;
}

void objmodel_kill(struct obj_model *mdl)
{
    sg_destroy_buffer(mdl->hitbox_vbuf);
    sg_destroy_buffer(mdl->vbuf);
    sg_destroy_buffer(mdl->ibuf);

    sg_destroy_image(mdl->imgdiff);
    sg_destroy_image(mdl->imgspec);
    sg_destroy_image(mdl->imgbump);
}

void objmodel_pipeline(struct pipelines *pipes)
{
    pipes->objmodel_shd = sg_make_shader(comboshader_shader_desc(SG_BACKEND_GLCORE33));
    
    pipes->objmodel = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->objmodel_shd,
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
        .cull_mode = SG_CULLMODE_FRONT,
    });
}
