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
#include "scheduler.h"
#include "syscall.h"
#include "interrupt.h"


/*Funzione di gestione degli interrupt*/
void int_handler(){
    unsigned int devSts;
    int intline=INT_PLT;
	
    /* Su quale linea c'è stato l'interrupt */
	while((intline<=INT_TERMINAL) && (!(CAUSE_IP_GET(new_old_areas[getPRID()][INT_OLD]->cause, intline)))){
		intline++;          
	}

	if (intline == INT_PLT){/* in questo caso è scaduto il time slice */
		if(currentproc[getPRID()] != NULL){
			/* stop e inserimento in ReadyQueue */
			state_t* before = (state_t*)new_old_areas[getPRID()][INT_OLD];
			pcb_t* suspend = currentproc[getPRID()];
			suspend->cpu_time += GET_TODLOW - suspend->last_sched_time;
			before->pc_epc += WORD_SIZE;
			suspend->p_s = *before;
			inserisciprocessoexpired(suspend);
			currentproc[getPRID()] == NULL;
		}
		return scheduler();
	}
	/*chiamo il gestore dei device, passandogli la linea su cui c'è stato l'interrupt*/
	deviceHandler(intline);
	LDST((state_t *)new_old_areas[getPRID()][INT_OLD]);
}

void tlb_handler(){
  
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

void prg_trap_handler(){

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

void syscall_bp_handler(){

	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	pcb_t* suspend = currentproc[getPRID()];
	before->pc_epc += WORD_SIZE;
	suspend->p_s = *before;

	if (suspend->handler[SYSBK]){
		/*il processo ha definito un suo handler*/
		*(suspend->handler[SYSBK+3]) = *before; /* copy old area in user defined space */
		return LDST(suspend->handler[SYSBK]);
	} else if (CAUSE_EXCCODE_GET(before->cause) == 8){
		/*SYSCALL*/
		if (before->status & STATUS_KUp){ /* look at previous bit */
			/*SYSCALL invoked in user mode*/
			if (suspend->handler[PGMTRAP] != NULL){
				/*il processo ha definito un suo handler*/
				*(suspend->handler[PGMTRAP+3]) = *before;
				suspend->handler[PGMTRAP+3]->cause = CAUSE_EXCCODE_SET(suspend->handler[PGMTRAP+3]->cause, EXC_RESERVEDINSTR);
				return LDST(suspend->handler[PGMTRAP]);
			} else {
				/*kill it with fire*/
				kill(suspend);
				currentproc[getPRID()] = NULL;
				return scheduler();
			}
		} else switch (before->reg_a0){ /*SYSCALL invoked in kernel mode*/
			case CREATE_PROCESS:
				create_process();
				break;
			case CREATE_BROTHER:
				create_brother();
				break;
			case TERMINATE_PROCESS:
				return terminate_process();
				break;
			case VERHOGEN:
				verhogen();
				break;
			case PASSEREN:
				passeren();
				break;
			case GET_CPU_TIME:
				get_cpu_time();
				break;
			case WAIT_FOR_CLOCK:
				wait_for_clock();
				break;
			case WAIT_FOR_IO_DEVICE:
				wait_for_io_device();
				break;
			case SPECIFY_PRG_STATE_VECTOR:
				specify_prg_state_vector();
				break;
			case SPECIFY_TLB_STATE_VECTOR:
				specify_tlb_state_vector();
				break;
			case SPECIFY_SYS_STATE_VECTOR:
				specify_sys_state_vector();
				break;
			default:
				PANIC();
		}
	} else if (CAUSE_EXCCODE_GET(before->cause) == 9){
			/*BREAKPOINT #FIXME */
			/*kill it with fire*/
			kill(suspend);
			currentproc[getPRID()] = NULL;
			return scheduler();
	} else PANIC();
	LDST(suspend->p_s);
}
