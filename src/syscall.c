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


void create_process(void){
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
		son->p_s = *((state_t*)before->reg_a1);
		while (!CAS(&mutex_scheduler,0,1));
		insertChild(suspend,son);
		/*
		myprinthex("alloco processo figlio con indirizzo",son);
		myprintint("con PID",son->pid);
		myprint("prima dell'inserimento\n");
		stampalista(readyQ);*/
		insertProcQ(readyQ,son);
		//myprint("dopo l'inserimento\n");
		//stampalista(readyQ);
		CAS(&mutex_scheduler,1,0);
		before->reg_v0 = 0;
	} else {
		before->reg_v0 = -1;
	}
}

void create_brother(void){
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
	pcb_t* bro;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if (bro = allocaPcb(before->reg_a2)){
		bro->p_s = *((state_t*)before->reg_a1);
		while (!CAS(&mutex_scheduler,0,1));
		insertChild(father,bro);  //#FIXME
		insertProcQ(readyQ,bro);
		CAS(&mutex_scheduler,1,0);
		before->reg_v0 = 0;
	} else {
		before->reg_v0 = -1;
	}
}

void terminate_process(void){
	// quando invocata, la sys3 termina il processo corrente e tutta la sua progenie.
	pcb_t* suspend = currentproc[getPRID()];
	kill(suspend);
	return scheduler();
}

void verhogen(void){
	/*
		Quando invocata, la sys4 esegue una V sul semaforo con chiave semKey
		Registro a1: chiave del semaforo su cui effettuare la V.
	*/
	
	pcb_t* next;
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	int semkey = before->reg_a1;
	semd_t* sem = mygetSemd(semkey);
	/*if((semkey == 8) || (semkey == 7) || (semkey == 9)){
		myprint("verhogen: semafori allocati:\n");
		stampasemafori(&semd_h);
		myprintint("V su semkey",semkey);
		//myprint("prima della P\n");
		//stampalista(readyQ);
	}*/
	if(!sem) myprint("da phuk: sem == NULL\n");
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	//myprintint("s_value prima",sem->s_value);
	sem->s_value++;
	//myprintint("s_value dopo",sem->s_value);
	//stampasemafori(&semd_h);
	if (headBlocked(semkey)){ /* wake up someone! */
		//myprint("svegliamo qualcuno.\n");
		next = removeBlocked(semkey);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		inserisciprocessoready(next);
		//myprint("readyQ!\n");
		//stampalista(readyQ);
	} else {
		//myprint("nessuno da svegliare.\n");
		//myprint("readyQ!\n");
		//stampalista(readyQ);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
	}
}

void passeren(void){
	/*
		Quando invocata, la sys5 esegue una P sul semaforo con chiave semKey.
		Registro a1: chiave del semaforo su cui effettuare la P.
	*/

	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	int semkey = before->reg_a1;
	semd_t* sem = mygetSemd(semkey);
	/*if((semkey == 8) || (semkey == 7) || (semkey == 9)){
		myprint("passeren: semafori allocati:\n");
		stampasemafori(&semd_h);
		myprintint("P su semkey",semkey);
		//myprint("prima della P\n");
		//stampalista(readyQ);
	}*/
	//myprinthex("che si trova all'indirizzo",sem);
	while (!CAS(&mutex_semaphore[semkey],0,1)); /* critical section */
	//myprintint("s_value prima",sem->s_value);
	sem->s_value--;
	//myprintint("s_value dopo",sem->s_value);
	//stampasemafori(&semd_h);
	if (sem->s_value >= 0){ /* GO! */
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		LDST(&suspend->p_s);
	} else { /* wait */
		insertBlocked(semkey,suspend);
		CAS(&mutex_semaphore[semkey],1,0); /* release mutex */
		scheduler();
	}
}

void get_cpu_time(void){
	/*
		Quando invocata, la sys6 restituisce il tempo di CPU (in microsecondi) usato dal processo corrente.
		Registro v0: valore di ritorno.
	*/

	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	before->reg_v0 = suspend->cpu_time;
}

void wait_for_clock(void)
{
	/*
		Quando invocata, la sys7 esegue una P sul semaforo associato allo pseudo-clock timer. 
		La V su tale semaforo deve essere eseguito dal nucleo ogni 100 millisecondi (tutti i processi in coda su tale 	
		semaforo devono essere sbloccati).
	*/
	int i=0;
	pcb_t* suspend = currentproc[getPRID()];
	//while (!CAS(&mutex_wait_clock,0,1)); /* critical section */
	/*
	for(;i<MAXPROC;i++){
		if (wait_clock[i] == NULL){
			wait_clock[i] = suspend;
			softBlockCounter += 1;
			break;
		}
	}
	* */
	
	//CAS(&mutex_wait_clock,1,0);
	softBlockCounter++;
	_passerenclock(MAXPROC+MAX_DEVICES);
	
	return scheduler();
}

void wait_for_io_device(void){
	/*
		Quando invocata, la sys8 esegue una P sul semaforo associato al device identificato da intNo, dnume e waitForTermRead
		Registro a1: linea di interrupt
		Registro a2: device number
		Registro a3: operazione di terminal read/write
		Registro v0: status del device
	*/

	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	int line = before->reg_a1;
	int devno = before->reg_a2;
	int rw = before->reg_a3;

	while (!CAS(&mutex_wait_clock,0,1)); /* critical section */
	
	CAS(&mutex_wait_clock,1,0);

	_passeren((line*(devno+1)+20)+rw);

	before->reg_v0 = devstatus[line][devno+rw];
	devstatus[line][devno+rw] = 0;
	LDST(before);
}

void specify_prg_state_vector(void)
{
	/*
		Quando invocata, la sys9 consente di definire gestori di PgmTrap per il processo corrente.
		Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
		Registro a2: indirizzo della NEWArea del processo (da utilizzare nel caso si verifichi un PgmTrap)
	*/
	
	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if ((suspend->handler[PGMTRAP+3] == NULL) && (suspend->handler[PGMTRAP] == NULL)){
		suspend->handler[PGMTRAP+3] = (state_t*)before->reg_a1;
		suspend->handler[PGMTRAP] = (state_t*)before->reg_a2;
		LDST(&suspend->p_s);
	} else kill(suspend);
}

void specify_tlb_state_vector(void){
	/*
		Quando invocata, la sys10 consente di definire gestori di TLB Exception per il processo corrente.
		Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
		Registro a2: indirizzo della NEWArea del processore (da utilizzare nel caso si verifichi una TLB Exception)
	*/

	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if ((suspend->handler[TLB+3] == NULL) && (suspend->handler[TLB] == NULL)){
		suspend->handler[TLB+3] = (state_t*)before->reg_a1;
		suspend->handler[TLB] = (state_t*)before->reg_a2;
		LDST(&suspend->p_s);
	} else kill(suspend);
}

void specify_sys_state_vector(void){
	/*
		Quando invocata, la sys11 consente di definire gestori di SYS/BP Exception per il processo corrente.
		Registro a1: indirizzo della OLDArea in cui salvare lo stato corrente del processore.
		Registro a2: indirizzo della NEWArea del processore (da utilizzare nel caso si verifichi una SYS/BP Exception)
	*/

	pcb_t* suspend = currentproc[getPRID()];
	state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
	
	if ((suspend->handler[SYSBK+3] == NULL) && (suspend->handler[SYSBK] == NULL)){
		suspend->handler[SYSBK+3] = (state_t*)before->reg_a1;
		suspend->handler[SYSBK] = (state_t*)before->reg_a2;
		LDST(&suspend->p_s);
	} else kill(suspend);
}
