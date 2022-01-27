#ifndef __extrahmm_h
#define __extrahmm_h
#include "hmm.h"
#define hmm_quat hmm_quaternion

hmm_mat4 calc_matrix(hmm_vec3 pos, hmm_vec4 rotation, hmm_vec3 scale);
hmm_vec3 get_tangent(hmm_vec3 *v0pos, hmm_vec3 *v1pos, hmm_vec3 *v2pos,
        hmm_vec2 *v0uv, hmm_vec2 *v1uv, hmm_vec2 *v2uv);

hmm_quat quat_calcw(hmm_quat q);
hmm_quat quat_multvec(hmm_quat q, hmm_vec3 v);
hmm_vec3 quat_rotat(hmm_quat q, hmm_vec3 in);

hmm_mat4 extrahmm_transpose_inverse_mat3(hmm_mat4 A);
hmm_mat4 extrahmm_inverse_mat4(hmm_mat4 src);

#endif
