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

U32 devstatus[DEV_USED_INTS][DEV_PER_INT];

void _verhogen(int semkey, int* status){
	/*
		Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
		Registro a1: chiave del semaforo su cui effettuare la V.
	*/
	
	pcb_t* next;
	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	semd_t* sem = getSemd(semkey);
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value += 1;
	if (headBlocked(semkey)){ /* wake up someone! */
		next = removeBlocked(semkey);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		next->reg_v0 = *status;
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
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
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


inline dtpreg_t* devBaseAddrCalc(U8 line, U8 devNum){
	return (dtpreg_t*)DEV_REGS_START + (line * 0x80) + (devNum * 0x10);
}

/*Funzione per la gestione dei specifici device*/
void deviceHandler(int intline){
    U32 terminalRcv,terminalTra,i,rw,status;
    dtpreg_t* device_requested;
    U32 bitmap = *(   (int*)(PENDING_BITMAP_START + (intline - 3) * WORD_SIZE)  );
    status = rw = i = 0;
	
    /*cerco il numero del device che ha effetuato l'interrupt*/

    for(i = 0; i < DEV_PER_INT; i++) {		
		if ((bitmap) & 0x1)
			break;	
		bitmap >>= 1;
	}
    /*ricavo il registro del device richiesto*/
    device_requested = devBaseAddrCalc((U8)intline,(U8)i);

    /*ACK*/
    if(intline != INT_TERMINAL){ /*non è sulla linea del terminale*/
        device_requested->command = DEV_C_ACK;
        status = device_requested->status;
    }else{ 
        terminalRcv = device_requested->recv_status;
        terminalTra = device_requested->transm_status;
        if (terminalTra == DEV_TTRS_S_CHARTRSM) {
            device_requested->transm_command = DEV_C_ACK;
            status=terminalTra;
        } else if (terminalRcv == DEV_TRCV_S_CHARRECV) {
			device_requested->recv_command = DEV_C_ACK;
			rw = 1;
			status = terminalRcv;
        }
    }
	devstatus[intline][i+rw] = status;
	_verhogen((intline*i)+rw,&devstatus[intline][i+rw]);
}
