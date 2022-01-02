#include "mouse2world.h"
#include "extrahmm.h"
#include "heightmap.h"
#include <stdio.h>

#define M2_RECURSE_MAX 600
#define M2_MAXRAY 600

#define printvec4(a)\
    printf("%f,%f,%f,%f\n", a.X, a.Y, a.Z, a.W)

#define printvec3(a)\
    printf("%f,%f,%f\n", a.X, a.Y, a.Z)

inline static hmm_vec3 pointray(hmm_vec3 start, hmm_vec3 dir, float distance)
{
    hmm_vec3 tmp = HMM_MultiplyVec3f(dir, distance);
    return HMM_AddVec3(tmp, start);
}

static bool intersect_inrange(struct m2world *m2, float start, float end, hmm_vec3 dir)
{
    hmm_vec3 startp = pointray(m2->cam, dir, start);
    hmm_vec3 endp = pointray(m2->cam, dir, end);
    return (!worldmap_isunder(m2->map, startp) && worldmap_isunder(m2->map, endp));
}

static hmm_vec3 binsearch(struct m2world *m2, int count, float start, float end, hmm_vec3 dir)
{
    float half = start + ((end - start) / 2.0f);
    if (count >= M2_RECURSE_MAX)
        return pointray(m2->cam, dir, half);

    if (intersect_inrange(m2, start, half, dir))
        return binsearch(m2, count + 1, start, half, dir);
    
    return binsearch(m2, count + 1, half, end, dir);
}

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
	if (intersect_inrange(m2, 0, M2_MAXRAY, xd)) {
        return binsearch(m2, 0, 0, M2_MAXRAY, xd);
    }

//not found, return anything
	return xd;
}

