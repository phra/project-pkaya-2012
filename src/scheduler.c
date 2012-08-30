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

int pigliapid(void){
	int i = usedpid;
	while (PIDs[i++] != 0)
		if (i == MAXPID) i = 0;
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
	while(temp = removeChild(target)){
		kill(temp);
	}
	while (!CAS(&mutex_scheduler,0,1));
	processCounter -= 1;
	freePcb(target);
	CAS(&mutex_scheduler,1,0);
}

/* alloca Pcb ed assegna il pid e la priorita' ai processi */  
pcb_t* allocaPcb(int priority){
	while (!CAS(&mutex_scheduler,0,1));
	pcb_t* pcb = allocPcb();
	CAS(&mutex_scheduler,1,0);
	if (pcb == NULL) return NULL;
	//memset(pcb,0,sizeof(pcb)); /* dovrebbe già farlo la allocPcb() */
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
	for(;i<MAXCPUs;i++){
		if (currentproc[i] != NULL) return 0;
	}
	return 1;
}

void scheduler(void){
	myprint("SCHEDULER!\n");
	myprint("readyQ!\n");
	stampalista(readyQ);
	myprint("expiredQ!\n");
	stampalista(expiredQ);
	while (!CAS(&mutex_scheduler,0,1));
	//setSTATUS((getSTATUS() & ~(STATUS_IEc | STATUS_KUc)); /*disabilito interrupt, attivo kernel mode #FIXME */
	if (!list_empty(readyQ)){
		
		myprint("prendo da readyQ!\n");
		pcb_t* next = removeProcQ(readyQ);
		CAS(&mutex_scheduler,1,0);
		currentproc[getPRID()] = next;
		myprinthex("currentPROC",currentproc[getPRID()]);
		next->last_sched_time = GET_TODLOW;
		setTIMER(SCHED_TIME_SLICE);
		myprintint("getTIMER()",getTIMER());
		LDST(&next->p_s);

	} else if (!list_empty(expiredQ)){

		/*scambio le due liste per evitare starvation*/
		myprint("scambioleliste!\n");
		struct list_head* temp = expiredQ;
		expiredQ = readyQ;
		readyQ = temp;
		CAS(&mutex_scheduler,1,0);
		return scheduler();

	} else if ((processCounter > 0) && (softBlockCounter == 0) && (inactivecpu())) {
				myprint("Panic!\n");
				PANIC();

	} else if (processCounter == 0) {

				myprint("HAlt!\n");
				CAS(&mutex_scheduler,1,0);
				HALT();

	} else {
				myprint("WAIT\n");
				CAS(&mutex_scheduler,1,0);
				setTIMER(100*SCHED_TIME_SLICE);
				WAIT();

	}
}
