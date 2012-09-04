#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"
#include "lib/pcb.h"
#include "lib/asl.h"
#include "lib/utils.h"


#include "init.h"
#include "exception.h"
#include "syscall.h"
#include "interrupt.h"


extern struct list_head expiredQueue;
extern struct list_head readyQueue;
extern struct list_head* readyQ;
extern struct list_head* expiredQ;

void scheduler(void);
int pigliapid(void);
pcb_t* allocaPcb(int priority);
void inserisciprocessoready(pcb_t* pcb);
void inserisciprocessoexpired(pcb_t* pcb);
void kill(pcb_t* target);
#endif
