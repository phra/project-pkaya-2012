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

/**
	Quando invocata, la sys1 determina la creazione di un processo figlio del processo chiamante.
	Registro a1: indirizzo fisico dello state_t del nuovo processo.
	Registro a2: priorità del nuovo processo.
	Valori di ritorno nel registro v0:
	- 0 nel caso di creazione corretta
	- -1 nel caso di errore
*/
void create_process(void){
	
	pcb_t* suspend = currentproc[getPRID()];
	pcb_t* son;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];

	
	
	if (son = allocaPcb(before->reg_a2)){
		son->p_s = *((state_t*)before->reg_a1);
		while (!CAS(&mutex_scheduler,0,1));
		insertChild(suspend,son);
		insertProcQ(readyQ,son);
		CAS(&mutex_scheduler,1,0);
		before->reg_v0 = 0;
	} else {
		before->reg_v0 = -1;
	}
}

/**
	Quando invocata, la sys2 determina la creazione di un processo fratello del processo chiamante.
	Registro a1: indirizzo fisico dello state_t del nuovo processo.
	Registro a2: priorità del nuovo processo.
	Valori di ritorno nel registro v0:
	- 0 nel caso di creazione corretta
	- -1 nel caso di errore.
*/
void create_brother(void){

	pcb_t* suspend = currentproc[getPRID()];
	pcb_t* father = suspend->p_parent;
	pcb_t* bro;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if (bro = allocaPcb(before->reg_a2)){
		bro->p_s = *((state_t*)before->reg_a1);
		while (!CAS(&mutex_scheduler,0,1));
		insertChild(father,bro);
		insertProcQ(readyQ,bro);
		CAS(&mutex_scheduler,1,0);
		before->reg_v0 = 0;
	} else {
		before->reg_v0 = -1;
	}
}

/**
 * quando invocata la sys3 termina il processo corrente e tutta la sua progenia.
 */
void terminate_process(void){
	pcb_t* suspend = currentproc[getPRID()];
	kill(suspend);
	return scheduler();
}

/**
	Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
	Registro a1: chiave del semaforo su cui effettuare la V.
*/
void verhogen(void){

	
	pcb_t* next;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	int semkey = before->reg_a1;
	semd_t* sem;
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	sem = mygetSemd(semkey);
	sem->s_value++;
	if (headBlocked(semkey)){ /* wake up someone! */
		next = removeBlocked(semkey);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		inserisciprocessoready(next);
	} else {
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
	}
}

/**
	Quando invocata, la sys5 esegue una P sul semaforo con chiave semKey.
	Registro a1: chiave del semaforo su cui effettuare la P.
*/
void passeren(void){

	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	int semkey = before->reg_a1;
	semd_t* sem;
	while (!CAS(&mutex_semaphoreprova,0,1)); /* critical section */
	sem = mygetSemd(semkey);
	sem->s_value--;
	if (sem->s_value >= 0){ /* GO! */
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		LDST(&suspend->p_s);
	} else { /* wait */
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphoreprova,1,0); /* release mutex */
		scheduler();
	}
}

/**
	Quando invocata, la sys6 restituisce il tempo di CPU (in microsecondi) usato dal processo corrente.
	Registro v0: valore di ritorno.
*/
void get_cpu_time(void){

	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	before->reg_v0 = suspend->cpu_time;
}

/**
	Quando invocata, la sys7 esegue una P sul semaforo associato allo pseudo-clock timer. 
	La V su tale semaforo deve essere eseguito dal nucleo ogni 100 millisecondi (tutti i processi in coda su tale 	
	semaforo devono essere sbloccati).
*/
void wait_for_clock(void){

	int i=0;
	pcb_t* suspend = currentproc[getPRID()];

	while (!CAS(&mutex_wait_clock,0,1)); /* critical section */
	
	softBlockCounter++;
	CAS(&mutex_wait_clock,1,0);
	_passerenclock(MAXPROC+MAX_DEVICES);
	
	return scheduler();
}

/**
	Quando invocata, la sys8 esegue una P sul semaforo associato al device identificato da intNo, dnume e waitForTermRead
	Registro a1: linea di interrupt
	Registro a2: device number
	Registro a3: operazione di terminal read/write
	Registro v0: status del device
*/
void wait_for_io_device(void){

	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	int line = before->reg_a1;
	int devno = before->reg_a2;
	int rw = before->reg_a3;

	_passeren((line*(devno+1)+20)+rw);

	before->reg_v0 = devstatus[line-3][devno+rw];
	devstatus[line-3][devno+rw] = 0;
	LDST(before);
}

/**
	Quando invocata, la sys9 consente di definire gestori di PgmTrap per il processo corrente.
	Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
	Registro a2: indirizzo della NEWArea del processo (da utilizzare nel caso si verifichi un PgmTrap)
*/
void specify_prg_state_vector(void){

	
	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if ((suspend->handler[PGMTRAP+3] == NULL) && (suspend->handler[PGMTRAP] == NULL)){
		suspend->handler[PGMTRAP+3] = (state_t*)before->reg_a1;
		suspend->handler[PGMTRAP] = (state_t*)before->reg_a2;
		LDST(&suspend->p_s);
	} else {
		kill(suspend);
		scheduler();
	}
}

/**
	Quando invocata, la sys10 consente di definire gestori di TLB Exception per il processo corrente.
	Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
	Registro a2: indirizzo della NEWArea del processore (da utilizzare nel caso si verifichi una TLB Exception)
*/
void specify_tlb_state_vector(void){


	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if ((suspend->handler[TLB+3] == NULL) && (suspend->handler[TLB] == NULL)){
		suspend->handler[TLB+3] = (state_t*)before->reg_a1;
		suspend->handler[TLB] = (state_t*)before->reg_a2;
		LDST(&suspend->p_s);
	} else {
		kill(suspend);
		scheduler();
	}
}

/**
	Quando invocata, la sys11 consente di definire gestori di SYS/BP Exception per il processo corrente.
	Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
	Registro a2: indirizzo della NEWArea del processore (da utilizzare nel caso si verifichi una SYS/BP Exception)
*/
void specify_sys_state_vector(void){


	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if ((suspend->handler[SYSBK+3] == NULL) && (suspend->handler[SYSBK] == NULL)){
		suspend->handler[SYSBK+3] = (state_t*)before->reg_a1;
		suspend->handler[SYSBK] = (state_t*)before->reg_a2;
		LDST(&suspend->p_s);
	} else {
		kill(suspend);
		scheduler();
	}
}
