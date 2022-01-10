#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "growing_allocator.h"
#include "objloader.h"
#include "texloader.h"
#include "fast_obj.h"
#include "hmm.h"
#include "khash.h"
#include "genshd_combo.h"

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)
extern sg_image imgdummy;
static float *vbuf = 0;
static int vbufsize = 0;

KHASH_MAP_INIT_STR(vmodelmap, struct model *)
static khash_t(vmodelmap) vmodel;

static struct vmodel_data {
    struct growing_alloc models;
    struct growing_alloc names;
} vmodel_data;

//common index buffer for a simple cube
static sg_buffer cube_index_buffer;
static void init_cube_index_buffer()
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

void simpleobj_pipeline(struct pipelines *pipes)
{
    pipes->simpleobj_shd = sg_make_shader(comboshader_shader_desc(SG_BACKEND_GLCORE33));
    
    pipes->simpleobj = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->simpleobj_shd,
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

void vmodel_kill()
{
    growing_alloc_kill(&vmodel_data.models);
    growing_alloc_kill(&vmodel_data.names);
}

static int vmodel_append(const char *fp, const char *name)
{
    int namelen = strlen(name);
    char *newname = growing_alloc_get(&vmodel_data.names, namelen+1);
    assert(newname);
    memcpy(newname, name, namelen);
    newname[namelen] = 0;

    struct model *model = (struct model *)growing_alloc_get(&vmodel_data.models, sizeof(struct model));
    assert(model);

    if (obj_load(model, fp))
        return -1;

    int ret;
    khint_t idx = kh_put(vmodelmap, &vmodel, newname, &ret);
    kh_value(&vmodel, idx) = model;

    return 0;
}

int vmodel_get_key_value(int idx, struct model const **model, char const **name)
{
    if (!kh_exist(&vmodel, idx))
        return -1;

    *model = kh_val(&vmodel, idx);
    *name = kh_key(&vmodel, idx);
    return 0;
}

int vmodel_init()
{
    init_cube_index_buffer();

    FILE *f = fopen("models.txt", "r");
    if (!f) {
        printf("Can't open models.txt\n");
        return -1;
    }

    assert(!growing_alloc_init(&vmodel_data.models, sizeof(struct model) * 2, 1));
    assert(!growing_alloc_init(&vmodel_data.names, 0, 1));

    char line[0x1000];
    int linecount = 1;
    while (1) {
        line[0] = 0;
        fgets(line, 0x1000, f);
        if (!line[0] || line[0] == '#')
            break;

        char *fp = line;
        char *name = 0;

        int c = 0;
        while (1) {
            if (!line[c])
                break;

            if (line[c] == ':') {
                line[c] = 0;
                name = &line[c+1];
            }

            if (line[c] == '\n') {
                line[c] = 0;
                break;
            }
            c++;
        }

        if (!name) {
            printf("models.txt error on line %d\n", linecount);
            continue;
        }

        if (vmodel_append(fp, name)) {
            printf("Loading model %s failed\n", fp);
            return -1;
        }
    }

    return 0;
}

struct model *vmodel_find(const char *key)
{
    struct model *model = 0;
    khint_t idx = kh_get(vmodelmap, &vmodel, key);
    if (idx != kh_end(&vmodel))
        model = kh_val(&vmodel, idx);

    return model;
}

static int parse_obj_vertex(struct model *model, fastObjMesh *mesh)
{
    int facecount = mesh->face_count;
    printf("%d\n", facecount*3);
    int datasize = facecount * 3 * 8 * sizeof(float);
    if (datasize > vbufsize) {
        vbuf = (float *)realloc((void *)vbuf, datasize);
        if (!vbuf) {
            fatalerror("parse_obj out of memory\n");
            return -1;
        }
        vbufsize = datasize;
    }

    printf("facecount: %d, x3: %d\n", mesh->face_count, mesh->face_count*3);

    if (mesh->position_count != mesh->texcoord_count ||
            mesh->position_count != mesh->normal_count ||
            mesh->texcoord_count != mesh->normal_count)
    {
        printf("Non equal amount of vertex atributes: %d, %d, %d\n",
                mesh->position_count, mesh->normal_count, mesh->texcoord_count);
    }

    struct hitbox hitbox = {
        .x = {INFINITY, -INFINITY},
        .y = {INFINITY, -INFINITY},
        .z = {INFINITY, -INFINITY},
    };

    for (int i = 0; i < facecount *3; ++i) {
        fastObjIndex vertex = mesh->indices[i];

        int pos = i * 8;
        int vpos = vertex.p * 3;
        int npos = vertex.n * 3;
        int tpos = vertex.t * 2;

        memcpy(&vbuf[pos + 0], mesh->positions + vpos, 3 * sizeof(float));
        memcpy(&vbuf[pos + 3], mesh->normals + npos, 3 * sizeof(float));
        memcpy(&vbuf[pos + 6], mesh->texcoords + tpos, 2 * sizeof(float));

        if (vbuf[pos + 0] < hitbox.x[0])
            hitbox.x[0] = vbuf[pos + 0];
        if (vbuf[pos + 0] > hitbox.x[1])
            hitbox.x[1] = vbuf[pos + 0];

        if (vbuf[pos + 1] < hitbox.y[0])
            hitbox.y[0] = vbuf[pos + 1];
        if (vbuf[pos + 1] > hitbox.y[1])
            hitbox.y[1] = vbuf[pos + 1];

        if (vbuf[pos + 2] < hitbox.z[0])
            hitbox.z[0] = vbuf[pos + 2];
        if (vbuf[pos + 2] > hitbox.z[1])
            hitbox.z[1] = vbuf[pos + 2];
    }

    float cube_vertices[24] = {
        // front
        hitbox.x[1], hitbox.y[0], hitbox.z[0],
        hitbox.x[1], hitbox.y[0], hitbox.z[1],
        hitbox.x[0], hitbox.y[0], hitbox.z[1],
        hitbox.x[0], hitbox.y[0], hitbox.z[0],
        // back
        hitbox.x[1], hitbox.y[1], hitbox.z[0],
        hitbox.x[1], hitbox.y[1], hitbox.z[1],
        hitbox.x[0], hitbox.y[1], hitbox.z[1],
        hitbox.x[0], hitbox.y[1], hitbox.z[0],
    };

    model->hitbox_vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(cube_vertices),
        .data.ptr = cube_vertices,
    });
    model->hitbox_ibuf = cube_index_buffer;


    model->buffer = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = datasize,
        .data.ptr = (const void *)vbuf,
    });

    model->vcount = mesh->face_count * 3;

    return 0;
}

