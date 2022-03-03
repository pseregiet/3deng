#include "pipelines.h"
#include "objloader.h"
#include "terrain.h"
#include "lightcube.h"
#include "animodel.h"
#include "particle_mngr.h"
#include "wirebox.h"

int pipelines_init(struct pipelines *pipes)
{
    objmodel_pipeline(pipes);
    terrain_pipeline(pipes);
    lightcube_pipeline(pipes);
    animodel_pipeline(pipes);
    animodel_shadow_pipeline(pipes);
    particle_pipeline(pipes);
    wirebox_pipeline(pipes);

    return 0;
}

void pipelines_kill(struct pipelines *pipes)
{
    sg_destroy_pipeline(pipes->objmodel);
    sg_destroy_pipeline(pipes->terrain);
    sg_destroy_pipeline(pipes->lightcube);
    sg_destroy_pipeline(pipes->animodel);
    sg_destroy_pipeline(pipes->animodel_shadow);
    sg_destroy_pipeline(pipes->particle);
    sg_destroy_pipeline(pipes->wirebox);

    sg_destroy_shader(pipes->objmodel_shd);
    sg_destroy_shader(pipes->terrain_shd);
    sg_destroy_shader(pipes->lightcube_shd);
    sg_destroy_shader(pipes->animodel_shd);
    sg_destroy_shader(pipes->animodel_shadow_shd);
    sg_destroy_shader(pipes->particle_shd);
    sg_destroy_shader(pipes->wirebox_shd);
}

