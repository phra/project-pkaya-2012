#ifndef __ASL__
#define __ASL__

#include "../lib/const11.h"
#include "../lib/const.h"
#include "../lib/base.h"
#include "../lib/uMPStypes.h"
#include "../lib/libumps.h"
#include "../lib/listx.h"
#include "../lib/types11.h"

void outChildBlocked(pcb_t *p);
pcb_t *headBlocked(int key);
pcb_t *outBlocked(pcb_t *p);
pcb_t *removeBlocked(int key);
int insertBlocked(int key,pcb_t *p);
semd_t *getSemd(int key);
void initASL(void);
extern struct list_head semd_h;
extern semd_t semd_table[MAXPROC];
extern struct list_head semdFree_h;
#endif
