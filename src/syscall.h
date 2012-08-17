#ifndef __SYSCALL__
#define __SYSCALL__

#include "lib/const11.h"
#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"

void specify_sys_state_vector();
void specify_tlb_state_vector();
void specifiy_prg_state_vector();
void wait_for_io_device();
void wait_for_clock();
void get_cpu_time();
void passeren();
void verhogen();
void terminate_process();
void create_brother();
void create_process();
#endif
