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


U32 devBaseAddrCalc(U8 line, U8 devNum);
void deviceHandler(unsigned int intline);
void _verhogen(int semkey);
void _passeren(int semkey);

#endif
