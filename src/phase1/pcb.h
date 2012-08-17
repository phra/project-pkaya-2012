#ifndef __PCB__
#define __PCB__

#include "../lib/const11.h"
#include "../lib/const.h"
#include "../lib/base.h"
#include "../lib/uMPStypes.h"
#include "../lib/libumps.h"
#include "../lib/listx.h"
#include "../lib/types11.h"

pcb_t *outChild(pcb_t *p);
pcb_t *removeChild(pcb_t *p);
void insertChild(pcb_t *prnt,pcb_t *p);
int emptyChild(pcb_t *p);
pcb_t *outProcQ(struct list_head *head,pcb_t *p);
pcb_t *removeProcQ(struct list_head *head);
pcb_t *headProcQ(struct list_head *head);
void insertProcQ(struct list_head *head,pcb_t *p);
int emptyProcQ(struct list_head *head);
void mkEmptyProcQ(struct list_head *head);
pcb_t *allocPcb();
void freePcb(pcb_t *p);
void initPcbs(void);
extern pcb_t pcbFree_table[MAXPROC];
extern struct list_head pcbfree_h;
#endif
