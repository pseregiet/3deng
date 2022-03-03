#ifndef __md5bbox_h
#define __md5bbox_h

#include "md5model.h"
#include "hmm.h"

inline static hmm_mat4 calc_bbox_matrix(hmm_vec3 min, hmm_vec3 max)
{
    hmm_vec3 size = HMM_Vec3(
            max.X - min.X,
            max.Y - min.Y,
            max.Z - min.Z
    );
    hmm_vec3 center = HMM_Vec3(
            (min.X + max.X) / 2,
            (min.Y + max.Y) / 2,
            (min.Z + max.Z) / 2
    );

    hmm_mat4 out = HMM_Mat4d(1.0f);
    out.Elements[0][0] = size.X;
    out.Elements[1][1] = size.Y;
    out.Elements[2][2] = size.Z;

    out.Elements[3][0] = center.X;
    out.Elements[3][1] = center.Y;
    out.Elements[3][2] = center.Z;

    /*
    hmm_mat4 rot = HMM_Rotate(90.f, HMM_Vec3(0.5f, 0.0f, 0.0f));

    return HMM_MultiplyMat4(out, rot);
    */

    return out;
}

inline static void md5bbox_calc(struct md5_bbox *bbox, hmm_mat4 *outbuf, hmm_mat4 matrix)
{
    *outbuf = HMM_MultiplyMat4(matrix, calc_bbox_matrix(bbox->min, bbox->max));
}

#endif
