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

	pcb_t	*son_process;
	int	 priority;
	state_t	*indFisico;

	son_process = allocPcb();		
	
	if (son_process != NULL)
	{
		getRegistro(indFisico, (state_t*), s_a1);
		getRegistro(priority,  (int),      s_a2);

		insertProcQ(&readyQueue, son_process);
		processCount++;
		currentProcess->proc_state.s_v0 = 0;
	}
	else
		currentProcess->proc_state.s_v0 = -1;
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

	pcb_t	*brother_process;
	int	 priority;
	state_t	*indFisico;

	brother_process = allocPcb();		
	
	if (brother_process != NULL)
	{
		getRegistro(indFisico, (state_t*), s_a1);
		getRegistro(priority,  (int),      s_a2);

		insertProcQ(&readyQueue, brother_process);
		processCount++;
		currentProcess->proc_state.s_v0 = 0;
	}
	else
		currentProcess->proc_state.s_v0 = -1;
}

void terminate_process()
{
	// quando invocata, la sys3 termina il processo corrente e tutta la sua progenie.
	freePcb(currentProcess);	// Liberare le risorse utilizzate da ogni processo che si intende terminare...
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
	state_t* before = (state_t*)new_old_areas[getPRID()][6];
	semkey = before->reg_a1;
	before->pc_epc += WORD_SIZE;
	sem = getSemd(semkey);
	suspend->p_s = *before;
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value += 1;
	if (headBlocked(semkey) != NULL){ /* wake up someone! */
		next = removeBlocked(semkey);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		inserisciprocessoready(next);
		return scheduler();
	}
	else CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
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
	state_t* before = (state_t*)new_old_areas[getPRID()][6];
	semkey = before->reg_a1;
	before->pc_epc += WORD_SIZE;
	sem = getSemd(semkey);
	suspend->p_s = *before;
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	sem->s_value -= 1;
	if (sem->s_value >= 0){ /* GO! */
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		inserisciprocessoready(suspend);
		return scheduler();
	} else { /* wait */
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		return scheduler();
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
		Quando invocata, la sys10 consente di definire gestori di TLP Exception per il processo corrente.
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
