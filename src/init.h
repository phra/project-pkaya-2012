#ifndef __INIT__
#define __INIT__

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
#include "interrupt.h"

#ifdef __INIT_CONST__
#define MAXCPUs 1
#define MAXPID 1024
#define DEF_PRIORITY 5
#define MAXPRINT 1024

#define INT_OLD 0
#define INT_NEW 1
#define TLB_OLD 2
#define TLB_NEW 3
#define PGMTRAP_OLD 4
#define PGMTRAP_NEW 5
#define SYSBK_OLD 6
#define SYSBK_NEW 7
#endif

void timerHandler(void);
inline void initSemaphore(semd_t* sem, int value);
void print(char *label, char *value);
devreg mytermstat(memaddr *stataddr);

extern semd_t pseudo_clock;
extern semd_t device;
extern pcb_t PIDs[MAXPID];
extern int usedpid;
extern int PROVA;
extern int softBlockCounter;
extern int processCounter;
extern int readyproc;
extern pcb_t* currentproc;
extern U32 mutex_semaphore[MAXPROC];
extern U32 mutex_scheduler;

#endif
