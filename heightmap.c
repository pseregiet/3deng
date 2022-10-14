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

    const hmm_vec3 v3 = {{hl-hr, 2.0f, hd-hu}};
    return HMM_NormalizeVec3(v3);
}

struct vertex {
    hmm_vec3 pos;
    hmm_vec3 normal;
    hmm_vec2 uv;
    hmm_vec3 tangent;
};

static void generate_vbuf(float scale, uint16_t *gfx, int stride, struct vertex *verts, uint16_t *indices)
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
            verts[vptr].pos.X = ((float)x / vsize) * offsetx;
            verts[vptr].pos.Y = get_height(pixel);
            verts[vptr].pos.Z = (float)y / vsize * offsety;

            //normals
            verts[vptr].normal.X = normal.X;
            verts[vptr].normal.Y = normal.Y;
            verts[vptr].normal.Z = normal.Z;

            //UVs
            verts[vptr].uv.U = ((float)x / vsize);
            verts[vptr].uv.V = ((float)y / vsize);

            //tangents
            //verts[vptr].tangent.X = 0.0f;
            //verts[vptr].tangent.Y = 1.0f;
            //verts[vptr].tangent.Z = 0.0f;
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

            const hmm_vec3 *v0pos = &verts[i0].pos;
            const hmm_vec3 *v1pos = &verts[i1].pos;
            const hmm_vec3 *v2pos = &verts[i2].pos;

            const hmm_vec2 *v0uv = &verts[i0].uv;
            const hmm_vec2 *v1uv = &verts[i1].uv;
            const hmm_vec2 *v2uv = &verts[i2].uv;

            const hmm_vec3 t0 = get_tangent(*v0pos, *v1pos, *v2pos, *v0uv, *v1uv, *v2uv);

            verts[i0].tangent.X = t0.X; verts[i0].tangent.Y = t0.Y; verts[i0].tangent.Z = t0.Z;
            verts[i1].tangent.X = t0.X; verts[i1].tangent.Y = t0.Y; verts[i1].tangent.Z = t0.Z;
            verts[i2].tangent.X = t0.X; verts[i2].tangent.Y = t0.Y; verts[i2].tangent.Z = t0.Z;
    }
}

static int load_map(const char *fn, int w, int h, struct worldmap *map) {
    const int maxsize = 131 * 131 * 2;
    const int vbufsize = 129 * 129;
    const int ibufsize = 128 * 128 * 6;

    const int hbufsize = 131 * 131;
    const int hbufstride = hbufsize * w;

    uint16_t *indices = malloc(ibufsize * w * h * sizeof(uint16_t));
    uint16_t *pixels = malloc(maxsize * w * h);
    struct vertex *verts = malloc(vbufsize * w * h * sizeof(*verts));
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
            struct vertex *vbuf = &verts[vbufsize * offset];
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
                .data.size = vbufsize * sizeof(struct vertex),
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

