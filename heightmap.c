#include "heightmap.h"
#include "hmm.h"
#include "extrahmm.h"

#include <stdio.h>
#include <string.h>

#define SCALEW 100.0f
#define SCALEH 40.0f

inline static float get_height(uint16_t pixel)
{
    return ((float)pixel / (float)0xFFFF) * SCALEH;
}

static hmm_vec3 get_normal(uint16_t *gfx, int x, int y, int w, int h)
{
    float hl = 0.0f, hr = 0.0f, hd = 0.0f, hu = 0.0f;
    if (x)
        hl = get_height(gfx[ ((y+0)*w) + x-1 ]);
    if (x < w-1)
        hr = get_height(gfx[ ((y+0)*w) + x+1 ]);
    if (y)
        hd = get_height(gfx[ ((y-1)*w) + x-0 ]);
    if (y < h-1)
        hu = get_height(gfx[ ((y+1)*w) + x-0 ]);

    hmm_vec3 v3 = {hl-hr, 2.0f, hd-hu};
    return HMM_NormalizeVec3(v3);
}

hmm_vec3 computeTangent(hmm_vec3 *v0pos, hmm_vec3 *v1pos, hmm_vec3 *v2pos,
        hmm_vec2 *v0uv, hmm_vec2 *v1uv, hmm_vec2 *v2uv) {

    hmm_vec3 edge0 = HMM_SubtractVec3(*v1pos, *v0pos);
    hmm_vec3 edge1 = HMM_SubtractVec3(*v2pos, *v0pos);
    hmm_vec2 delta_uv0 = HMM_SubtractVec2(*v1uv, *v0uv);
    hmm_vec2 delta_uv1 = HMM_SubtractVec2(*v2uv, *v0uv);

    float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);
    float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_Vec3(x, y, z);
}

static void fix_seams_parts_lr(float *a, float *b)
{
    float *ay = a + (130 * 11)+1;
    float *by = b + (0 * 11)+1;

    for (int i = 0; i < 131; ++i) {
        float avg = (*ay + *by)/2.0f;
        *ay = avg;
        *by = avg;

        ay += (131 * 11);
        by += (131 * 11);
    }
}

static void fix_seams_parts_tb(float *a, float *b)
{
    float *ay = a + (131 * 11) * 130 + 1;
    float *by = b + (0 * 11) * 0 + 1;

    for (int i = 0; i < 131; ++i) {
        float avg = (*ay + *by)/2.0f;
        *ay = avg;
        *by = avg;

        ay += 11;
        by += 11;
    }
}

static void fix_seams(struct worldmap *map, float *vbuf)
{
    const int vsize = 131 * 131 * 11;
    const int isize = 130 * 130 * 6;

    for (int y = 0; y < map->h; ++y) {
        for (int x = 0; x < map->w; ++x) {
            float *vbuf1 = &vbuf[vsize * ((y*map->w) + x)];

            if (x < map->w-1) {
                float *vbuf2 = &vbuf[vsize * ((y*map->w) + x + 1)];
                fix_seams_parts_lr(vbuf1, vbuf2);
            }
            if (y < map->h-1) {
                float *vbuf2 = &vbuf[vsize * (((y+1)*map->w) + x)];
                fix_seams_parts_tb(vbuf1, vbuf2);
            }
        }
    }
}
/*
static hmm_vec3 get_normalv(float *gfx, int x, int y, int w, int h)
{
    float hl = 0.0f, hr = 0.0f, hd = 0.0f, hu = 0.0f;
    if (x)
        hl = get_height(gfx[ ((y+0)*w) + x-1 ]);
    if (x < w-1)
        hr = get_height(gfx[ ((y+0)*w) + x+1 ]);
    if (y)
        hd = get_height(gfx[ ((y-1)*w) + x-0 ]);
    if (y < h-1)
        hu = get_height(gfx[ ((y+1)*w) + x-0 ]);

    hmm_vec3 v3 = {hl-hr, 2.0f, hd-hu};
    return HMM_NormalizeVec3(v3);
}

static void fix_mormals_lr(float *a, float *b)
{
    float *ay = a + (130 * 11)+3;
    float *by = b + (0 * 11)+3;

    for (int i = 0; i < 131; ++i) {
        float avg = (*ay + *by)/2.0f;
        *ay = avg;
        *by = avg;

        ay += (131 * 11);
        by += (131 * 11);
    }

}

static void fix_normals(struct worldmap *map, float vbuf)
{
    const int vsize = 131 * 131 * 11;
    const int isize = 130 * 130 * 6;

    for (int y = 0; y < map->h; ++y) {
        for (int x = 0; x < map->w; ++x) {
            float *vbuf1 = &vbuf[vsize * ((y*map->w) + x)];

            if (x < map->w-1) {
                float *vbuf2 = &vbuf[vsize * ((y*map->w) + x + 1)];
                fix_seams_parts_lr(vbuf1, vbuf2);
            }
            if (y < map->h-1) {
                float *vbuf2 = &vbuf[vsize * (((y+1)*map->w) + x)];
                fix_seams_parts_tb(vbuf1, vbuf2);
            }
        }
    }
}
*/

