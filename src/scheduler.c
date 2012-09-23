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
#include "interrupt.h"
#include "exception.h"
#include "syscall.h"

struct list_head readyQueue;
struct list_head expiredQueue;
struct list_head* readyQ = &readyQueue;
struct list_head* expiredQ = &expiredQueue;

void stampalista(struct list_head* head){
	pcb_t* item;
	list_for_each_entry(item,head,p_next){
		myprintint("PROCESSO IN LISTA CON PID",item->pid);
	}
}

void stampareadyq(void){
	pcb_t* item;
	myprint("READYQ:\n");
	list_for_each_entry(item,readyQ,p_next){
		myprintint("PROCESSO IN LISTA CON PID",item->pid);
	}
}

int pigliapid(void){
	int i = usedpid;
	while (PIDs[i] != 0)
		if (i++ == MAXPID) i = 0;
	usedpid = i+1;
	return i;
}

void inserisciprocessoexpired(pcb_t* pcb){
	while (!CAS(&mutex_scheduler,0,1));
	insertProcQ(expiredQ,pcb);
	CAS(&mutex_scheduler,1,0);
}

void inserisciprocessoready(pcb_t* pcb){
	while (!CAS(&mutex_scheduler,0,1));
	insertProcQ(readyQ,pcb);
	CAS(&mutex_scheduler,1,0);
}

void kill(pcb_t* target){
	pcb_t* temp;
	PIDs[target->pid] = NULL;
	currentproc[getPRID()] = NULL;
	while(temp = removeChild(target)){
		kill(temp);
	}

	while (!CAS(&mutex_wait_clock,0,1));
	processCounter--;
	CAS(&mutex_wait_clock,1,0);

	
	while (!CAS(&mutex_scheduler,0,1));
	if (processCounter == 0) {
		CAS(&mutex_scheduler,1,0);
		scheduler();
	} else {
		outProcQ(readyQ,target);
		outProcQ(expiredQ,target);
		CAS(&mutex_scheduler,1,0);
		
		if (target->p_semkey != -1){
			semd_t* block;
			while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
			if (block = getSemd(target->p_semkey)){
				block->s_value++;
				outBlocked(target);
			}
			CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		}
	freePcb(target);
	}
}

/* alloca Pcb ed assegna il pid e la priorita' ai processi */  
pcb_t* allocaPcb(int priority){
	while (!CAS(&mutex_scheduler,0,1));
	pcb_t* pcb = allocPcb();
	if (!pcb) {
		myprint("finiti i pcb\n");
		return NULL;
	}
	CAS(&mutex_scheduler,1,0);
	pcb->priority = priority;
	while (!CAS(&mutex_scheduler,0,1));
	pcb->pid = pigliapid();
	PIDs[pcb->pid] = pcb;
	processCounter += 1;						/*Incrementiamo il contatore dei processi*/
	CAS(&mutex_scheduler,1,0);
	return pcb;
}

static int inactivecpu(void){
	int i = 0;
	return 0;
	for(;i<MAXCPUs;i++){
		if (currentproc[i] != NULL) return 0;
	}
	return 1;
}

void scheduler(void){
	while (!CAS(&mutex_scheduler,0,1));
	currentproc[getPRID()] = NULL;
	if (processCounter == 0) {
				HALT();
	}
	if (!list_empty(readyQ)){
		pcb_t* next = removeProcQ(readyQ);
		CAS(&mutex_scheduler,1,0);
		currentproc[getPRID()] = next;
		next->last_sched_time = GET_TODLOW;
		setTIMER(SCHED_TIME_SLICE);
		LDST(&next->p_s);

	} else if (!list_empty(expiredQ)){

		/*scambio le due liste per evitare starvation*/
		struct list_head* temp = expiredQ;
		expiredQ = readyQ;
		readyQ = temp;
		CAS(&mutex_scheduler,1,0);
		return scheduler();

	} else if ((processCounter > 0) && (softBlockCounter == 0) && (inactivecpu())) {
				PANIC();

	} else {
				unsigned int status;
				CAS(&mutex_scheduler,1,0);
				setTIMER(10*SCHED_TIME_SLICE);
				status = getSTATUS();
				status |= STATUS_IEc;				/* Interrupt abilitati                 */
				status |= STATUS_IEp;				/* Set also previous bit for LDST()    */
				status |= STATUS_IEo;
				status |= STATUS_INT_UNMASKED;
				status |= STATUS_PLT;
				setSTATUS(status);
				WAIT();

	}
}
