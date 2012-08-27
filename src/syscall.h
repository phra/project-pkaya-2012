#ifndef __SYSCALL__
#define __SYSCALL__

#include "lib/const11.h"
#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"
#include "phase1/pcb.h"
#include "phase1/asl.h"
#include "lib/utils.h"


#include "scheduler.h"
#include "exception.h"
#include "init.h"
#include "interrupt.h"

void specify_sys_state_vector(void);
void specify_tlb_state_vector(void);
void specifiy_prg_state_vector(void);
void wait_for_io_device(void);
void wait_for_clock(void);
void get_cpu_time(void);
void passeren(void);
void verhogen(void);
void terminate_process(void);
void create_brother(void);
void create_process(void);
#endif
