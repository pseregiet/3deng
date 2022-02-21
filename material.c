#include "material.h"
#include "growing_allocator.h"
#include "fileops.h"
#include "qoi.h"
#include "cJSON.h"
#include "khash.h"

#include <stdio.h>
#include <assert.h>

KHASH_MAP_INIT_STR(matmap, struct material)

static struct materials {
    struct growing_alloc names;
    khash_t(matmap) map;
} materials;

struct tmpmat {
    char *name;
    char **fns;
    int count;
    bool min_filter;
    bool mag_filter;
    bool mips;
};

static int json_get_array_count(cJSON *obj)
{
    int count = 0;
    cJSON *tmp;
    cJSON_ArrayForEach(tmp, obj)
        count++;

    return count;
}

static void copy_fns(char **fns, cJSON *obj)
{
    int count = 0;
    cJSON *tmp;
    cJSON_ArrayForEach(tmp, obj)
        fns[count++] = tmp->valuestring;
}

static int texture_load(struct tmpmat *mat, sg_image *img)
{
    enum sg_filter min = mat->min_filter ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
    enum sg_filter mag = mat->mag_filter ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;

    if (mat->mips) {
        min = min == SG_FILTER_NEAREST ?
            SG_FILTER_NEAREST_MIPMAP_NEAREST : SG_FILTER_NEAREST_MIPMAP_LINEAR;
    }

    struct file f;
    qoi_desc d1;
    qoi_desc d2;
    int ret = -1;
    char tmp[0x1000];
    snprintf(tmp, 0x1000, "data/materials/%s", mat->fns[0]);

    if (openfile(&f, tmp))
        return ret;

    if (qoi_parse_header(f.udata, f.usize, &d1)) {
        printf("qoi_parse_header failed %s\n", tmp);
        goto freefile;
    }

    const int imgsize = d1.width * d1.height * 4;
    char *buf = malloc(imgsize * mat->count);
    if (qoi_decode(f.udata, buf, f.usize, imgsize, &d1)) {
        printf("qoi_decode failed %s\n", tmp);
        goto freebuf;
    }
    closefile(&f);

    for (int i = 1; i < mat->count; ++i) {
        snprintf(tmp, 0x1000, "data/materials/%s", mat->fns[i]);
        if (openfile(&f, tmp)) {
            free(buf);
            return ret;
        }

        if (qoi_parse_header(f.udata, f.usize, &d2)) {
            printf("qoi_parse_header failed %s\n", tmp);
            goto freebuf;
        }

        if (d1.width != d2.width || d1.height != d2.height) {
            printf("Image has bad image for array %s\n", tmp);
            goto freebuf;
        }

        if (qoi_decode(f.udata, &buf[imgsize * i], f.usize, imgsize, &d2)) {
            printf("qoi_decode failed %s\n", tmp);
            goto freebuf;
        }
        closefile(&f);
    }

    sg_image_desc imgdesc = {
        .type = mat->count == 1 ? SG_IMAGETYPE_2D : SG_IMAGETYPE_ARRAY,
        .width = d1.width,
        .height = d1.height,
        .num_slices = mat->count,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = min,
        .mag_filter = mag,
        .data.subimage[0][0] = {
            .ptr = buf,
            .size = imgsize * mat->count,
        },
    };

    if (mat->mips)
        *img = sg_make_image_with_mipmaps(&imgdesc);
    else
        *img = sg_make_image(&imgdesc);

    ret = 0;
    free(buf);
    return ret;
//-------------
freebuf:
    free(buf);
freefile:
    closefile(&f);
    return ret;
}

inline static char *addname(struct growing_alloc *alloc, const char *name)
{
    int namelen = strlen(name);
    char *newname = growing_alloc_get(alloc, namelen+1);
    assert(newname);
    memcpy(newname, name, namelen);
    newname[namelen] = 0;
    return newname;
}

static int material_append(struct tmpmat *tmpmat, struct material *finalmat)
{
    khint_t idx = kh_get(matmap, &materials.map, tmpmat->name);
    if (idx != kh_end(&materials.map)) {
        printf("Material %s already exists, skip\n", tmpmat->name);
        return 0;
    }

    char *newname = addname(&materials.names, tmpmat->name);
    int ret;
    idx = kh_put(matmap, &materials.map, newname, &ret);
    kh_value(&materials.map, idx) = *finalmat;

    return 0;
}

static void material_kill(struct material *mat)
{
    if (!mat)
        return;

    if (mat->flags & MAT_ARRAY)
        free(mat->shine.array);

    for (int i = 0; i < MAT_COUNT; ++i)
        if (mat->imgs[i].id)
            sg_destroy_image(mat->imgs[i]);
}

