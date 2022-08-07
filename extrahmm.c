#include "extrahmm.h"
#include <stdbool.h>
#include <stdio.h>

hmm_mat4 calc_matrix(hmm_vec3 pos, hmm_vec4 rotation, hmm_vec3 scale)
{
    hmm_mat4 t = HMM_Translate(pos);
    hmm_mat4 r = HMM_Rotate(rotation.W, rotation.XYZ);
    hmm_mat4 s = HMM_Scale(scale);

    hmm_mat4 tmp = HMM_MultiplyMat4(t, r);
    return HMM_MultiplyMat4(tmp, s);
}

hmm_vec3 get_tangent(const hmm_vec3 v0pos, const hmm_vec3 v1pos, const hmm_vec3 v2pos,
        const hmm_vec2 v0uv, const hmm_vec2 v1uv, const hmm_vec2 v2uv) {

    /*if the UV coords are the same their difference will be 0 which will cause a divide by zero
     * in the calculation of f. Not sure how to fix this bug yet and wether it's something to
     * worry about in real models. This breaks the weird 'trump' model I had from online. it was
     * using tiny texture with just few pixels of different colors so most of vertices had the same UV.
     * because all the calculations would go bad the model was mostly rendered black. Putting an if here
     * like this:
     *
     * if (delta_uv0.X == 0.0f && delta_uv0.Y == 0.0f &&
     *     delta_uv1.X == 0.0f && delta_uv1.Y == 0.0f) {
     *  
     *      delta_uv0 = HMM_Vec2(xxx, yyy);
     *      delta_uv1 = HMM_Vec2(xxx, yyy);
     *  }
     *
     *  makes the model render with valid colors but the lighting is broken. Besides, I'm not sure what
     *  values should I use. 1.0f and small number close to 0 (example 0.00001f) is still all black.
     *  random values seem to look ok except lighting is all wrong... well, for now I won't change anything
     *  it doesn't seem to affect any real models.
     */

    const hmm_vec3 edge0 = HMM_SubtractVec3(v1pos, v0pos);
    const hmm_vec3 edge1 = HMM_SubtractVec3(v2pos, v0pos);
    const hmm_vec2 delta_uv0 = HMM_SubtractVec2(v1uv, v0uv);
    const hmm_vec2 delta_uv1 = HMM_SubtractVec2(v2uv, v0uv);

    const float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);
    const float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    const float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    const float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_Vec3(x, y, z);
}

static bool gluInvertMatrix(float m[16], float invOut[16]);

hmm_quat quat_calcw(hmm_quat q)
{
    float t = 1.0f - (q.X * q.X) - (q.Y * q.Y) - (q.Z * q.Z);
    hmm_quat ret = q;
    if (t < 0.0f)
        ret.W = 0.0f;
    else
        ret.W = -HMM_SquareRootF(t);
    return ret;
}

hmm_quat quat_multvec(hmm_quat q, hmm_vec3 v)
{
    hmm_quat out;
    out.W = - (q.X * v.X) - (q.Y * v.Y) - (q.Z * v.Z);
    out.X =   (q.W * v.X) + (q.Y * v.Z) - (q.Z * v.Y);
    out.Y =   (q.W * v.Y) + (q.Z * v.X) - (q.X * v.Z);
    out.Z =   (q.W * v.Z) + (q.X * v.Y) - (q.Y * v.X);
    return out;
}

hmm_vec3 quat_rotat(hmm_quat q, hmm_vec3 in)
{
    hmm_quat inv = {
        .X = -q.X,
        .Y = -q.Y,
        .Z = -q.Z,
        .W =  q.W,
    };

    inv = HMM_NormalizeQuaternion(inv);
    hmm_quat tmp = quat_multvec(q, in);
    hmm_quat fin = HMM_MultiplyQuaternion(tmp, inv);

    return HMM_Vec3(fin.X, fin.Y, fin.Z);
}

hmm_mat4 extrahmm_transpose_inverse_mat3(hmm_mat4 A)
{
    hmm_mat4 out = A;

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
    hmm_mat4 out = src;
    gluInvertMatrix((float *)&src, (float *)&out);
    return out;
}

//from mesa
inline static bool gluInvertMatrix(float m[16], float invOut[16])
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
