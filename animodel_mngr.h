#ifndef __animodel_mngr_h
#define __animodel_mngr_h
#include "animodel.h"

int animodel_mngr_init();
void animodel_mngr_kill();
void animodel_mngr_calc_boneuv(struct animodel **ams, int count);
void animodel_mngr_upload_bones();
#endif

