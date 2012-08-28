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


inline U32 devBaseAddrCalc(U8 line, U8 devNum){
	return DEV_REGS_START + (line * 0x80) + (devNum * 0x10);
}

/*Funzione per la gestione dei specifici device*/
void deviceHandler(int intline){
    int devicesAdd,device_requested,terminalRcv,terminalTra ;
    int returnValue=0;
    int i=0;
    int rw = 0;

    devicesAdd= *(   (int*)(PENDING_BITMAP_START + (intline - 3)* WORD_SIZE)  );
    /*cerco il numero del device che ha effetuato l'interrupt*/
    while (i<= DEV_PER_INT && (devicesAdd%2 == 0)) {
            devicesAdd >>= 1;
            i++;
    }
    /*ricavo il registro del device richiesto*/
    device_requested=devBaseAddrCalc((U8)intline,(U8)i);
    
    /*ACK*/
    if(intline!=INT_TERMINAL){ /*non è sulla linea del terminale*/
        ((dtpreg_t*)device_requested)->command = DEV_C_ACK;

        returnValue= ( ((dtpreg_t*)device_requested)->status)& 0xFF;
    }else{ 

        terminalRcv = ( ((termreg_t *)device_requested)->recv_status)& 0xFF  ;

        terminalTra = ( ((termreg_t *)device_requested)->transm_status)& 0xFF ;

        if (terminalTra == DEV_TTRS_S_CHARTRSM) {
            ((termreg_t*)device_requested)->transm_command = DEV_C_ACK;
            returnValue=terminalTra;
        }else{

            if (terminalRcv == DEV_TRCV_S_CHARRECV) {
                ((termreg_t*)device_requested)->recv_command = DEV_C_ACK;
                rw = 1;
                returnValue=terminalRcv;
            }

        }
    }
	devstatus[intline][i+rw] = returnValue;
	_verhogen((intline*i)+rw,&devstatus[intline][i+rw]);
}
