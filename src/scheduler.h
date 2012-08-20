#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "lib/const11.h"
#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"

void scheduler();
int pigliapid();
extern struct list_head expiredQueue;
extern struct list_head readyQueue;
extern struct list_head* readyQ = &readyQueue;
extern struct list_head* expiredQ = &expiredQueue;
pcb_t* allocaPcb(int priority);
#endif
