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

#define getRegistro(x,cast,y) x = cast(((state_t*)SYSBK_OLDAREA)->y)
#define getRegistroMP(x) (new_old_areas[getPRID()][SYSBK_OLD])->x


void create_process()
{
	/*
		Quando invocata, la sys1 determina la creazione di un processo figlio del processo chiamante.
		Registro a1: indirizzo fisico dello state_t del nuovo processo.
		Registro a2: priorità del nuovo processo.
		Valori di ritorno nel registro v0:
		- 0 nel caso di creazione corretta
		- -1 nel caso di errore
	*/
	
	pcb_t* suspend = currentproc[getPRID()];
	pcb_t* son;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if (son = allocaPcb(before->reg_a2)){
		son->p_s = *(before->reg_a1);
		while (!CAS(&mutex_scheduler,0,1));
		insertChild(suspend,son);
		insertProcQ(readyQ,pcb);
		processCounter += 1;
		CAS(&mutex_scheduler,1,0);
		before->reg_v0 = 0;
	} else {
		before->reg_v0 = -1;
	}
}

void create_brother()
{
	/*
		Quando invocata, la sys2 determina la creazione di un processo fratello del processo chiamante.
		Registro a1: indirizzo fisico dello state_t del nuovo processo.
		Registro a2: priorità del nuovo processo.
		Valori di ritorno nel registro v0:
		- 0 nel caso di creazione corretta
		- -1 nel caso di errore.
	*/
		
	pcb_t* suspend = currentproc[getPRID()];
	pcb_t* father = suspend->p_parent;
	pcb_t* son;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if (son = allocaPcb(before->reg_a2)){
		son->p_s = *(before->reg_a1);
		while (!CAS(&mutex_scheduler,0,1));
		insertChild(father,son);  //#FIXME
		insertProcQ(readyQ,pcb);
		processCounter += 1;
		CAS(&mutex_scheduler,1,0);
		before->reg_v0 = 0;
	} else {
		before->reg_v0 = -1;
	}
}

void terminate_process()
{
	// quando invocata, la sys3 termina il processo corrente e tutta la sua progenie.
	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	kill(suspend);
	scheduler();
}

void verhogen()
{
	/*
		Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
		Registro a1: chiave del semaforo su cui effettuare la V.
	*/
	
	int semkey;
	semd_t* sem;
	pcb_t* next;
	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	semkey = before->reg_a1;
	sem = getSemd(semkey);
	//inserisciprocessoready(suspend);
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value += 1;
	if (headBlocked(semkey)){ /* wake up someone! */
		next = removeBlocked(semkey);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		inserisciprocessoready(next);
	} else {
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
	}
}

void passeren()
{
	/*
		Quando invocata, la sys5 esegue una P sul semaforo con chiave semKey.
		Registro a1: chiave del semaforo su cui effettuare la P.
	*/

	int semkey;
	semd_t* sem;
	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	semkey = before->reg_a1;
	sem = getSemd(semkey);
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value -= 1;
	if (sem->s_value >= 0){ /* GO! */
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		//inserisciprocessoready(suspend);
	} else { /* wait */
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
	}
}

void get_cpu_time()
{
	/*
		Quando invocata, la sys6 restituisce il tempo di CPU (in microsecondi) usato dal processo corrente.
		Registro v0: valore di ritorno.
	*/
	currentProcess->proc_state.s_v0 = currentProcess->startCpuTime;
}

void wait_for_clock()
{
	/*
		Quando invocata, la sys7 esegue una P sul semaforo associato allo pseudo-clock timer. 
		La V su tale semaforo deve essere eseguito dal nucleo ogni 100 millisecondi (tutti i processi in coda su tale 	
		semaforo devono essere sbloccati).
	*/
}

void wait_for_io_device()
{
	/*
		Quando invocata, la sys8 esegue una P sul semaforo associato al device identificato da intNo, dnume e waitForTermRead
		Registro a1: linea di interrupt
		Registro a2: device number
		Registro a3: operazione di temrinal read/write
		Registro v0: status del device
	*/
}

void specifiy_prg_state_vector()
{
	/*
		Quando invocata, la sys9 consente di definire gestori di PgmTrap per il processo corrente.
		Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
		Registro a2: indirizzo della NEWArea del processo (da utilizzare nel caso si verifichi un PgmTrap)
	*/
}

void specify_tlb_state_vector()
{
	/*
		Quando invocata, la sys10 consente di definire gestori di TLB Exception per il processo corrente.
		Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente dle processore.
		Registro a2: indirizzo della NEWArea del processore (da utilizzare nel caso si verifichi una TLB Exception)
	*/
}

void specify_sys_state_vector()
{
	/*
		Quando invocata, la sys11 consente di definire gestori di SYS/BP Exception per il processo corrente.
		Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente dle processore.
		Registro a2: indirizzo della NEWArea del processore (da utilizzare nel caso si verifichi una SYS/BP Exception)
	*/
}
