#include "texloader.h"
#include "growing_allocator.h"
#include "fileops.h"
#include "qoi.h"
#include "khash.h"
#include "cJSON.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

KHASH_MAP_INIT_STR(texmap, sg_image)

static struct textures {
    struct growing_alloc names;
    khash_t(texmap) map;
} textures;

struct texture {
    char *name;
    char *fps[10];
    int count;
    enum sg_filter min;
    enum sg_filter mag;
    bool mips;
};

extern sg_image imgdummy;

static sg_image texture_open(struct texture *tex)
{
    if (tex->mips) {
        if (tex->min == SG_FILTER_NEAREST)
            tex->min = SG_FILTER_NEAREST_MIPMAP_NEAREST;
        else
            tex->min = SG_FILTER_NEAREST_MIPMAP_LINEAR;
    }

    struct file f;
    qoi_desc d1;
    qoi_desc d2;
    sg_image ret = {.id = 0};
    char tmp[0x1000];
    snprintf(tmp, 0x1000, "data/materials/%s", tex->fps[0]);

    if (openfile(&f, tmp))
        return ret;

    if (qoi_parse_header(f.udata, f.usize, &d1)) {
        printf("qoi_parse_header failed %s\n", tmp);
        goto freefile;
    }

    const int imgsize = d1.width * d1.height * 4;
    char *buf = malloc(imgsize * tex->count);
    if (qoi_decode(f.udata, buf, f.usize, imgsize, &d1)) {
        printf("qoi_decode failed %s\n", tmp);
        goto freebuf;
    }
    closefile(&f);

    for (int i = 1; i < tex->count; ++i) {
        snprintf(tmp, 0x1000, "data/materials/%s", tex->fps[i]);
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
        .type = tex->count == 1 ? SG_IMAGETYPE_2D : SG_IMAGETYPE_ARRAY,
        .width = d1.width,
        .height = d1.height,
        .num_slices = tex->count,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = tex->min,
        .mag_filter = tex->mag,
        .data.subimage[0][0] = {
            .ptr = buf,
            .size = imgsize * tex->count,
        },
    };

    if (tex->mips)
        ret = sg_make_image_with_mipmaps(&imgdesc);
    else
        ret = sg_make_image(&imgdesc);
    
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

static int texture_append(struct texture *tex)
{
    khint_t idx = kh_get(texmap, &textures.map, tex->name);
    if (idx != kh_end(&textures.map)) {
        printf("Texture %s already exists, skip\n", tex->name);
        return 0;
    }

    sg_image img = texture_open(tex);
    if (img.id == 0)
        return -1;

    char *newname = addname(&textures.names, tex->name);
    int ret;
    idx = kh_put(texmap, &textures.map, newname, &ret);
    kh_value(&textures.map, idx) = img;

    return 0;
}

static int texloader_json() {
    const char *fn = "data/textures.json";
    struct texture texture = {
        .min = SG_FILTER_NEAREST,
        .mag = SG_FILTER_NEAREST,
        .mips = false
    };
    struct file jf;
    int ret = -1;
    if (openfile(&jf, fn))
        return -1;

    cJSON *json = cJSON_ParseWithLength(jf.udata, jf.usize);
    if (!json) {
        printf("cJSON_Parse(%s) failed\n", fn);
        goto freebuf;
    }

    cJSON *root = cJSON_GetObjectItem(json, "textures");
    if (!root || root->type != cJSON_Array) {
        printf("no textures array found\n");
        goto freejson;
    }

    cJSON *tex = 0;
    cJSON_ArrayForEach(tex, root) {
        int texcount = 0;
        cJSON *jname = cJSON_GetObjectItem(tex, "name");
        cJSON *jfiles = cJSON_GetObjectItem(tex, "files");
        cJSON *jmin = cJSON_GetObjectItem(tex, "min");
        cJSON *jmag = cJSON_GetObjectItem(tex, "mag");
        cJSON *jmips = cJSON_GetObjectItem(tex, "mips");

        if (!jname || !jname->valuestring || !jfiles || jfiles->type != cJSON_Array) {
            printf("%s: node %d is bad\n", fn, texcount);
            goto freejson;
        }

        texture.name = jname->valuestring;
        cJSON *jsubtex = 0;
        cJSON_ArrayForEach(jsubtex, jfiles) {
        if (texcount < 10 && jsubtex->valuestring)
            texture.fps[texcount++] = jsubtex->valuestring;
        }

        if (jmin && jmin->valuestring) {
            if (!strcmp(jmin->valuestring, "nearest"))
                texture.min = SG_FILTER_NEAREST;
            else if (!strcmp(jmin->valuestring, "linear"))
                texture.min = SG_FILTER_LINEAR;
        }

        if (jmag && jmag->valuestring) {
            if (!strcmp(jmag->valuestring, "nearest"))
                texture.mag = SG_FILTER_NEAREST;
            else if (!strcmp(jmag->valuestring, "linear"))
                texture.mag = SG_FILTER_LINEAR;
        }

        if (jmips)
            texture.mips = jmips->valueint;

        texture.count = texcount;
        if (texture_append(&texture))
            goto freejson;
    }

    ret = 0;
freejson:
    cJSON_Delete(json);
freebuf:
    closefile(&jf);
    return ret;
}

int texloader_init()
{
    assert(!growing_alloc_init(&textures.names, 0, 1));
    return texloader_json();
}

void texloader_kill()
{
    khint_t i;
    for (i = kh_begin(&textures.map); i != kh_end(&textures.map); ++i) {
        if (!kh_exist(&textures.map, i))
            continue;

        sg_destroy_image(kh_val(&textures.map, i));
    }

    kh_destroy(texmap, &textures.map);
    growing_alloc_kill(&textures.names);
}

sg_image texloader_find(const char *name)
{
    sg_image zero = imgdummy;
    if (!name)
        return zero;

    khint_t idx = kh_get(texmap, &textures.map, name);
    if (idx == kh_end(&textures.map))
        return zero;

    return kh_val(&textures.map, idx);
}

void make_sg_image_16f(float *ptr, int size, int *w, int *h, sg_image *img)
{
    int hi = 0;
    int wi = 0;

    if (size < 4096) {
        wi = size;
        hi = 1;
    }
    else {
        hi = size/4096;
        wi = 4096;
    }

    int bytesize = wi * hi * 8;
    *img = sg_make_image(&(sg_image_desc) {
        .width = wi,
        .height = hi,
        .pixel_format = SG_PIXELFORMAT_RG32F,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .data.subimage[0][0] = {
            .ptr = ptr,
            .size = bytesize,
        },
    });

    *w = wi;
    *h = hi;
}
