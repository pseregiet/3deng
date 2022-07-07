#include "atlas2d.h"
#include "khash.h"
#include "kvec.h"
#include "json_helpers.h"
#include "growing_allocator.h"
#include "material.h"

#include <assert.h>
#include <stdio.h>

#define DEFAULT_ATLAS_COUNT 16

KHASH_MAP_INIT_STR(atlasmap, struct atlas2d)

static struct atlases {
    struct growing_alloc names;
    khash_t(atlasmap) map;
    kvec_t(sg_buffer) bufs;
} atlases;

static int atlas_append(struct atlas2d *atlas, const char *name)
{
    khint_t idx = kh_get(atlasmap, &atlases.map, name);
    if (idx != kh_end(&atlases.map)) {
        printf("Atlas %s already exists, skip\n", name);
        return 0;
    }

    char *newname = addname(&atlases.names, name);
    int ret;
    idx = kh_put(atlasmap, &atlases.map, newname, &ret);
    kh_value(&atlases.map, idx) = *atlas;

    return 0;
}

static int add_sprites(struct atlas2d *atlas, cJSON *obj, int sprtid)
{
    uint16_t vals[4] = {0};
    if (json_get_array_count(obj) != 4) {
        printf("Invalid sprite coordinates\n");
        return -1;
    }

    int i = 0;
    cJSON *kid = 0;
    cJSON_ArrayForEach(kid, obj)
        vals[i++] = (uint16_t)kid->valuedouble;

    if (vals[0] < 0 || vals[1] < 0 || vals[2] +vals[0] > atlas->width
        || vals[3] + vals[1] > atlas->height) {

        printf("Invalid sprite coordinates\n");
        return -1;
    }

    struct rect *r = &atlas->sprites[sprtid];
    r->x = vals[0];
    r->y = vals[1];
    r->w = vals[2];
    r->h = vals[3];

    return 0;
}

struct vertex {
    uint16_t pos[2];
    uint16_t uv[2];
};

static void make_verts(struct vertex **vbuf, struct atlas2d *atlas, int *vertoffset)
{
    const float totalw = (float)atlas->width;
    const float totalh = (float)atlas->height;

    for (int i = 0; i < atlas->spritecount; ++i) {
        struct vertex *vb = *vbuf;
        const struct rect *rect = &atlas->sprites[i];
        const float fx1 = (float)(rect->x);
        const float fy1 = (float)(rect->y);
        const float fx2 = (float)(rect->x + rect->w);
        const float fy2 = (float)(rect->y + rect->h);
        const uint16_t x1 = (fx1 / totalw) * 0xffff;
        const uint16_t y1 = (fy1 / totalh) * 0xffff;
        const uint16_t x2 = (fx2 / totalw) * 0xffff;
        const uint16_t y2 = (fy2 / totalh) * 0xffff;

        uint16_t vx = 0xffff;
        uint16_t vy = 0xffff;
        if (atlas->sprites[i].w > atlas->sprites[i].h)
            vy *= ((float)rect->h / (float)(rect->w));
        else
            vx *= ((float)rect->w / (float)(rect->h));

        vb[0].pos[0] = 0;
        vb[0].pos[1] = 0;
        vb[0].uv[0] = x1;
        vb[0].uv[1] = y1;

        vb[1].pos[0] = 0;
        vb[1].pos[1] = vy;
        vb[1].uv[0] = x1;
        vb[1].uv[1] = y2;

        vb[2].pos[0] = vx;
        vb[2].pos[1] = 0;
        vb[2].uv[0] = x2;
        vb[2].uv[1] = y1;
        
        vb[3].pos[0] = vx;
        vb[3].pos[1] = vy;
        vb[3].uv[0] = x2;
        vb[3].uv[1] = y2;
        
        *vbuf += 4;
        atlas->sprites[i].voffset = *vertoffset;
        *vertoffset += 4;
    }
}

static void set_buffers(const sg_buffer vb)
{
    khint_t idx = kh_begin(&atlases.map);
    khint_t end = kh_end(&atlases.map);
    for(;idx != end; ++idx) {
        if (!kh_exist(&atlases.map, idx))
            continue;

        struct atlas2d *atlas = &kh_val(&atlases.map, idx);
        atlas->vbuf = vb;
    }
}