static int material_mngr_json()
{
    const char *fn = "data/materials.json";
    struct material *finalptr;
    struct tmpmat tmpmat = {0};
    struct file jf;
    int ret = -1;
    if (openfile(&jf, fn))
        return -1;

    cJSON *json = cJSON_ParseWithLength(jf.udata, jf.usize);
    if (!json) {
        printf("cJSON_Parse(%s) failed\n", fn);
        goto freebuf;
    }

    cJSON *root = cJSON_GetObjectItem(json, "materials");
    if (!root || root->type != cJSON_Array) {
        printf("no materials array found\n");
        goto freejson;
    }

    cJSON *mat = 0;
    int matid = 0;
    cJSON_ArrayForEach(mat, root) {
        struct material finalmat = {0};
        finalptr = &finalmat;
        matid++;
        cJSON *jname = cJSON_GetObjectItem(mat, "name");
        cJSON *jdiff = cJSON_GetObjectItem(mat, "diff");
        cJSON *jspec = cJSON_GetObjectItem(mat, "spec");
        cJSON *jnorm = cJSON_GetObjectItem(mat, "norm");
        cJSON *jhght = cJSON_GetObjectItem(mat, "hght");
        cJSON *jshin = cJSON_GetObjectItem(mat, "shine");
        cJSON *jmin  = cJSON_GetObjectItem(mat, "min");
        cJSON *jmag  = cJSON_GetObjectItem(mat, "mag");
        cJSON *jmips = cJSON_GetObjectItem(mat, "mips");

        if (!jname || !jname->valuestring) {
            printf("%s no name given in node %d\n", fn, matid);
            goto freejson;
        }

        if (!jdiff || jdiff->type != cJSON_Array) {
            printf("%s no diff given in node %d\n", fn, matid);
            goto freejson;
        }

        int diffcount = json_get_array_count(jdiff);
        if (jspec && jspec->type == cJSON_Array) {
            if (json_get_array_count(jspec) != diffcount) {
                printf("%s spec count different from diff count in node %d\n", fn, matid);
                goto freejson;
            }
        }

        if (jnorm && jnorm->type == cJSON_Array) {
            if (json_get_array_count(jnorm) != diffcount) {
                printf("%s norm count different from diff count in node %d\n", fn, matid);
                goto freejson;
            }
        }

        if (jhght && jhght->type == cJSON_Array) {
            if (json_get_array_count(jhght) != diffcount) {
                printf("%s hght count different from diff count in node %d\n", fn, matid);
                goto freejson;
            }
        }

        if (jshin && jshin->type == cJSON_Array) {
            if (json_get_array_count(jshin) != diffcount) {
                printf("%s shine count different from diff count in node %d\n", fn, matid);
                goto freejson;
            }
        }

        tmpmat.name = jname->valuestring;
        tmpmat.mips = (jmips != 0 && jmips->valueint);
        tmpmat.min_filter = (jmin != 0 && jmin->valueint);
        tmpmat.mag_filter = (jmag != 0 && jmag->valueint);
        if (tmpmat.fns == 0) {
            tmpmat.fns = malloc(sizeof(char *) * diffcount);
        }
        else {
           if (tmpmat.count < diffcount)
               tmpmat.fns = realloc(tmpmat.fns, sizeof(char *) * diffcount);
        }

        tmpmat.count = diffcount;
        copy_fns(tmpmat.fns, jdiff);

        if (diffcount > 1)
            finalmat.flags |= MAT_ARRAY;

        if (texture_load(&tmpmat, &finalmat.imgs[MAT_DIFF]))
            goto freetmpmat;

        finalmat.flags |= MAT_HAS_DIFF;

        if (jspec) {
            copy_fns(tmpmat.fns, jspec);
            if (texture_load(&tmpmat, &finalmat.imgs[MAT_SPEC]))
                goto freetmpmat;

            finalmat.flags |= MAT_HAS_SPEC;
        }

        if (jnorm) {
            copy_fns(tmpmat.fns, jnorm);
            if (texture_load(&tmpmat, &finalmat.imgs[MAT_NORM]))
                goto freetmpmat;

            finalmat.flags |= MAT_HAS_NORM;
        }

        if (jhght) {
            copy_fns(tmpmat.fns, jhght);
            if (texture_load(&tmpmat, &finalmat.imgs[MAT_HGHT]))
                goto freetmpmat;

            finalmat.flags |= MAT_HAS_HGHT;
        }

        if (diffcount > 1)
            finalmat.shine.array = calloc(sizeof(float), diffcount);
        else
            finalmat.shine.value = 0.0f;

        if (jshin) {
            if (tmpmat.count == 1) {
                finalmat.shine.value = jshin->child->valuedouble;
            }
            else {
                cJSON *tmp;
                int c = 0;
                cJSON_ArrayForEach(tmp, jshin)
                    finalmat.shine.array[c++] = tmp->valuedouble;
            }
        }

        finalmat.count = diffcount;
        material_append(&tmpmat, &finalmat);
    }

    ret = 0;
    finalptr = 0;
freetmpmat:
    if (tmpmat.fns)
        free(tmpmat.fns);

    material_kill(finalptr);
freejson:
    cJSON_Delete(json);
freebuf:
    closefile(&jf);
    return ret;
}

extern sg_image imgdummy;
extern sg_image imgdummy_norm;
void material_init_dummy(struct material *mat)
{
    mat->imgs[MAT_DIFF] = imgdummy;
    mat->imgs[MAT_SPEC] = imgdummy;
    mat->imgs[MAT_HGHT] = imgdummy;
    mat->imgs[MAT_NORM] = imgdummy_norm;
    mat->shine.value = 0.0f;
    mat->count = 1;
}

int material_mngr_init()
{
    assert(!growing_alloc_init(&materials.names, 0, 1));
    return material_mngr_json();
}

void material_mngr_kill()
{
    khint_t i;
    for (i = kh_begin(&materials.map); i != kh_end(&materials.map); ++i) {
        if (!kh_exist(&materials.map, i))
            continue;

        material_kill(&kh_val(&materials.map, i));
    }

    kh_destroy(matmap, &materials.map);
    growing_alloc_kill(&materials.names);
}

struct material *material_mngr_find(const char *name)
{
    struct material *ret = 0;
    if (!name)
        return ret;

    khint_t idx = kh_get(matmap, &materials.map, name);
    if (idx != kh_end(&materials.map))
        ret = &kh_val(&materials.map, idx);

    return ret;
}
