#include "heightmap.h"
#include "hmm.h"
#include "extrahmm.h"

#include <stdio.h>
#include <string.h>

inline static float get_height(uint16_t pixel)
{
    return ((float)pixel / (float)0xFFFF) * 600.0f;
}

static hmm_vec3 computeTangent(hmm_vec3 pos0, hmm_vec3 pos1, hmm_vec3 pos2, hmm_vec2 uv0, hmm_vec2 uv1, hmm_vec2 uv2) {
    hmm_vec3 edge0 = HMM_SubtractVec3(pos1, pos0);
    hmm_vec3 edge1 = HMM_SubtractVec3(pos2, pos0);
    hmm_vec2 delta_uv0 = HMM_SubtractVec2(uv1, uv0);
    hmm_vec2 delta_uv1 = HMM_SubtractVec2(uv2, uv0);

    float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);

    float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_Vec3(x, y, z);
}

static hmm_vec3 get_normal(uint16_t *gfx, int x, int y, int w, int h)
{
    float hl, hr, hd, hu = 0.0f;
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

static void generate_vbuf(uint16_t *gfx, int w, int h, int offx, int offy, sg_buffer *vbuf, sg_buffer *ibuf)
{
    int vptr = 0;
    float vsize = (float)h - 1;
    
    int verts_size = sizeof(float) * 11 * w * h;
    int indices_size = sizeof(uint16_t) * 6 * vsize * vsize;
    float *verts = malloc(verts_size);
    uint16_t *indices = malloc(indices_size);

    float offsetx = 50.0f * (float)offx;
    float offsety = 50.0f * (float)offy;

    for (int y = 0; y < h; ++y) {
        uint16_t *ptr = &gfx[w*y];
        for (int x = 0; x < w; ++x) {
            uint16_t pixel = ptr[x];
            hmm_vec3 normal = get_normal(gfx, x, y, w, h);

            //vertices
            verts[vptr * 11 + 0] = ((float)x / vsize) * offsetx - 25.0f;
            verts[vptr * 11 + 1] = get_height(pixel);
            verts[vptr * 11 + 2] = (float)y / vsize * offsety - 25.0f;

            //normals
            verts[vptr * 11 + 3] = normal.X;
            verts[vptr * 11 + 4] = normal.Y;
            verts[vptr * 11 + 5] = normal.Z;

            //UVs
            verts[vptr * 11 + 6] = ((float)x / vsize) * offsetx - 25.0f;
            verts[vptr * 11 + 7] = ((float)y / vsize) * offsety - 25.0f;

            //tangents
            //verts[vptr * 11 + 8] = 0;
            //verts[vptr * 11 + 9] = 0;
            vptr++;
        }
    }

    vptr = 0;

    for (int y = 0; y < h-1; ++y) {
        for (int x = 0; x < h; ++x) {
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

            hmm_vec3 *pos0 = (hmm_vec3 *)&indices[tl * 11 + 0];
            hmm_vec3 *pos1 = (hmm_vec3 *)&indices[tr * 11 + 0];
            hmm_vec3 *pos2 = (hmm_vec3 *)&indices[bl * 11 + 0];
            hmm_vec3 *pos3 = (hmm_vec3 *)&indices[br * 11 + 0];
            hmm_vec2 *uv0  = (hmm_vec2 *)&indices[tl * 11 + 6];
            hmm_vec2 *uv1  = (hmm_vec2 *)&indices[tr * 11 + 6];
            hmm_vec2 *uv2  = (hmm_vec2 *)&indices[bl * 11 + 6];
            hmm_vec2 *uv3  = (hmm_vec2 *)&indices[br * 11 + 6];

            hmm_vec3 ta0 = computeTangent(*pos0, *pos1, *pos2, *uv0, *uv1, *uv2);
            hmm_vec3 ta1 = computeTangent(*pos1, *pos3, *pos2, *uv1, *uv3, *uv2);

            indices[tl * 11 +  8] = ta0.X; indices[tl * 11 +  9] = ta0.Y; indices[tl * 11 + 10] = ta0.Z;
            indices[tr * 11 +  8] = ta0.X; indices[tr * 11 +  9] = ta0.Y; indices[tr * 11 + 10] = ta0.Z;
            indices[bl * 11 +  8] = ta0.X; indices[bl * 11 +  9] = ta0.Y; indices[bl * 11 + 10] = ta0.Z;
            
            indices[tr * 11 +  8] = ta0.X; indices[tr * 11 +  9] = ta0.Y; indices[tr * 11 + 10] = ta0.Z;
            indices[bl * 11 +  8] = ta0.X; indices[bl * 11 +  9] = ta0.Y; indices[bl * 11 + 10] = ta0.Z;
            indices[br * 11 +  8] = ta0.X; indices[br * 11 +  9] = ta0.Y; indices[br * 11 + 10] = ta0.Z;
        }
    }
    
    *vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.ptr = verts,
        .data.size = verts_size,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE,
    });

    *ibuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.ptr = indices,
        .data.size = indices_size,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE,
    });
}

static int load_map(const char *fn, int w, int h, struct worldmap *map) {
    const int maxsize = 131 * 131 * 2;
    int bindex = 0;
    int ret = -1;
    char path[0x1000];
    uint16_t *pixels = malloc(maxsize);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            sprintf(path, "%s/%03d%03d/height.raw", w, h);
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

            int fgot = fread(pixels, 1, maxsize, f) != maxsize;
            fclose(f);
            if (fgot != maxsize) {
                printf("read only %d bytes, expected %d\n", fgot, maxsize);
                goto freepixels;
            }

            sg_buffer *vbuf = &map->vbuffers[bindex];
            sg_buffer *ibuf = &map->ibuffers[bindex++];
            generate_vbuf(pixels, 131, 131, x+1, y+1, vbuf, ibuf);
        }
    }

    ret = 0;

freepixels:
    free(pixels);
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
