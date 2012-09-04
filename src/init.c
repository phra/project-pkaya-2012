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

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        myprint(str1);
        myprint(" -> ");
        itoa(numero,intero,10);
        myprint(intero);
        myprint("\n");
}

void myprintbin(char *str1, int numero){
		static char intero[64];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        myprint(str1);
        myprint(" -> ");
        itoa(numero,intero,2);
        myprint(intero);
        myprint("\n");
}

void myprinthex(char *str1, int numero){
		static char intero[64];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

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
int processCounter 	 = 0;	/* Contatore processi */
int softBlockCounter = 0;	/* Contatore processi bloccati per I/O */
int readyproc = 0;
int PROVA = 0;
int usedpid = 0;
pcb_t* currentproc[MAXCPUs];
pcb_t* PIDs[MAXPID];	/* un array di processi, 0 se e libero, altrimenti l'indirizzo del pcb*/
pcb_t* wait_clock[MAXPROC];
state_t* new_old_areas[MAXCPUs][8];
state_t real_new_old_areas[MAXCPUs-1][8];

U32 mutex_semaphore[MAXPROC+MAX_DEVICES+1];
U32 mutex_scheduler = 0;
U32 mutex_wait_clock = 0;


extern void test();

static inline void initWaitClock(void){
	memset(wait_clock,0,MAXPROC*sizeof(pcb_t*));
}

static inline void initCurrentProcs(void){
	memset(currentproc,0,MAXCPUs*sizeof(pcb_t*));
}

static inline void initSemaphoreASL(int value){
	int i = 0;
	for (;i<MAXPROC+MAX_DEVICES;i++){
		 semd_table[i].s_value = value;
		 semd_table[i].s_key = i;
	}
}

static inline void initMutexSemaphore(int value){
	int i = 0;
	for (;i<MAXPROC+MAX_DEVICES;i++)
		 mutex_semaphore[i] = value;
}

static inline void initDevStatus(void){
	memset(devstatus,0,DEV_USED_INTS*DEV_PER_INT*sizeof(U32));
}

static inline void initSchedQueue(void){
	INIT_LIST_HEAD(readyQ);
	INIT_LIST_HEAD(expiredQ);
}

static inline void initPIDs(){
	memset(PIDs,0,MAXPID*sizeof(pcb_t*));
}

/* INIZIALIZZAZIONE DI NEW AREA CON KU,IE e PLT SETTATI
   inizializza una new area puntata da addr con pc_epc puntato da pc */
static void initNewArea(memaddr handler, state_t* addr){
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
	state->reg_sp = RAMTOP;
	state->pc_epc = state->reg_t9 = handler;
}

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
		initNewArea((memaddr) int_handler, new_old_areas[i][INT_NEW]);
		initNewArea((memaddr) tlb_handler, new_old_areas[i][TLB_NEW]);
		initNewArea((memaddr) pgmtrap_handler, new_old_areas[i][PGMTRAP_NEW]);
		initNewArea((memaddr) sysbk_handler, new_old_areas[i][SYSBK_NEW]);
	}
}

static void initCpuStates(state_t* addr, int i){
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
	state->reg_sp = RAMTOP - i*FRAME_SIZE;
	state->pc_epc = state->reg_t9 = scheduler;
}

static void initCPUs(void){
	state_t now;
	int i;
	for(i=1;i<MAXCPUs;i++){
		myprintint("inizializzo cpu",i);
		initCpuStates(&now,i);
		INITCPU(i,&now,new_old_areas[i]);
	}
}

int main(void)
{	
	pcb_t* p1 = NULL;

	/* Inizializzazione strutture dati */
	//myprint("init\n");
	initPcbs();		
	initASL();
	initSemaphoreASL(0);
	initMutexSemaphore(0);
	initPIDs();
	initSchedQueue();
	initCurrentProcs();
	initWaitClock();
	initDevStatus();

	/* Inizializzazione per ogni new area */
	initNewOldAreas();

	/*Instanziare il PCB e lo stato del processo test*/
	p1 = allocaPcb(DEF_PRIORITY);		/* Allocare PCB */
	initTest(&p1->p_s);

	/*Inserire il processo nella Ready Queue*/
	inserisciprocessoready(p1);
	myprinthex("indirizzo PCB test",p1);
	SET_IT(SCHED_PSEUDO_CLOCK);
	initCPUs();
	//sleep(10000);
	scheduler();						/*Richiamo lo scheduler*/

	return 0;	
}
