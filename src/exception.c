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


/**
 * handler degli interrupt
 */
void int_handler(void){
    int intline=INT_PLT;
    state_t* before = (state_t*)new_old_areas[getPRID()][INT_OLD];
	pcb_t* suspend = currentproc[getPRID()];

    /* Su quale linea c'è stato l'interrupt */
	while((intline<=INT_TERMINAL) && (!(CAUSE_IP_GET(before->cause, intline)))){
		intline++;          
	}
	if (intline == INT_PLT){/* in questo caso è scaduto il time slice */
		if(suspend){
			/* stop e inserimento in ReadyQueue */
			suspend->cpu_time += (GET_TODLOW - suspend->last_sched_time);
			suspend->p_s = *before;
			inserisciprocessoexpired(suspend);
			currentproc[getPRID()] = NULL;
		}
		return scheduler();
	} else if (intline == INT_TIMER){
		/*pseudoclock*/
		_verhogenclock(MAXPROC+MAX_DEVICES);
		SET_IT(SCHED_PSEUDO_CLOCK);
	} else {
		/*chiamo il gestore dei device, passandogli la linea su cui c'è stato l'interrupt*/
		deviceHandler(intline);
	}
	if (currentproc[getPRID()]) LDST(before);
	else scheduler();
}

/**
 * handler delle tlb exception
 */
void tlb_handler(void){
  
	state_t* before = (state_t*)new_old_areas[getPRID()][TLB_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;

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

/**
 * handler dei program trap
 */
void pgmtrap_handler(void){

	state_t* before = (state_t*)new_old_areas[getPRID()][PGMTRAP_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;

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

/**
 * handler delle system calls
 */
void sysbk_handler(void){

	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;
	suspend->cpu_time += (GET_TODLOW - suspend->last_sched_time);
	suspend->last_sched_time = GET_TODLOW;


	if (CAUSE_EXCCODE_GET(before->cause) == 8){
		/*SYSCALL*/
		if(before->reg_a0 > 12) {
			if (suspend->handler[SYSBK]){
				/*il processo ha definito un suo handler*/
				*(suspend->handler[SYSBK+3]) = *before; /* copy old area in user defined space */
				LDST(suspend->handler[SYSBK]);
			} else {
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
			default:
				PANIC();
		}
	} else if (CAUSE_EXCCODE_GET(before->cause) == 9){
		/*BREAKPOINT */
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
