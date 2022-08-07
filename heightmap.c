#include "heightmap.h"
#include "fileops.h"
#include "extrahmm.h"

#include <stdio.h>
#include <string.h>

#define SCALEH 600.0f

inline static float get_height(uint16_t pixel)
{
    return ((float)pixel / (float)0xFFFF) * SCALEH;
}

static hmm_vec3 get_normal(uint16_t *gfx, int x, int y, int w, int h)
{
    const float hl = get_height(gfx[ ((y+0)*w) + x-1 ]);
    const float hr = get_height(gfx[ ((y+0)*w) + x+1 ]);
    const float hd = get_height(gfx[ ((y-1)*w) + x-0 ]);
    const float hu = get_height(gfx[ ((y+1)*w) + x-0 ]);

    const hmm_vec3 v3 = {hl-hr, 2.0f, hd-hu};
    return HMM_NormalizeVec3(v3);
}

static void generate_vbuf(float scale, uint16_t *gfx, int stride, float *verts, uint16_t *indices)
{
    int vptr = 0;
    const float vsize = (float)(129 - 1);
    
    const int indices_size = 6 * (129-1) * (129-1);

    const float offsetx = scale;
    const float offsety = scale;

    for (int y = 0; y < 129; ++y) {
        const uint16_t *ptr = &gfx[stride * (y+1)];
        for (int x = 0; x < 129; ++x) {
            const uint16_t pixel = ptr[x+1];
            const hmm_vec3 normal = get_normal(gfx, x+1, y+1, 131, 131);

            //vertices
            verts[vptr * 11 + 0] = ((float)x / vsize) * offsetx;
            verts[vptr * 11 + 1] = get_height(pixel);
            verts[vptr * 11 + 2] = (float)y / vsize * offsety;

            //normals
            verts[vptr * 11 + 3] = normal.X;
            verts[vptr * 11 + 4] = normal.Y;
            verts[vptr * 11 + 5] = normal.Z;

            //UVs
            verts[vptr * 11 + 6] = ((float)x / vsize);
            verts[vptr * 11 + 7] = ((float)y / vsize);

            //tangents
            //verts[vptr * 11 +  8] = 0.0f;
            //verts[vptr * 11 +  9] = 1.0f;
            //verts[vptr * 11 + 10] = 0.0f;
            vptr++;
        }
    }

    vptr = 0;

    for (int y = 0; y < 129-1; ++y) {
        for (int x = 0; x < 129-1; ++x) {
            const int tl = (y * 129) + x;
            const int tr = tl + 1;
            const int bl = (y+1) * 129 + x;
            const int br = bl + 1;

            indices[vptr++] = tl;
            indices[vptr++] = tr;
            indices[vptr++] = bl;
            indices[vptr++] = tr;
            indices[vptr++] = br;
            indices[vptr++] = bl;

        }
    }

    for (int i = 0; i < indices_size; i+=3) {
            const int i0 = indices[i+0];
            const int i1 = indices[i+1];
            const int i2 = indices[i+2];

            const hmm_vec3 *v0pos = (hmm_vec3 *)&verts[i0 * 11 + 0];
            const hmm_vec3 *v1pos = (hmm_vec3 *)&verts[i1 * 11 + 0];
            const hmm_vec3 *v2pos = (hmm_vec3 *)&verts[i2 * 11 + 0];

            const hmm_vec2 *v0uv = (hmm_vec2 *)&verts[i0 * 11 + 6];
            const hmm_vec2 *v1uv = (hmm_vec2 *)&verts[i1 * 11 + 6];
            const hmm_vec2 *v2uv = (hmm_vec2 *)&verts[i2 * 11 + 6];

            const hmm_vec3 t0 = get_tangent(*v0pos, *v1pos, *v2pos, *v0uv, *v1uv, *v2uv);

            verts[i0 * 11 +  8] = t0.X; verts[i0 * 11 +  9] = t0.Y; verts[i0 * 11 + 10] = t0.Z;
            verts[i1 * 11 +  8] = t0.X; verts[i1 * 11 +  9] = t0.Y; verts[i1 * 11 + 10] = t0.Z;
            verts[i2 * 11 +  8] = t0.X; verts[i2 * 11 +  9] = t0.Y; verts[i2 * 11 + 10] = t0.Z;
    }
}

