#ifndef __SCHEDULER__
#define __SCHEDULER__

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
#include "exception.h"
#include "syscall.h"
#include "interrupt.h"

void scheduler(void);
int pigliapid(void);
extern struct list_head expiredQueue;
extern struct list_head readyQueue;
extern struct list_head* readyQ = &readyQueue;
extern struct list_head* expiredQ = &expiredQueue;
pcb_t* allocaPcb(int priority);
void inserisciprocessoready(pcb_t* pcb);
void inserisciprocesso(struct list_head* queue, pcb_t* pcb);
void kill(pcb_t* target);
#endif
