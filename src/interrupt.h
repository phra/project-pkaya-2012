#ifndef __INTERRUPT__
#define __INTERRUPT__

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
#include "syscall.h"
#include "init.h"

#ifndef __INT_MACROS__
#define __INT_MACROS__
#define devBaseAddrCalc(line,devNum) (dtpreg_t*)DEV_REGS_START + (line * 0x80) + (devNum * 0x10)
#define bitmapCalc(line) *((int*)(PENDING_BITMAP_START + (line - 3) * WORD_SIZE));
#endif

extern U32 devstatus[DEV_USED_INTS][DEV_PER_INT];

void deviceHandler(U32 intline);
void _verhogen(int semkey, int* status);
void _passeren(int semkey);

#endif