static void generate_vbuf(uint16_t *gfx, int w, int h, float *verts, uint16_t *indices)
{
    int vptr = 0;
    float vsize = (float)h - 1;
    
    //int verts_size = sizeof(float) * 11 * w * h;
    int indices_size = sizeof(uint16_t) * 6 * (h-1) * (h-1);
    //float *verts = malloc(verts_size);
    //uint16_t *indices = malloc(indices_size);

    float offsetx = SCALEW;// * (float)offx;
    float offsety = SCALEW;// * (float)offy;

    for (int y = 0; y < h; ++y) {
        uint16_t *ptr = &gfx[w*y];
        for (int x = 0; x < w; ++x) {
            uint16_t pixel = ptr[x];
            hmm_vec3 normal = get_normal(gfx, x, y, w, h);

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
            verts[vptr * 11 +  8] = 0.0f;
            verts[vptr * 11 +  9] = 1.0f;
            verts[vptr * 11 + 10] = 0.0f;
            vptr++;
        }
    }

    vptr = 0;

    for (int y = 0; y < h-1; ++y) {
        for (int x = 0; x < h-1; ++x) {
            int tl = (y * h) + x;
            int tr = tl + 1;
            int bl = (y+1) * h + x;
            int br = bl + 1;

            indices[vptr++] = tl;
            indices[vptr++] = tr;
            indices[vptr++] = bl;
            indices[vptr++] = tr;
            indices[vptr++] = br;
            indices[vptr++] = bl;

        }
    }

    for (int i = 0; i < indices_size/2; i+=3) {
            int i0 = indices[i+0];
            int i1 = indices[i+1];
            int i2 = indices[i+2];

            hmm_vec3 *v0pos = (hmm_vec3 *)&verts[i0 * 11 + 0];
            hmm_vec3 *v1pos = (hmm_vec3 *)&verts[i1 * 11 + 0];
            hmm_vec3 *v2pos = (hmm_vec3 *)&verts[i2 * 11 + 0];

            hmm_vec2 *v0uv = (hmm_vec2 *)&verts[i0 * 11 + 6];
            hmm_vec2 *v1uv = (hmm_vec2 *)&verts[i1 * 11 + 6];
            hmm_vec2 *v2uv = (hmm_vec2 *)&verts[i2 * 11 + 6];

            hmm_vec3 t0 = computeTangent(v0pos, v1pos, v2pos, v0uv, v1uv, v2uv);

            verts[i0 * 11 +  8] = t0.X; verts[i0 * 11 +  9] = t0.Y; verts[i0 * 11 + 10] = t0.Z;
            verts[i1 * 11 +  8] = t0.X; verts[i1 * 11 +  9] = t0.Y; verts[i1 * 11 + 10] = t0.Z;
            verts[i2 * 11 +  8] = t0.X; verts[i2 * 11 +  9] = t0.Y; verts[i2 * 11 + 10] = t0.Z;
    }
}

static int load_map(const char *fn, int w, int h, struct worldmap *map) {
    const int maxsize = 131 * 131 * 2;
    const int vbufsize = 131 * 131 * 11;
    const int ibufsize = 130 * 130 * 6;

    int bindex = 0;
    int ret = -1;
    char path[0x1000];
    uint16_t *pixels = malloc(maxsize);
    uint16_t *indices = malloc(ibufsize * w * h * sizeof(uint16_t));
    float *verts = malloc(vbufsize * w * h * sizeof(float));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            sprintf(path, "%s/%03d%03d/height.raw", fn, x, y);
            FILE *f = fopen(path, "rb");
            if (!f) {
                printf("cannot open file %s\n", path);
                goto freepixels;
            }
            
            fseek(f, 0, SEEK_END);
            int fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (fsize != maxsize) {
                printf("File %s has weird size %d, expected %d\n", fsize, maxsize);
                fclose(f);
                goto freepixels;
            }

            int fgot = fread(pixels, 1, maxsize, f);
            fclose(f);
            if (fgot != maxsize) {
                printf("read only %d bytes, expected %d\n", fgot, maxsize);
                goto freepixels;
            }

            //sg_buffer *vbuf = &map->vbuffers[bindex];
            //sg_buffer *ibuf = &map->ibuffers[bindex++];
            //generate_vbuf(pixels, 131, 131, x+1, y+1, vbuf, ibuf);
            int offset = ((y*w) + x);
            generate_vbuf(pixels, 131, 131, &verts[vbufsize * offset], &indices[ibufsize * offset]);
        }
    }

    fix_seams(map, verts);

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
    free(pixels);
    free(indices);
    free(verts);

    return ret;
}

int worldmap_init(struct worldmap *map, const char *fn)
{
    //TODO: find size in file
    const int w = 2;
    const int h = 2;

    map->w = w;
    map->h = h;

    if (load_map(fn, w, h, map))
        return -1;

    return 0;
}
