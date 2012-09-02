#ifndef __ASL__
#define __ASL__

#include "const11.h"
#include "const.h"
#include "base.h"
#include "uMPStypes.h"
#include "libumps.h"
#include "listx.h"
#include "types11.h"

void outChildBlocked(pcb_t *p);
pcb_t *headBlocked(int key);
pcb_t *outBlocked(pcb_t *p);
pcb_t *removeBlocked(int key);
int insertBlocked(int key,pcb_t *p);
semd_t *getSemd(int key);
semd_t* mygetSemd(int key);
void initASL(void);
extern struct list_head semd_h;
extern semd_t semd_table[MAXPROC];
extern struct list_head semdFree_h;
void stampasemafori(struct list_head* head);
#endif
