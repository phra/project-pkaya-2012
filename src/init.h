#ifndef __INIT__
#define __INIT__

#include "lib/const11.h"
#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"
#include "lib/pcb.h"
#include "lib/asl.h"
#include "lib/utils.h"


#include "scheduler.h"
#include "exception.h"
#include "syscall.h"
#include "interrupt.h"


#define MAXCPUs 1
#define MAXPID 128
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
#define TRANSMITTED	5
#define TRANSTATUS    2
#define ACK	1
#define PRINTCHR	2
#define CHAROFFSET	8
#define STATUSMASK	0xFF
#define	TERM0ADDR	0x10000250
#define DEVREGSIZE 16       
#define READY     1
#define DEVREGLEN   4
#define BUSY      3

#define RECVSTATUS 0
#define RECVCOMMAND 1
#define TRANSTATUS    2
#define TRANCOMMAND   3



extern int processCounter;
extern int softBlockCounter;
extern int readyproc;
extern int PROVA;
extern int usedpid;
extern pcb_t* currentproc[MAXCPUs];
extern pcb_t* PIDs[MAXPID];
extern pcb_t* wait_clock[MAXPROC];
extern state_t* new_old_areas[MAXCPUs][8];
extern state_t real_new_old_areas[MAXCPUs-1][8];
extern U32 mutex_semaphore[MAXPROC+MAX_DEVICES+1];
extern U32 mutex_scheduler;
extern U32 mutex_wait_clock;

void timerHandler(void);
inline void initSemaphore(semd_t* sem, int value);
void print(char *label, char *value);
U32 mytermstat(memaddr *stataddr);
void myprintint(char *str1, int numero);
void myprintbin(char *str1, int numero);
void myprinthex(char *str1, int numero);
void myprint(char *str1);

#endif
