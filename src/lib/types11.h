#ifndef _TYPES11_H
#define _TYPES11_H

#include "uMPStypes.h"
#include "listx.h"

#ifndef __HANDLERS_CONST__
#define __HANDLERS_CONST__
#define SYSBP 0
#define PGMTRAP 1
#define TLB 2
#endif

/* Process Control Block (PCB) data structure */
typedef struct pcb_t {
	/*process queue fields */
	struct list_head	p_next;

	/*process tree fields */
	struct pcb_t		*p_parent;
	struct list_head	p_child,
						p_sib;
	

	/* processor state, etc */
	state_t       		p_s;     

	/* process priority */
	int					priority;
	
	/* key of the semaphore on which the process is eventually blocked */
	int					p_semkey;


	int pid;

	state_t* handler[3];

} pcb_t;



/* Semaphore Descriptor (SEMD) data structure */
typedef struct semd_t {
	struct list_head	s_next;
	
	/* Semaphore value */
	int					s_value;
	
	/* Semaphore key */
	int					s_key;
	
	/* Queue of PCBs blocked on the semaphore */
	struct list_head	s_procQ;
} semd_t;



#endif
