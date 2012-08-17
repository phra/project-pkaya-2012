#ifndef __INIT__
#define __INIT__

#include "lib/const11.h"
#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"

void timerHandler();
void inline initSemaphore(semd_t* sem, int value);
void inserisciprocessoready(struct list_head* queue, pcb_t* pcb);
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
extern uint32_t mutex_semaphore[MAXPROC];
extern uint32_t mutex_scheduler;

#endif
