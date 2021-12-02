#include "mouse2world.h"
#include <stdio.h>

static hmm_mat4 myhmm_inverse(hmm_mat4 src);
static bool gluInvertMatrix(float m[16], float invOut[16]);

#define printvec4(a)\
    printf("%f,%f,%f,%f\n", a.X, a.Y, a.Z, a.W)

#define printvec3(a)\
    printf("%f,%f,%f\n", a.X, a.Y, a.Z)

hmm_vec3 mouse2ray(struct m2world *m2)
{
    float fmx = (2.0f * (float)m2->mx) / m2->ww - 1.0f;
	float fmy = 1.0f - (2.0f * (float)m2->my) / m2->wh;

	hmm_vec4 rayclip = HMM_Vec4(fmx, fmy, -1.0f, 1.0f);
	hmm_mat4 invproject = myhmm_inverse(m2->projection);
	hmm_vec4 viewcoords = HMM_MultiplyMat4ByVec4(invproject, rayclip);
	viewcoords = HMM_Vec4(viewcoords.X, viewcoords.Y, -1.0f, 0.0f);
	hmm_mat4 invview = myhmm_inverse(m2->view);
	hmm_vec4 rayworld = HMM_MultiplyMat4ByVec4(invview, viewcoords);

	hmm_vec3 xd = HMM_NormalizeVec3(HMM_Vec3(rayworld.X, rayworld.Y, rayworld.Z));
	return xd;
}

static hmm_mat4 myhmm_inverse(hmm_mat4 src)
{
    hmm_mat4 out;
    float fi[16];
    float fo[16];

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            fi[y*4+x] = src.Elements[y][x];

    if (!gluInvertMatrix(fi, fo))
        printf("wtf\n");

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            out.Elements[y][x] = fo[y*4+x];

    return out;
}

static bool gluInvertMatrix(float m[16], float invOut[16])
{
    float inv[16], det;
    int i;
 
    inv[ 0] =  m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[ 4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[ 8] =  m[4] * m[ 9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[ 9];
    inv[12] = -m[4] * m[ 9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[ 9];
    inv[ 1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[ 5] =  m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[ 9] = -m[0] * m[ 9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[ 9];
    inv[13] =  m[0] * m[ 9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[ 9];
    inv[ 2] =  m[1] * m[ 6] * m[15] - m[1] * m[ 7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[ 7] - m[13] * m[3] * m[ 6];
    inv[ 6] = -m[0] * m[ 6] * m[15] + m[0] * m[ 7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[ 7] + m[12] * m[3] * m[ 6];
    inv[10] =  m[0] * m[ 5] * m[15] - m[0] * m[ 7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[ 7] - m[12] * m[3] * m[ 5];
    inv[14] = -m[0] * m[ 5] * m[14] + m[0] * m[ 6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[ 6] + m[12] * m[2] * m[ 5];
    inv[ 3] = -m[1] * m[ 6] * m[11] + m[1] * m[ 7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[ 9] * m[2] * m[ 7] + m[ 9] * m[3] * m[ 6];
    inv[ 7] =  m[0] * m[ 6] * m[11] - m[0] * m[ 7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[ 8] * m[2] * m[ 7] - m[ 8] * m[3] * m[ 6];
    inv[11] = -m[0] * m[ 5] * m[11] + m[0] * m[ 7] * m[ 9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[ 9] - m[ 8] * m[1] * m[ 7] + m[ 8] * m[3] * m[ 5];
    inv[15] =  m[0] * m[ 5] * m[10] - m[0] * m[ 6] * m[ 9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[ 9] + m[ 8] * m[1] * m[ 6] - m[ 8] * m[2] * m[ 5];
 
    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
 
    if(det == 0)
        return false;
 
    det = 1.f / det;
 
    for(i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;
 
    return true;
}

