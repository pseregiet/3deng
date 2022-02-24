#ifndef __particle_mngr_h
#define __particle_mngr_h

#include "pipelines.h"
#include "particle.h"
#include "particle_emiter.h"

int particle_mngr_init();
void particle_mngr_kill();
void particle_mngr_upload_vbuf();
void particle_mngr_calc_buffers(struct particle_emiter **pes, int count);

void particle_pipeline(struct pipelines *pipes);

#endif