static int get_material(sg_image *img, const char *fn)
{
    if (!fn)
        return -1;

    int ret = load_sg_image(fn, img);
    return (ret != 0);
}

static void parse_obj_material(struct model *model, fastObjMesh *mesh)
{
    sg_image imgdiff = imgdummy;
    sg_image imgspec = imgdummy;
    sg_image imgbump = imgdummy;

    get_material(&imgdiff, mesh->materials[0].map_Kd.path);
    get_material(&imgspec, mesh->materials[0].map_Ks.path);
    //get_material(&imgbump, mesh->materials[0].map_bump.name);
    
    model->imgdiff = imgdiff;
    model->imgspec = imgspec;
    //model->imgbump = imgbump;
}

int obj_load(struct model *model, const char *fn)
{
    fastObjMesh *mesh = fast_obj_read(fn);
    if (!mesh) {
        fatalerror("model %s couldn't be loaded\n", fn);
        return -1;
    }

    if (parse_obj_vertex(model, mesh))
        return -1;

    parse_obj_material(model, mesh);
    return 0;
}

void obj_bind(struct model *model, sg_bindings *bind)
{
    bind->vertex_buffers[0] = model->buffer;
    bind->fs_images[SLOT_imgdiff] = model->imgdiff;
    bind->fs_images[SLOT_imgspec] = model->imgspec;
    //bind->fs_images[SLOT_imgbump] = model->imgbump;

}