static void make_buffers(int spritecount)
{
    struct vertex *vbuf = calloc(sizeof(*vbuf), spritecount * 4);

    khint_t idx = kh_begin(&atlases.map);
    khint_t end = kh_end(&atlases.map);
    
    int vertcount = 0;
    struct vertex *vbufptr = vbuf;
    for(;idx != end; ++idx) {
        if (!kh_exist(&atlases.map, idx))
            continue;

        struct atlas2d *atlas = &kh_val(&atlases.map, idx);
        make_verts(&vbufptr, atlas, &vertcount);
    }

    sg_buffer vb = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = vertcount * sizeof(struct vertex),
        .data.ptr = vbuf,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
    });

    kv_push(sg_buffer, atlases.bufs, vb);
    set_buffers(vb);
    free(vbuf);
}

static int atlas2d_json()
{
    const char *fn = "atlases";
    struct file jf;
    cJSON *json;
    cJSON *root;
    int ret = -1;

    if (json_start(fn, &json, &root, &jf))
        return -1;

    cJSON *atl = 0;
    int atlid = 0;
    int spritecount = 0;
    cJSON_ArrayForEach(atl, root) {
        struct atlas2d atlas = {0};
        atlid++;
        cJSON *jname    = cJSON_GetObjectItem(atl, "name");
        cJSON *jmat     = cJSON_GetObjectItem(atl, "material");
        cJSON *jsprites = cJSON_GetObjectItem(atl, "sprites");

        if (!jname || !jname->valuestring) {
            printf("%s no name given in node %d\n", fn, atlid);
            goto freejson;
        }

        if (!jmat || !jmat->valuestring) {
            printf("%s no material given in node %d\n", fn, atlid);
            goto freejson;
        }

        if (!jsprites || jsprites->type != cJSON_Array) {
            printf("%s no sprites given in node %d\n", fn, atlid);
            goto freejson;
        }

        struct material *realmat = material_mngr_find(jmat->valuestring);
        if (!realmat) {
            printf("%s no material %s found in node %d\n", fn, jmat->valuestring, atlid);
            goto freejson;
        }

        atlas.img = realmat->imgs[MAT_DIFF];
        atlas.width = realmat->width;
        atlas.height = realmat->height;

        atlas.spritecount = json_get_array_count(jsprites);
        atlas.sprites = calloc(atlas.spritecount, sizeof(struct rect));

        spritecount += atlas.spritecount;

        int sprtid = 0;
        cJSON *sprt = 0;
        cJSON_ArrayForEach(sprt, jsprites) {
            if (add_sprites(&atlas, sprt, sprtid++)) {
                printf("%s sprite is bad in node %d\n", fn, atlid);
                free(atlas.sprites);
                goto freejson;
            }
        }

        if (atlas_append(&atlas, jname->valuestring))
            goto freejson;
    }

    make_buffers(spritecount);

    ret = 0;
freejson:
    cJSON_Delete(json);
    closefile(&jf);
    return ret;
}

inline static void atlas2d_kill(struct atlas2d *atlas)
{
    free(atlas->sprites);
}

int atlas2d_mngr_init()
{
    const int default_bufs_count = 10;
    assert(!growing_alloc_init(&atlases.names, 0, 1));
    kv_init(atlases.bufs);
    kv_resize(sg_buffer, atlases.bufs, default_bufs_count);

    memset(&atlases.map, 0, sizeof(atlases.map));
    kh_resize(atlasmap, &atlases.map, DEFAULT_ATLAS_COUNT);
    return atlas2d_json();
}

void atlas2d_mngr_kill()
{
    khint_t i;
    for (i = kh_begin(&atlases.map); i != kh_end(&atlases.map); ++i) {
        if (!kh_exist(&atlases.map, i))
            continue;

        atlas2d_kill(&kh_val(&atlases.map, i));
    }

    kh_destroy(atlasmap, &atlases.map);
    growing_alloc_kill(&atlases.names);

    for (int i = 0; i < kv_size(atlases.bufs); ++i)
        sg_destroy_buffer(kv_A(atlases.bufs, i));

    kv_destroy(atlases.bufs);
}

struct atlas2d *atlas2d_mngr_find(const char *name)
{
    struct atlas2d *ret = 0;
    if (!name)
        return ret;

    khint_t idx = kh_get(atlasmap, &atlases.map, name);
    if (idx != kh_end(&atlases.map))
        ret = &kh_val(&atlases.map, idx);

    return ret;
}
