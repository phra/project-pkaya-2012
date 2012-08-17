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

#define TIME_SLICE 5000000

struct list_head readyQueue;
struct list_head expiredQueue;


/* INIZIALIZZAZIONE DI NEW AREA CON KU,IE e PLT SETTATI
   inizializza una new area puntata da addr con pc_epc puntato da pc */

int pigliapid(void){
	int i = usedpid;
	while (PIDs[i++] != 0)
		if (i == MAXPID) i = 0;
	usedpid = i+1;
	return i;
}

/* alloca Pcb ed assegna il pid e la priorita' ai processi */  
pcb_t* allocaPcb(int priority){
	pcb_t* pcb = allocPcb();
	int pid = pigliapid();
	memset(pcb,0,sizeof(pcb));
	pcb->priority = priority;
	PIDs[pid] = pcb;
	processCounter += 1;							/*Incrementiamo il contatore dei processi*/
	return pcb;
}

void scheduler(void){
	while (!CAS(&mutex_scheduler,0,1));
	setSTATUS((getSTATUS() & ~STATUS_IEc) | STATUS_KUc); /*disabilito interrupt, attivo kernel mode */
	if (!list_empty(&readyQueue)){
		
		pcb_t* next = removeProcQ(&readyQueue);
		setTIMER(TIME_SLICE);
		currentproc[getprid()] = next;
		CAS(&mutex_scheduler,1,0);
		LDST(next->p_s);

	} else if (!list_empty(&expiredQueue)){
			
		/*scambio le due liste per evitare starvation*/
		struct list_head temp = expiredQueue;
		expiredQueue = readyQueue;
		readyQueue = temp;
		CAS(&mutex_scheduler,1,0);
		return scheduler();
		
	} else if ((processCounter > 0) && (softBlockCount > 0)) {
		
				CAS(&mutex_scheduler,1,0);
				SET_TIMER(10*TIME_SLICE);
				WAIT();
				
	} else if ((processCounter > 0) && (softBlockCount == 0)) {
		
				PANIC();
				
	} else if (processCounter == 0) {
		
				HALT();
				
	}
}
