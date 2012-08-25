#ifndef __EXCEPTION__
#define __EXCEPTION__

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
#include "init.h"
#include "syscall.h"
#include "interrupt.h"

void sysbk_handler();
void pgmtrap_handler();
void tlb_handler();
void int_handler();
#endif
