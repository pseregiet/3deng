#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "growing_allocator.h"
#include "objloader.h"
#include "texloader.h"
#include "fast_obj.h"
#include "hmm.h"
#include "genshader.h"
#include "khash.h"

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)
extern sg_image imgdummy;
static float *vbuf = 0;
static int vbufsize = 0;

KHASH_MAP_INIT_STR(vmodelmap, struct model *)
static khash_t(vmodelmap) vmodel;
static khash_t(vmodelmap) *vmodelptr;

static struct vmodel_data {
    struct growing_alloc models;
    struct growing_alloc names;
} vmodel_data;

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
    khint_t idx = kh_put(vmodelmap, vmodelptr, newname, &ret);
    kh_value(vmodelptr, idx) = model;

    return 0;
}

int vmodel_get_key_value(int idx, struct model const **model, char const **name)
{
    if (!kh_exist(vmodelptr, idx))
        return -1;

    *model = kh_val(vmodelptr, idx);
    *name = kh_key(vmodelptr, idx);
    return 0;
}

int vmodel_init()
{
    vmodelptr = &vmodel;

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
    khint_t idx = kh_get(vmodelmap, vmodelptr, key);
    if (idx != kh_end(vmodelptr))
        model = kh_val(vmodelptr, idx);

    return model;
}
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

    for (int i = 0; i < facecount *3; ++i) {
        fastObjIndex vertex = mesh->indices[i];

        int pos = i * 8;
        int vpos = vertex.p * 3;
        int npos = vertex.n * 3;
        int tpos = vertex.t * 2;

        memcpy(&vbuf[pos + 0], mesh->positions + vpos, 3 * sizeof(float));
        memcpy(&vbuf[pos + 3], mesh->normals + npos, 3 * sizeof(float));
        memcpy(&vbuf[pos + 6], mesh->texcoords + tpos, 2 * sizeof(float));
    }
    
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

