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
#include "exception.h"
#include "syscall.h"

#ifndef __INT_MACROS__
#define __INT_MACROS__
#define devBaseAddrCalc(line,devNum) (dtpreg_t*)DEV_REGS_START + (line * 0x80) + (devNum * 0x10)
#define bitmapCalc(line) *((int*)(PENDING_BITMAP_START + (line - 3) * WORD_SIZE));
#endif

U32 devstatus[DEV_USED_INTS][DEV_PER_INT];

void _verhogen(int semkey, int* status){
	/*
		Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
		Registro a1: chiave del semaforo su cui effettuare la V.
	*/
	
	pcb_t* next;
	semd_t* sem = getSemd(semkey);
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value += 1;
	if (headBlocked(semkey)){ /* wake up someone! */
		next = removeBlocked(semkey);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		next->p_s.reg_v0 = *status;
		*status = 0;
		inserisciprocessoready(next);
	} else {
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
	}
}

void _passeren(int semkey){
	/*
		Quando invocata, la sys5 esegue una P sul semaforo con chiave semKey.
		Registro a1: chiave del semaforo su cui effettuare la P.
	*/

	pcb_t* suspend = currentproc[getPRID()];
	semd_t* sem = getSemd(semkey);
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value -= 1;
	if (sem->s_value >= 0){ /* GO! */
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
	} else { /* wait */
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		scheduler();
	}
}

/*Funzione per la gestione dei specifici device*/
void deviceHandler(U32 intline){
	dtpreg_t* device_requested;
    U32 i,rw,status;
    U32 bitmap = bitmapCalc(intline)
    status = rw = i = 0;
	
    /*cerco il numero del device che ha effetuato l'interrupt*/
    for(i = 0; i < DEV_PER_INT; i++) {		
		if ((bitmap) & 0x1)
			break;	
		bitmap >>= 1;
	}
    /*ricavo il registro del device richiesto*/
    device_requested = devBaseAddrCalc(intline,i);

    /*ACK*/
    if(intline != INT_TERMINAL){ /*non è sulla linea del terminale*/
        device_requested->command = DEV_C_ACK;
        status = device_requested->status;
    } else {
		termreg_t* terminal_requested = (termreg_t*)device_requested;
		if (terminal_requested->transm_status == DEV_TTRS_S_CHARTRSM) {
			terminal_requested->transm_command = DEV_C_ACK;
			status = DEV_TTRS_S_CHARTRSM;
		} else if (terminal_requested->recv_status == DEV_TRCV_S_CHARRECV) {
			terminal_requested->recv_command = DEV_C_ACK;
			status = DEV_TRCV_S_CHARRECV;
			rw = 1;
		}
	}
	devstatus[intline][i+rw] = status;
	_verhogen((intline*i)+rw,&devstatus[intline][i+rw]);
}
