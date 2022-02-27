#include "particle.h"
#include "khash.h"
#include "growing_allocator.h"
#include "json_helpers.h"

#include <assert.h>
#include <stdio.h>

KHASH_MAP_INIT_STR(partimap, struct particle_base)

static struct particles {
    struct growing_alloc names;
    khash_t(partimap) map;
} particles;

static int particle_base_append(struct particle_base *pb, const char *name)
{
    khint_t idx = kh_get(partimap, &particles.map, name);
    if (idx != kh_end(&particles.map)) {
        printf("Particle %s already exists, skip\n", name);
        return 0;
    }

    char *newname = addname(&particles.names, name);
    int ret;
    idx = kh_put(partimap, &particles.map, newname, &ret);
    kh_value(&particles.map, idx) = *pb;

    return 0;
}

static int validate_particle_base(struct particle_base *pb, const char *atlasname)
{
    struct atlas2d *atlas = atlas2d_mngr_find(atlasname);
    if (!atlas) {
        printf("No atlas %s found\n", atlasname);
        return -1;
    }

    if (pb->idx < 0 || pb->count < 0 || pb->fps < 0) {
        printf("Negative values are illegal\n");
        return -1;
    }

    if (pb->idx > atlas->spritecount) {
        printf("Index too large %d vs %d\n", pb->idx, atlas->spritecount);
        return -1;
    }

    if (pb->idx + pb->count - 1 > atlas->spritecount) {
        printf("Count too large %d-%d vs %d\n", pb->idx, pb->count, atlas->spritecount);
        return -1;
    }

    if (pb->count == 0)
        pb->count = 1;

    pb->atlas = atlas;
    return 0;
}

static int particle_base_json()
{
    const char *fn = "particles";
    struct file jf;
    cJSON *json;
    cJSON *root;
    int ret = -1;

    if (json_start(fn, &json, &root, &jf))
        return -1;

    cJSON *part = 0;
    int partid = 0;
    cJSON_ArrayForEach(part, root) {
        struct particle_base pb = {0};
        partid++;
        cJSON *jname = cJSON_GetObjectItem(part, "name");
        cJSON *jatlas = cJSON_GetObjectItem(part, "atlas");
        cJSON *jidx = cJSON_GetObjectItem(part, "index");
        cJSON *jcount = cJSON_GetObjectItem(part, "count");
        cJSON *jfps = cJSON_GetObjectItem(part, "fps");

        if (!jname || !jname->valuestring) {
            printf("%s no name given in node %d\n", fn, partid);
            goto freejson;
        }

        if (!jatlas || !jatlas->valuestring) {
            printf("%s no atlas given in node %d\n", fn, partid);
            goto freejson;
        }

        if (jidx && jidx->type == cJSON_Number)
            pb.idx = (int)jidx->valuedouble;

        if (jcount && jcount->type == cJSON_Number)
            pb.count = (int)jcount->valuedouble;

        if (jfps && jfps->type == cJSON_Number)
            pb.fps = 1.0f / (float)jfps->valuedouble;

        if (validate_particle_base(&pb, jatlas->valuestring)) {
            printf("%s node %d is bad\n", fn, partid);
            goto freejson;
        }

        if (particle_base_append(&pb, jname->valuestring))
            goto freejson;
    }

    ret = 0;
freejson:
    cJSON_Delete(json);
    closefile(&jf);
    return ret;
}

int particle_base_init()
{
    assert(!growing_alloc_init(&particles.names, 0, 1));
    return particle_base_json();
}

void particle_base_kill()
{
    kh_destroy(partimap, &particles.map);
    growing_alloc_kill(&particles.names);
}

struct particle_base *particle_base_find(const char *name)
{
    struct particle_base *ret = 0;
    if (!name)
        return ret;

    khint_t idx = kh_get(partimap, &particles.map, name);
    if (idx != kh_end(&particles.map))
        ret = &kh_val(&particles.map, idx);

    return ret;
}
