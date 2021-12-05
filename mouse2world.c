#include "mouse2world.h"
#include "extrahmm.h"
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
	hmm_mat4 invproject = extrahmm_inverse_mat4(m2->projection);
	hmm_vec4 viewcoords = HMM_MultiplyMat4ByVec4(invproject, rayclip);
	viewcoords = HMM_Vec4(viewcoords.X, viewcoords.Y, -1.0f, 0.0f);
	hmm_mat4 invview = extrahmm_inverse_mat4(m2->view);
	hmm_vec4 rayworld = HMM_MultiplyMat4ByVec4(invview, viewcoords);

	hmm_vec3 xd = HMM_NormalizeVec3(HMM_Vec3(rayworld.X, rayworld.Y, rayworld.Z));
	return xd;
}


