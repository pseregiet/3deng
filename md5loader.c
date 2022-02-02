#include "md5loader.h"
#include "md5model.h"
#include "growing_allocator.h"
#include "fileops.h"
#include "khash.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>

KHASH_MAP_INIT_STR(modelmap, struct md5_model)
KHASH_MAP_INIT_STR(animmap, struct md5_anim)

static struct animations {
    struct growing_alloc names;
    khash_t(animmap) map;
} animations;

static struct models {
    struct growing_alloc names;
    khash_t(modelmap) map;
} models;

#define MODELS_PER_POOL 10
#define MODELS_POOL_COUNT 1
#define ANIMS_PER_POOL 10
#define ANIMS_POOL_COUNT 1

inline static char *addname(struct growing_alloc *alloc, const char *name)
{
    int namelen = strlen(name);
    char *newname = growing_alloc_get(alloc, namelen+1);
    assert(newname);
    memcpy(newname, name, namelen);
    newname[namelen] = 0;
    return newname;
}

static struct md5_model *model_append(const char *fp, const char *name)
{
    struct md5_model mdl;
    if (md5model_open(fp, &mdl))
        return 0;

    char *newname = addname(&models.names, name);
    int ret;
    khint_t idx = kh_put(modelmap, &models.map, newname, &ret);
    kh_value(&models.map, idx) = mdl;
    return &kh_value(&models.map, idx);
}

static khint_t animation_append(const char *fp, const char *name)
{
    struct md5_anim anim;
    if (md5anim_open(fp, &anim))
        return 0;

    char *newname = addname(&models.names, name);
    int ret;
    khint_t idx = kh_put(animmap, &animations.map, newname, &ret);
    kh_value(&animations.map, idx) = anim;

    return idx;
}

union aniunio{
    struct md5_anim *a;
    khint_t i;
};
static int load_model_anims(const char* model, const char **animfps, int ac)
{
    if (!model || !animfps)
        return -1;

    char fp[0x1000];
    snprintf(fp, 0x1000, "md5/models/%s.md5mesh", model);
    khint_t idx = kh_get(modelmap, &models.map, model);

    if (idx != kh_end(&models.map)) {
        printf("Model %s already exists, skip\n");
        return 0;
    }

    struct md5_model *mdl = model_append(fp, model);
    if (!mdl)
        return -1;

    union aniunio *anims = calloc(ac, sizeof(union aniunio));
    for (int i = 0; i < ac; ++i) {
        idx = kh_get(animmap, &animations.map, animfps[i]);
        if (idx != kh_end(&animations.map)) {
            anims[i].i = idx;
            continue;
        }

        snprintf(fp, 0x1000, "md5/models/%s.md5anim", animfps[i]);
        anims[i].i = animation_append(fp, animfps[i]);
        if (anims[i].i == kh_end(&animations.map))
            return -1;
    }
    mdl->anims = (struct md5_anim **)anims;
    mdl->acount = ac;

    return 0;
}

static void fixup_animations_pointers() {
    khint_t mi;

    for (mi = kh_begin(&models.map); mi != kh_end(&models.map); ++mi) {
        if (!kh_exist(&models.map, mi))
            continue;

        struct md5_model *mdl = &kh_val(&models.map, mi);
        for (int i = 0; i < mdl->acount; ++i) {
            union aniunio au = {
                .a = mdl->anims[i]
            };
            mdl->anims[i] = &kh_val(&animations.map, au.i);
        }
    }
}

static int md5models_json() {
    const char *fn = "md5models.json";
    struct file jf;
    int ret = -1;
    if (openfile(&jf, fn))
        return -1;

    cJSON *json = cJSON_ParseWithLength(jf.udata, jf.usize);
    if (!json) {
        printf("cJSON_Parse(%s) failed\n", fn);
        goto freebuf;
    }

    cJSON *root = cJSON_GetObjectItem(json, "md5models");
    if (!root || root->type != cJSON_Array) {
        printf("%s: no md5models array found\n");
        goto freejson;
    }

    cJSON *mdl = 0;
    int mdlcount = 0;
    cJSON_ArrayForEach(mdl, root) {
        char *model = 0;
        char *anims[100] = {0};
        mdlcount++;

        cJSON *jmodel = cJSON_GetObjectItem(mdl, "model");
        cJSON *janims = cJSON_GetObjectItem(mdl, "anims");

        if (!jmodel || !janims) {
            printf("%s: node %d is bad\n", fn, mdlcount);
            goto freejson;
        }

        model = jmodel->valuestring;
        cJSON *a = 0;
        int ac = 0;
        cJSON_ArrayForEach(a, janims) {
            if (ac < 100 && a->valuestring)
                anims[ac++] = a->valuestring;
        }

        if (load_model_anims(model, (const char **)&anims, ac))
            goto freejson;
    }

    fixup_animations_pointers();

    ret = 0;
freejson:
    cJSON_Delete(json);
freebuf:
    closefile(&jf);
    return ret;
}

struct md5_model *md5loader_find(const char *name)
{
    khint_t idx = kh_get(modelmap, &models.map, name);
    if (idx == kh_end(&models.map))
        return 0;

    return &kh_val(&models.map, idx);
}

int md5loader_init()
{
    const int ms = sizeof(struct md5_model);
    const int as = sizeof(struct md5_anim);
    assert(!growing_alloc_init(&models.names, 0, 1));
    assert(!growing_alloc_init(&animations.names, 0, 1));
    return md5models_json();
}

void md5loader_kill()
{
    khint_t i;
    for (i = kh_begin(&models.map); i != kh_end(&models.map); ++i) {
        if (!kh_exist(&models.map, i))
            continue;

        md5model_kill(&kh_val(&models.map, i));
    }

    for (i = kh_begin(&animations.map); i != kh_end(&animations.map); ++i) {
        if (!kh_exist(&animations.map, i))
            continue;

        md5anim_kill(&kh_val(&animations.map, i));
    }
    kh_destroy(modelmap, &models.map);
    kh_destroy(animmap, &animations.map);

    growing_alloc_kill(&models.names);
    growing_alloc_kill(&animations.names);
}

