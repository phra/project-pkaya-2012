#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"
#include "lib/pcb.h"
#include "lib/asl.h"
#include "lib/utils.h"

#include "scheduler.h"
#include "exception.h"
#include "syscall.h"
#include "interrupt.h"

/*DEBUG*/

U32 mytermstat(memaddr *stataddr) {
	return((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if 
 * print was successful, FALSE if not   */
unsigned int mytermprint(char * str, unsigned int term) {

	memaddr *statusp;
	memaddr *commandp;
	U32 stat;
	U32 cmd;
	unsigned int error = FALSE;
	if (term < DEV_PER_INT) {
		/* terminal is correct */
		/* compute device register field addresses */
		statusp = (U32 *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
		commandp = (U32 *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));
		/* test device status */
		stat = mytermstat(statusp);
		if ((stat == READY) || (stat == TRANSMITTED)) {
			/* device is available */
			/* print cycle */
			while ((*str != '\0') && (!error)) {
				cmd = (*str << CHAROFFSET) | PRINTCHR;
				*commandp = cmd;
				/* busy waiting */
				while ((stat = mytermstat(statusp)) == BUSY);
				/* end of wait */
				if (stat != TRANSMITTED) {
					error = TRUE;
				} else {
					/* move to next char */
					str++;
				}
			}
		}	else {
			/* device is not available */
			error = TRUE;
		}
	}	else {
		/* wrong terminal device number */
		error = TRUE;
	}
	return (!error);		
}

void myprint(char *str1){
        static char output[MAXPRINT];

        strcpy(output, str1);
        mytermprint(output,0);
}

void myprintint(char *str1, int numero){
		static char intero[30];

        myprint(str1);
        myprint(" -> ");
        itoa(numero,intero,10);
        myprint(intero);
        myprint("\n");
}

void myprintbin(char *str1, int numero){
		static char intero[64];

        myprint(str1);
        myprint(" -> ");
        itoa(numero,intero,2);
        myprint(intero);
        myprint("\n");
}

void myprinthex(char *str1, int numero){
		static char intero[64];

        myprint(str1);
        myprint(" -> 0x");
        itoa(numero,intero,16);
        myprint(intero);
        myprint("\n");
}

void sleep(int count){
	int i = 0;
	for(;i<count;i++);
}

/*END DEBUG*/

/*Inizializzazione variabili del kernel*/
/**
 * \var processCounter
 * \brief contatore dei processi
 */
int processCounter = 0;	/* Contatore processi */

/**
 * \var softBlockCounter 
 * \brief Contatore processi bloccati per I/O
 */
int softBlockCounter = 0;	/* Contatore processi bloccati per I/O */

/**
 * \var usedpid 	
 * \brief Per tenere conto dei pid assegnati
 */
int usedpid = 0;

/**
 * \var currentproc
 * \brief Puntatori ai processi attualmente in esecuzione
 */
pcb_t* currentproc[MAXCPUs];

/**
 * \var PIDs
 * \brief un array per tenere conto dei pid assegnati, null se è libero, altrimenti l'indirizzo del pcb
 */
pcb_t* PIDs[MAXPID];	/* un array di processi, 0 se e libero, altrimenti l'indirizzo del pcb*/

/**
 * \var new_old_areas
 * \brief Tabella generale per le new/old areas, in modo da avere un'unica tabella per tutti i processori.
 */
state_t* new_old_areas[MAXCPUs][8];

/**
 * \var real_new_old_areas
 * \brief new/old area reali dei processori 1..n che le hanno sullo stack 
 */
state_t real_new_old_areas[MAXCPUs-1][8];

/**
 * \var mutex_semaphoreprova
 * \brief mutex per accedere ai semafori 
 */
U32 mutex_semaphoreprova = 0;

/**
* \var mutex_scheduler
* \brief mutex per accedere allo scheduler, process counter e code dei processi
*/
U32 mutex_scheduler = 0;

/**
* \var mutex_wait_clock
* \brief mutex per accedere al softBlockCounter
*/
U32 mutex_wait_clock = 0;


extern void test();

/**
 * funzione per inizializzare l'array dei processi correnti
 */
static inline void initCurrentProcs(void){
	memset(currentproc,0,MAXCPUs*sizeof(pcb_t*));
}

/**
 * funzione per inizializzare i semafori dell'ASL
 * \param value valore a cui inizializzare i semafori
 */
static inline void initSemaphoreASL(int value){
	int i = 0;
	for (;i<MAXPROC+MAX_DEVICES;i++){
		 semd_table[i].s_value = value;
		 semd_table[i].s_key = i;
	}
}

/**
 * funzione per inizializzare la tabella per gestire gli interrupt
 */
static inline void initDevStatus(void){
	memset(devstatus,0,DEV_USED_INTS*DEV_PER_INT*sizeof(U32));
}

/**
 * funzione per inizializzare le code dello scheduler
 */
static inline void initSchedQueue(void){
	INIT_LIST_HEAD(readyQ);
	INIT_LIST_HEAD(expiredQ);
}

/**
 * funzione per inizializzare i pid
 */
static inline void initPIDs(){
	memset(PIDs,0,MAXPID*sizeof(pcb_t*));
}

/**
 * funzione per inizializzare le new areas
 * \param handler indirizzo della funzione da eseguire
 * \param addr indirizzo della new area
 * Bisogna settare i bit precedenti IE/KU/VM nella new area, poiché la LDST fa una pop su questi bit
 */
static void initNewArea(memaddr handler, state_t* addr, int numcpu){
	int status = 0;
	state_t* state = addr;
	memset(state,0,sizeof(state_t));
	status &= ~STATUS_IEc;				/* Interrupt non abilitati             */
	status &= ~STATUS_IEp;				/* Set also previous bit for LDST()    */
	status &= ~STATUS_IEo;
	status &= ~STATUS_INT_UNMASKED;
	status &= ~STATUS_VMc;				/* Virtual Memory OFF                  */
	status &= ~STATUS_VMp;				/* Set also previous bit for LDST()    */
	status &= ~STATUS_VMo;
	status &= ~STATUS_KUc;				/* Kernel-Mode ON                      */
	status &= ~STATUS_KUp;				/* Set also previous bit for LDST()    */
	status &= ~STATUS_KUo;
	state->status = status;
	state->reg_sp = RAMTOP - numcpu*PAGE_SIZE;
	state->pc_epc = state->reg_t9 = handler;
}

/**
 * funzione per inizializzare lo stato del Test
 * \param addr indirizzo dello state del test
 */
static void initTest(state_t* addr){
	int status = 0;
	state_t* state = addr;
	memset(state,0,sizeof(state_t));
	status |= STATUS_IEc;				/* Interrupt abilitati                 */
	status |= STATUS_IEp;				/* Set also previous bit for LDST()    */
	status |= STATUS_IEo;
	status |= STATUS_INT_UNMASKED;
	status &= ~STATUS_VMc;				/* Virtual Memory OFF                  */
	status &= ~STATUS_VMp;				/* Set also previous bit for LDST()    */
	status &= ~STATUS_VMo;
	status &= ~STATUS_KUc;				/* Kernel-Mode ON                      */
	status &= ~STATUS_KUp;				/* Set also previous bit for LDST()    */
	status &= ~STATUS_KUo;
	status |= STATUS_PLT;
	state->status = status;
	state->reg_sp = RAMTOP - (MAXCPUs+1)*FRAME_SIZE;			/* $SP = RAMPTOP - FRAMESIZE */
	state->pc_epc = state->reg_t9 = (memaddr)test;
}

/**
 * funzione per inizializzare le new/old areas
 */
static void initNewOldAreas(void){
	int i;
	new_old_areas[0][INT_OLD] = (state_t*)INT_OLDAREA;
	new_old_areas[0][INT_NEW] = (state_t*)INT_NEWAREA;
	new_old_areas[0][TLB_OLD] = (state_t*)TLB_OLDAREA;
	new_old_areas[0][TLB_NEW] = (state_t*)TLB_NEWAREA;
	new_old_areas[0][PGMTRAP_OLD] = (state_t*)PGMTRAP_OLDAREA;
	new_old_areas[0][PGMTRAP_NEW] = (state_t*)PGMTRAP_NEWAREA;
	new_old_areas[0][SYSBK_OLD] = (state_t*)SYSBK_OLDAREA;
	new_old_areas[0][SYSBK_NEW] = (state_t*)SYSBK_NEWAREA;
	for (i=1;i<MAXCPUs;i++){
		new_old_areas[i][INT_OLD] = &real_new_old_areas[i-1][INT_OLD];
		new_old_areas[i][INT_NEW] = &real_new_old_areas[i-1][INT_NEW];
		new_old_areas[i][TLB_OLD] = &real_new_old_areas[i-1][TLB_OLD];
		new_old_areas[i][TLB_NEW] = &real_new_old_areas[i-1][TLB_NEW];
		new_old_areas[i][PGMTRAP_OLD] = &real_new_old_areas[i-1][PGMTRAP_OLD];
		new_old_areas[i][PGMTRAP_NEW] = &real_new_old_areas[i-1][PGMTRAP_NEW];
		new_old_areas[i][SYSBK_OLD] = &real_new_old_areas[i-1][SYSBK_OLD];
		new_old_areas[i][SYSBK_NEW] = &real_new_old_areas[i-1][SYSBK_NEW];
	}
	for (i=0;i<MAXCPUs;i++){
		initNewArea((memaddr) int_handler, new_old_areas[i][INT_NEW],i);
		initNewArea((memaddr) tlb_handler, new_old_areas[i][TLB_NEW],i);
		initNewArea((memaddr) pgmtrap_handler, new_old_areas[i][PGMTRAP_NEW],i);
		initNewArea((memaddr) sysbk_handler, new_old_areas[i][SYSBK_NEW],i);
	}
}

/**
 * funzione per inizializzare lo stato della CPU
 * \param addr indirizzo dello stato da inizializzare
 * \param i numero del processore
 */
static void initCpuStates(state_t* addr, int i){
	int status = 0;
	state_t* state = addr;
	memset(state,0,sizeof(state_t));
	state->status = getSTATUS();
	state->reg_sp = RAMTOP - i*FRAME_SIZE;
	state->pc_epc = state->reg_t9 = (memaddr)scheduler;
}

/**
 * funzione per inizializzare le CPU
 */
static void initCPUs(void){
	state_t now;
	int i;
	for(i=1;i<*(int*)0x10000500;i++){
		initCpuStates(&now,i);
		INITCPU(i,&now,new_old_areas[i][0]);
	}
}

/**
 * Entry point dal Kernel
 * \return 0
 */
int main(void)
{	
	pcb_t* p1 = NULL;

	/* Inizializzazione strutture dati */
	initPcbs();		
	initASL();
	initSemaphoreASL(0);
	initPIDs();
	initSchedQueue();
	initCurrentProcs();
	//initWaitClock();
	initDevStatus();

	/* Inizializzazione per ogni new area */
	initNewOldAreas();

	/*Instanziare il PCB e lo stato del processo test*/
	p1 = allocaPcb(DEF_PRIORITY);		/* Allocare PCB */
	initTest(&p1->p_s);

	/*Inserire il processo nella Ready Queue*/
	inserisciprocessoready(p1);
	//myprinthex("indirizzo PCB test",p1);
	initCPUs();
	SET_IT(SCHED_PSEUDO_CLOCK);
	scheduler();						/*Richiamo lo scheduler*/

	return 0;	
}
