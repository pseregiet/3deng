#ifndef __extrahmm_h
#define __extrahmm_h
#include "hmm.h"
#define hmm_quat hmm_quaternion

hmm_quat quat_calcw(hmm_quat q);
hmm_quat quat_multvec(hmm_quat q, hmm_vec3 v);
hmm_vec3 quat_rotat(hmm_quat q, hmm_vec3 in);

hmm_mat4 extrahmm_transpose_inverse_mat3(hmm_mat4 A);
hmm_mat4 extrahmm_inverse_mat4(hmm_mat4 src);

#endif
