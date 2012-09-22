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
#include "scheduler.h"
#include "syscall.h"
#include "interrupt.h"


/*Funzione di gestione degli interrupt*/
void int_handler(void){
    int intline=INT_PLT;
    state_t* before = (state_t*)new_old_areas[getPRID()][INT_OLD];
	pcb_t* suspend = currentproc[getPRID()];

	if (getPRID()) BREAK();
	
    /* Su quale linea c'è stato l'interrupt */
	while((intline<=INT_TERMINAL) && (!(CAUSE_IP_GET(before->cause, intline)))){
		intline++;          
	}
	//myprint("readyQ\n");
	//stampalista(readyQ);
	//myprint("expiredQ\n");
	//stampalista(expiredQ);
	//myprintint("INTERRUPT handler",intline);
	//myprintint("getTIMER()",getTIMER());
	//myprint("");
	if (intline == INT_PLT){/* in questo caso è scaduto il time slice */
		//myprinthex("currentPROC",currentproc[getPRID()]);
		if(suspend){
			/* stop e inserimento in ReadyQueue */
			suspend->cpu_time += (GET_TODLOW - suspend->last_sched_time);
			//myprintint("suspend->cpu_time",suspend->cpu_time);
			//before->pc_epc += WORD_SIZE;
			suspend->p_s = *before;
			//myprinthex("indirizzo PCB da sospendere",suspend);
			inserisciprocessoexpired(suspend);
			currentproc[getPRID()] = NULL;
		}
		return scheduler();
	} else if (intline == INT_TIMER){
		//myprint("PSEUDOCLOCK!\n");
		/*pseudoclock*/
		int i=0;
		/*
		while (!CAS(&mutex_wait_clock,0,1));
		for(;i<MAXPROC;i++){
			myprintint("wait_clock[i]",wait_clock[i]);
			if (wait_clock[i]) {
				inserisciprocessoready(wait_clock[i]);
				wait_clock[i] = NULL;
				softBlockCounter--;
			}
		}
		CAS(&mutex_wait_clock,1,0);*/

		_verhogenclock(MAXPROC+MAX_DEVICES);
		SET_IT(SCHED_PSEUDO_CLOCK);
		//if (!currentproc[getPRID()]) scheduler();
	} else {
		/*chiamo il gestore dei device, passandogli la linea su cui c'è stato l'interrupt*/
		deviceHandler(intline);
	}
	if (currentproc[getPRID()]) LDST(before);
	else scheduler();
}

void tlb_handler(void){
  
	state_t* before = (state_t*)new_old_areas[getPRID()][TLB_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;

	myprint("TLB handler\n");

	if (suspend->handler[TLB]){
		/*il processo ha definito un suo handler*/
		*(suspend->handler[TLB+3]) = *before; /* copy old area in user defined space by syscall*/
		LDST(suspend->handler[TLB]);
	} else {
		/*kill it with fire*/
		kill(suspend);
		currentproc[getPRID()] = NULL;
		scheduler();
	}
}

void pgmtrap_handler(void){

	state_t* before = (state_t*)new_old_areas[getPRID()][PGMTRAP_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;

	while (!CAS(&mutex_scheduler,0,1));
	myprintint("PGMTRAP handler",getPRID());
	CAS(&mutex_scheduler,1,0);
	/*myprintint("processocounter",processCounter);
	myprintint("softBlockCounter",softBlockCounter);
	stampareadyq();*/

	if (suspend->handler[PGMTRAP]){
		/*il processo ha definito un suo handler*/
		*(suspend->handler[PGMTRAP+3]) = *before; /* copy old area in user defined space */
		LDST(suspend->handler[PGMTRAP]);
	} else {
		/*kill it with fire*/
		kill(suspend);
		currentproc[getPRID()] = NULL;
		scheduler();
	}
}

void BREAK(void){};

void sysbk_handler(void){

	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;
	suspend->cpu_time += (GET_TODLOW - suspend->last_sched_time);
	suspend->last_sched_time = GET_TODLOW;

	

	
	if (CAUSE_EXCCODE_GET(before->cause) == 8){
		/*SYSCALL*/
		//myprintint("SYS",before->reg_a0); /*#PANIC #FIXME*/
		if(before->reg_a0 > 12) {
			if (suspend->handler[SYSBK]){
				/*il processo ha definito un suo handler*/
				*(suspend->handler[SYSBK+3]) = *before; /* copy old area in user defined space */
				LDST(suspend->handler[SYSBK]);
			} else {
				//if (before->reg_a0 == 13) myprintint("syscall",before->reg_a0);
				kill(suspend);
				return scheduler();
			}
		}
		if (before->status & STATUS_KUp){ /* look at previous bit */
			/*SYSCALL invoked in user mode*/
			if (suspend->handler[PGMTRAP]){
				/*il processo ha definito un suo handler*/
				*(suspend->handler[PGMTRAP+3]) = *before;
				suspend->handler[PGMTRAP+3]->cause = CAUSE_EXCCODE_SET(suspend->handler[PGMTRAP+3]->cause, EXC_RESERVEDINSTR);
				LDST(suspend->handler[PGMTRAP]);
			} else {
				/*kill it with fire*/
				kill(suspend);
				currentproc[getPRID()] = NULL;
				scheduler();
			}
		} else switch (before->reg_a0){ /*SYSCALL invoked in kernel mode*/
			case CREATEPROCESS:
				create_process();
				break;
			case CREATEBROTHER:
				create_brother();
				break;
			case TERMINATEPROCESS:
				return terminate_process();
				break;
			case VERHOGEN:
				verhogen();
				break;
			case PASSEREN:
				return passeren();
				break;
			case GETCPUTIME:
				get_cpu_time();
				break;
			case WAITCLOCK:
				wait_for_clock();
				break;
			case WAITIO:
				wait_for_io_device();
				break;
			case SPECPRGVEC:
				specify_prg_state_vector();
				break;
			case SPECTLBVEC:
				specify_tlb_state_vector();
				break;
			case SPECSYSVEC:
				specify_sys_state_vector();
				break;
			case 14:
				stampasemafori(&semd_h);
				break;
			case 15:
				BREAK();
				break;
			case 16:
				stampareadyq();
				break;
				/*
			case 14:
				stampasemafori(&semd_h);
				break;
			case 15:
				BREAK();
				break;
			case 16:
				stampareadyq();
				break;
			default:
				
				if (suspend->handler[SYSBK]){
					/*il processo ha definito un suo handler
					*(suspend->handler[SYSBK+3]) = *before; /* copy old area in user defined space 
					LDST(suspend->handler[SYSBK]);
				}
				else {
					//if (before->reg_a0 == 13) myprintint("syscall",before->reg_a0);
					kill(suspend);
					return scheduler();}*/
			default:
				myprintint("SYSPANIC\n",before->reg_a0);
				PANIC();
		}
	} else if (CAUSE_EXCCODE_GET(before->cause) == 9){
		/*BREAKPOINT #FIXME */
		myprint("BREAKPOINT handler\n");
		if (suspend->handler[SYSBK]){
			/*il processo ha definito un suo handler*/
			*(suspend->handler[SYSBK+3]) = *before; /* copy old area in user defined space */
			LDST(suspend->handler[SYSBK]);
		} else {
			/*kill it with fire*/
			kill(suspend);
			currentproc[getPRID()] = NULL;
			scheduler();
		}
	} else PANIC();
	LDST(before);
}
