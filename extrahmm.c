#include "extrahmm.h"
#include <stdbool.h>
#include <stdio.h>

static bool gluInvertMatrix(float m[16], float invOut[16]);

hmm_mat4 extrahmm_transpose_inverse_mat3(hmm_mat4 A)
{
    hmm_mat4 out;

    double determinant =    +A.Elements[0][0]*(A.Elements[1][1]*A.Elements[2][2]-A.Elements[2][1]*A.Elements[1][2])
                            -A.Elements[0][1]*(A.Elements[1][0]*A.Elements[2][2]-A.Elements[1][2]*A.Elements[2][0])
                            +A.Elements[0][2]*(A.Elements[1][0]*A.Elements[2][1]-A.Elements[1][1]*A.Elements[2][0]);
    double invdet = 1/determinant;

    if (invdet == 0)
        return out;

    out.Elements[0][0] =  (A.Elements[1][1]*A.Elements[2][2]-A.Elements[2][1]*A.Elements[1][2])*invdet;
    out.Elements[1][0] = -(A.Elements[0][1]*A.Elements[2][2]-A.Elements[0][2]*A.Elements[2][1])*invdet;
    out.Elements[2][0] =  (A.Elements[0][1]*A.Elements[1][2]-A.Elements[0][2]*A.Elements[1][1])*invdet;
    out.Elements[0][1] = -(A.Elements[1][0]*A.Elements[2][2]-A.Elements[1][2]*A.Elements[2][0])*invdet;
    out.Elements[1][1] =  (A.Elements[0][0]*A.Elements[2][2]-A.Elements[0][2]*A.Elements[2][0])*invdet;
    out.Elements[2][1] = -(A.Elements[0][0]*A.Elements[1][2]-A.Elements[1][0]*A.Elements[0][2])*invdet;
    out.Elements[0][2] =  (A.Elements[1][0]*A.Elements[2][1]-A.Elements[2][0]*A.Elements[1][1])*invdet;
    out.Elements[1][2] = -(A.Elements[0][0]*A.Elements[2][1]-A.Elements[2][0]*A.Elements[0][1])*invdet;
    out.Elements[2][2] =  (A.Elements[0][0]*A.Elements[1][1]-A.Elements[1][0]*A.Elements[0][1])*invdet;

    return out;
}

hmm_mat4 extrahmm_inverse_mat4(hmm_mat4 src)
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

//from mesa
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
