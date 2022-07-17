#include "pipelines.h"
#include "objloader.h"
#include "terrain.h"
#include "animodel.h"
#include "particle_mngr.h"
#include "wirebox.h"
#include "shadow.h"

int pipelines_init(struct pipelines *pipes)
{
    objmodel_pipeline(pipes);
    objmodel_shadow_pipeline(pipes);
    terrain_pipeline(pipes);
    terrain_shadow_pipeline(pipes);
    animodel_pipeline(pipes);
    animodel_shadow_pipeline(pipes);
    particle_pipeline(pipes);
    wirebox_pipeline(pipes);

    return 0;
}

void pipelines_kill(struct pipelines *pipes)
{
    sg_destroy_pipeline(pipes->objmodel);
    sg_destroy_pipeline(pipes->objmodel_shadow);
    sg_destroy_pipeline(pipes->terrain);
    sg_destroy_pipeline(pipes->terrain_shadow);
    sg_destroy_pipeline(pipes->animodel);
    sg_destroy_pipeline(pipes->animodel_shadow);
    sg_destroy_pipeline(pipes->particle);
    sg_destroy_pipeline(pipes->wirebox);
    sg_destroy_pipeline(pipes->objmodel_shadow);
    sg_destroy_pipeline(pipes->terrain_shadow);

    sg_destroy_shader(pipes->objmodel_shd);
    sg_destroy_shader(pipes->objmodel_shadow_shd);
    sg_destroy_shader(pipes->terrain_shd);
    sg_destroy_shader(pipes->terrain_shadow_shd);
    sg_destroy_shader(pipes->animodel_shd);
    sg_destroy_shader(pipes->animodel_shadow_shd);
    sg_destroy_shader(pipes->particle_shd);
    sg_destroy_shader(pipes->wirebox_shd);
}