static int load_map(const char *fn, int w, int h, struct worldmap *map) {
    const int maxsize = 131 * 131 * 2;
    const int vbufsize = 129 * 129 * 11;
    const int ibufsize = 128 * 128 * 6;

    const int hbufsize = 131 * 131;
    const int hbufstride = hbufsize * w;

    uint16_t *indices = malloc(ibufsize * w * h * sizeof(uint16_t));
    uint16_t *pixels = malloc(maxsize * w * h);
    float *verts = malloc(vbufsize * w * h * sizeof(float));
    char path[0x1000];
    struct file f;
    int stride = 131;
    int ret = -1;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            snprintf(path, 0x1000, "data/%s/%03d%03d/height.raw", fn, x, y);
            if (openfile(&f, path))
                goto freepixels;

            if (f.usize != maxsize) {
                printf("File %s has weird size %d, expected %d\n", path, f.usize, maxsize);
                closefile(&f);
                goto freepixels;
            }
//this is for editor only. And maybe for collision tests in the future or something
            uint16_t *heightbuf = &pixels[hbufstride * y + (hbufsize * x)];
            map->hmap[y][x] = heightbuf;
            memcpy(heightbuf, f.udata, f.usize);
//----
            closefile(&f);
        }
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int offset = ((y*w) + x);
            uint16_t *pptr = map->hmap[y][x];
            float *vbuf = &verts[vbufsize * offset];
            uint16_t *ibuf = &indices[ibufsize * offset];

            generate_vbuf(map->scale, pptr, stride, vbuf, ibuf);
        }
    }

    int idx = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int offset = ((y*w) + x);

            map->vbuffers[idx] = sg_make_buffer(&(sg_buffer_desc) {
                .data.ptr = &verts[vbufsize * offset],
                .data.size = vbufsize * sizeof(float),
                .type = SG_BUFFERTYPE_VERTEXBUFFER,
                .usage = SG_USAGE_IMMUTABLE,
            });
            
            map->ibuffers[idx] = sg_make_buffer(&(sg_buffer_desc) {
                .data.ptr = &indices[ibufsize * offset],
                .data.size = ibufsize * sizeof(uint16_t),
                .type = SG_BUFFERTYPE_INDEXBUFFER,
                .usage = SG_USAGE_IMMUTABLE,
            });

            idx++;
        }
    }


    ret = 0;

freepixels:
    free(indices);
    free(verts);

    return ret;
}

int worldmap_init(struct worldmap *map, const char *fn)
{
    map->scale = 600.0f;
    //TODO: find size in file
    const int w = 2;
    const int h = 2;

    map->w = w;
    map->h = h;

    if (load_map(fn, w, h, map))
        return -1;

    struct material *bm = material_mngr_find("worldmap/alpha");
    if (!bm || !(bm->flags & MAT_ARRAY) )
        return -1;

    map->blendmap = bm->imgs[MAT_DIFF];

    return 0;
}

bool worldmap_isunder(struct worldmap *map, hmm_vec3 pos)
{
    if (pos.X < 0 || pos.Z < 0)
        return false;
    if (pos.X >= map->scale * map->w)
        return false;
    if (pos.Z >= map->scale * map->h)
        return false;

    float x = pos.X / map->scale;
    float y = pos.Z / map->scale;
    int xi = (int)x;
    int yi = (int)y;
    int xp = (int)((x - xi) * 130);
    int yp = (int)((y - yi) * 130);
    uint16_t *hmap = map->hmap[yi][xi];

    uint16_t hp = hmap[yp * 131 + xp];
    float hf = get_height(hp);// - 50.0f;

    return (pos.Y < hf);
}

