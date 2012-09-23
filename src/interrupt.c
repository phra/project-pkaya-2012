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
#include "exception.h"
#include "syscall.h"

#ifndef __INT_MACROS__
#define __INT_MACROS__
#define devBaseAddrCalc(line,devNum) (dtpreg_t*)DEV_REGS_START + (line * 0x80) + (devNum * 0x10)
#define bitmapCalc(line) *((int*)(PENDING_BITMAP_START + (line - 3) * WORD_SIZE))
#endif

/**
 * \var devstatus
 * \brief matrice per la gestione degli interrupt
 */
U32 devstatus[DEV_USED_INTS][DEV_PER_INT];

/**
 * verhogen per gli interrupt
 */
void _verhogen(int semkey, unsigned int* status){
	
	pcb_t* next;
	semd_t* sem;
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	sem = mygetSemd(semkey);
	
	if (headBlocked(semkey)){ /* wake up someone! */
		sem->s_value++;
		
		next = removeBlocked(semkey);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		next->p_s.reg_v0 = *status;
		*status = 0;
		while (!CAS(&mutex_wait_clock,0,1));
		softBlockCounter--;
		CAS(&mutex_wait_clock,1,0);
		inserisciprocessoready(next);
	} else {
		sem->s_value = 1;
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	}
}

/**
 * passeren per gli interrupt dei devices
 */
void _passeren(int semkey){

	pcb_t* suspend = currentproc[getPRID()];
	semd_t* sem;
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	sem = mygetSemd(semkey);
	sem->s_value -= 1;
	if (sem->s_value >= 0){ /* GO! */
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	} else { /* wait */
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		while (!CAS(&mutex_wait_clock,0,1));
		softBlockCounter += 1;
		CAS(&mutex_wait_clock,1,0);
		scheduler();
	}
}

/**
 * passeren per il clock
 */
void _passerenclock(int semkey){

	pcb_t* suspend = currentproc[getPRID()];
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	semd_t* sem = mygetSemd(semkey);
	sem->s_value -= 1;
	insertBlocked(semkey,suspend);
	CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	suspend->p_semkey = semkey;
	while (!CAS(&mutex_wait_clock,0,1));
	softBlockCounter++;
	CAS(&mutex_wait_clock,1,0);
}

/**
 * verhogen per il clock
 */
void _verhogenclock(int semkey){

	pcb_t* next;
	int i = 0;
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	semd_t* sem = mygetSemd(semkey);
	while (headBlocked(semkey)){ /* wake up someone! */
		i++;
		if ((next = removeBlocked(semkey)) == NULL) myprint("next == NULL");
		sem->s_value++;
		inserisciprocessoready(next);
	}
	CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	while (!CAS(&mutex_wait_clock,0,1));
		softBlockCounter -= i;
	CAS(&mutex_wait_clock,1,0);
}

/**
 * funzione per la gestione degli interrupt
 */
void deviceHandler(U32 intline){
	dtpreg_t* device_requested;
    U32 i,rw,status;
    U32 bitmap = bitmapCalc(intline);
    status = rw = i = 0;
    memaddr* sendstatus = (U32 *) (TERM0ADDR + (0 * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
	memaddr* sendcommand = (U32 *) (TERM0ADDR + (0 * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));
    memaddr* recvstatus = (U32 *) (TERM0ADDR + (0 * DEVREGSIZE) + (RECVSTATUS * DEVREGLEN));
	memaddr* recvcommand = (U32 *) (TERM0ADDR + (0 * DEVREGSIZE) + (RECVCOMMAND * DEVREGLEN));
	
    /*cerco il numero del device che ha effetuato l'interrupt*/
    for(i = 0; i < DEV_PER_INT; i++) {	
		if ((bitmap) & 0x1)
			break;	
		bitmap >>= 1;
	}
    device_requested = devBaseAddrCalc(intline,i);

    /*ACK*/
    if(intline != INT_TERMINAL){ /*non è sulla linea del terminale*/
        device_requested->command = DEV_C_ACK;
        status = device_requested->status;
    } else {

		termreg_t* terminal_requested = (termreg_t*)device_requested;
		if (mytermstat(sendstatus) == DEV_TTRS_S_CHARTRSM) {
			status = *sendstatus;
			*sendcommand = DEV_C_ACK;
		} else if (mytermstat(recvstatus) == DEV_TRCV_S_CHARRECV) {
			status = *recvstatus;
			*recvcommand = DEV_C_ACK;
			rw = 1;
		}
	}
	devstatus[intline-3][i+rw] = status;
	_verhogen((intline*(i+1)+20)+rw,&devstatus[intline-3][i+rw]);
}
