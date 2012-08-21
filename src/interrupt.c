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

#include "init.h"
#include "scheduler.h"
#include "exception.h"
#include "syscall.h"


U32 devBaseAddrCalc(U8 line, U8 devNum)
{
	return DEV_REGS_START + (line * 0x80) + (devNum * 0x10);
}
