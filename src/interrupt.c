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

U32 devstatus[DEV_USED_INTS][DEV_PER_INT];

void _verhogen(int semkey, unsigned int* status){
	/*
		Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
		Registro a1: chiave del semaforo su cui effettuare la V.
	*/
	
	pcb_t* next;
	semd_t* sem;
	//myprintint("_verhogen su key",semkey);
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	//if (semkey == 27) myprint("_verhogen\n");
	sem = mygetSemd(semkey);
	
	if (headBlocked(semkey)){ /* wake up someone! */
		sem->s_value++; //FIXME
		
		next = removeBlocked(semkey);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		next->p_s.reg_v0 = *status;
		//myprintint("svegliamo qualcuno, sem->s_value",sem->s_value);
		*status = 0;
		//myprintint("svegliamo qualcuno, sem->s_value",sem->s_value);
		//myprintint("softBlockCounter",softBlockCounter);
		while (!CAS(&mutex_wait_clock,0,1));
		softBlockCounter--;
		//myprintint("softBlockCounter",softBlockCounter);
		CAS(&mutex_wait_clock,1,0);
		//stampareadyq();
		inserisciprocessoready(next);
		
		stampareadyq();
	} else {
		sem->s_value = 1;
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	}
}

void _passeren(int semkey){
	/*
		Quando invocata, la sys5 esegue una P sul semaforo con chiave semKey.
		Registro a1: chiave del semaforo su cui effettuare la P.
	*/

	pcb_t* suspend = currentproc[getPRID()];
	semd_t* sem;
	//if (semkey == 27) myprint("_passeren\n");
	//myprintint("_passeren su key",semkey);
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	sem = mygetSemd(semkey);
	sem->s_value -= 1;
	if (sem->s_value >= 0){ /* GO! */
		//myprintint("WAITIO non BLOCCANTE SU SEMKEY",semkey);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	} else { /* wait */
		//myprintint("WAITIOBLOCCANTE SU SEMKEY",semkey);
		//stampareadyq();
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		while (!CAS(&mutex_wait_clock,0,1));
		softBlockCounter += 1;
		CAS(&mutex_wait_clock,0,1);
		scheduler();
	}
}

void _passerenclock(int semkey){
	/*
		Quando invocata, la sys5 esegue una P sul semaforo con chiave semKey.
		Registro a1: chiave del semaforo su cui effettuare la P.
	*/

	pcb_t* suspend = currentproc[getPRID()];
	//if (semkey == 27) myprint("_passerenclock\n");
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	semd_t* sem = mygetSemd(semkey);
	//myprintint("_passerenclock prima",sem->s_value);
	sem->s_value -= 1;
	//myprintint("_passerenclock dopo",sem->s_value);
	insertBlocked(semkey,suspend);
	currentproc[getPRID()] = NULL;
	CAS(&mutex_semaphoreprova,1,0); /* release mutex */
}

void _verhogenclock(int semkey){
	/*
		Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
		Registro a1: chiave del semaforo su cui effettuare la V.
	*/
	
	pcb_t* next;
	//if (semkey == 27) myprint("_verhogenclock\n");
	//myprintint("_verhogenclock su semkey",semkey);
	//myprinthex("all'indirizzo",sem);
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	semd_t* sem = mygetSemd(semkey);
	//myprintint("s_value",sem->s_value);
	while (headBlocked(semkey)){ /* wake up someone! */
		next = removeBlocked(semkey);
		//myprinthex("sblocco processo",next);
		sem->s_value++;
		while (!CAS(&mutex_wait_clock,0,1));
		softBlockCounter--;
		CAS(&mutex_wait_clock,1,0);
		inserisciprocessoready(next);
	}
	//myprintint("s_value",sem->s_value);
	CAS(&mutex_semaphoreprova,1,0); /* release mutex */
}

/*Funzione per la gestione dei specifici device*/
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
	//myprintint("numero device",i);
    /*ricavo il registro del device richiesto*/
    device_requested = devBaseAddrCalc(intline,i);
    //myprinthex("device_requested",device_requested);
    //myprinthex("indirizzo teorico term0",DEV_REGS_START + (intline * 0x80));

    /*ACK*/
    if(intline != INT_TERMINAL){ /*non è sulla linea del terminale*/
        device_requested->command = DEV_C_ACK;
        status = device_requested->status;
    } else {
	/*
		termreg_t* terminal_requested = (termreg_t*)device_requested;
		
		if (terminal_requested->transm_status == DEV_TTRS_S_CHARTRSM) {
			myprint("CHARTRSM\n");
			terminal_requested->transm_command = DEV_C_ACK;
			status = DEV_TTRS_S_CHARTRSM;
		} else if (terminal_requested->recv_status == DEV_TRCV_S_CHARRECV) {
			myprint("CHARRECV\n");
			terminal_requested->recv_command = DEV_C_ACK;
			status = DEV_TRCV_S_CHARRECV;
			rw = 1;
		}*/

		termreg_t* terminal_requested = (termreg_t*)device_requested;
		  //myprintint("mytermstat(*sendstatus)",mytermstat(sendstatus));
		if (mytermstat(sendstatus) == DEV_TTRS_S_CHARTRSM) {
			//myprint("CHARTRSM\n");
			status = *sendstatus;
			*sendcommand = DEV_C_ACK;
		} else if (mytermstat(recvstatus) == DEV_TRCV_S_CHARRECV) {
			//myprint("CHARRECV\n");
			status = *recvstatus;
			*recvcommand = DEV_C_ACK;
			rw = 1;
		}
	}
	devstatus[intline-3][i+rw] = status;
	_verhogen((intline*(i+1)+20)+rw,&devstatus[intline-3][i+rw]);
}
