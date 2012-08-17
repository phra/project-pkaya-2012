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


#include "scheduler.h"
#include "exception.h"

#define MAXCPUs 1
#define MAXPID 1024
#define DEF_PRIORITY 5
#define MAXPRINT 1024

/*Inizializzazione variabili del kernel*/
int processCounter 	 = 0;	/* Contatore processi */
int softBlockCounter = 0;	/* Contatore processi bloccati per I/O */
int readyproc = 0;
int PROVA = 0;
int usedpid = 0;
pcb_t* currentproc[MAXCPUs];
pcb_t* PIDs[MAXPID];	/* un array di processi, 0 se e libero, altrimenti l'indirizzo del pcb*/
semd_t	pseudo_clock;	/* Uno per lo pseudo-clock timer */
semd_t 	device;		/* Uno per ogni device o sub-device */
state_t* new_old_areas[MAXCPUs][8];
state_t real_new_old_areas[MAXCPUs-1][8];

uint32_t mutex_semaphore[MAXPROC];
uint32_t mutex_scheduler = 0;


extern void test();

static inline void initCurrentProcs(){
	memset(currentproc,0,MAXCPUs*sizeof(pcb_t*));
}

static inline void initSemaphoreASL(int value){
	int i = 0;
	for (;i<MAXPROC;i++){
		 semd_table[i].s_value = value;
		 semd_table[i].s_key = i;
	}
}
static inline void initMutexSemaphore(int value){
	int i = 0;
	for (;i<MAXPROC;i++)
		 mutex_semaphore[i] = value;
}

void initSemaphore(semd_t* sem, int value){
	sem->s_value = value;
}

void timerHandler(){
	int cause = getCAUSE();
	if (CAUSE_IP_GET(cause,INT_TIMER)) 
		PROVA++;
	if (PROVA == 5) 
		HALT();
	SET_IT(5000000);
}

static inline void initSchedQueue(void){
	INIT_LIST_HEAD(&readyQueue);
	INIT_LIST_HEAD(&expiredQueue);
}

inline void inserisciprocesso(struct list_head* queue, pcb_t* pcb){
	insertProcQ(queue,pcb);
}

inline void inserisciprocessoready(pcb_t* pcb){
	while (!CAS(&mutex_scheduler,0,1));
	insertProcQ(&readyQueue,pcb);
	CAS(&mutex_scheduler,1,0);
}

static void newAreaKIT(memaddr pc, state_t* addr){
	int status = getSTATUS();
	state_t* state = addr;
	memset(state,0,sizeof(state_t));
	status |= STATUS_IEc;				/* Interrupt abilitato                 */
	status &= ~STATUS_VMc;				/* Virtual Memory OFF                  */
	status |= STATUS_PLTc				/* Processor local timer abilitato     */
	//status |= STATUS_KUc;				/* Kernel-Mode ON #FIXME               */
	state.status = status;
	state.reg_sp = RAMTOP;
	state.pc_epc = state.reg_t9 = pc;
}

static void initNewOldAreas(){
	int i;
	new_old_areas[0][0] = (state_t*)INT_OLDAREA);
	new_old_areas[0][1] = (state_t*)INT_NEWAREA);
	new_old_areas[0][2] = (state_t*)TLB_OLDAREA);
	new_old_areas[0][3] = (state_t*)TLB_NEWAREA);
	new_old_areas[0][4] = (state_t*)PGMTRAP_OLDAREA);
	new_old_areas[0][5] = (state_t*)PGMTRAP_NEWAREA);
	new_old_areas[0][6] = (state_t*)SYSBK_OLDAREA);
	new_old_areas[0][7] = (state_t*)SYSBK_NEWAREA);
	for (i=1;i<MAXCPUs;i++){
		new_old_areas[i][0] = &real_new_old_areas[i-1][0];
		new_old_areas[i][1] = &real_new_old_areas[i-1][1];
		new_old_areas[i][2] = &real_new_old_areas[i-1][2];
		new_old_areas[i][3] = &real_new_old_areas[i-1][3];
		new_old_areas[i][4] = &real_new_old_areas[i-1][4];
		new_old_areas[i][5] = &real_new_old_areas[i-1][5];
		new_old_areas[i][6] = &real_new_old_areas[i-1][6];
		new_old_areas[i][7] = &real_new_old_areas[i-1][7];
	}
	for (i=0;i<MAXCPUs;i++){
		newAreaKIT((memaddr) int_handler, new_old_areas[i][1]);
		newAreaKIT((memaddr) tlb_handler, new_old_areas[i][3]);
		newAreaKIT((memaddr) pgmtrap_handler, new_old_areas[i][5]);
		newAreaKIT((memaddr) sysbk_handler, new_old_areas[i][7]);
	}
}

static inline void initPIDs(){
	memset(PIDs,0,MAXPID*sizeof(pcb_t*));
}

devreg mytermstat(memaddr *stataddr) {
	return((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if 
 * print was successful, FALSE if not   */
static unsigned int mytermprint(char * str, unsigned int term) {

	memaddr *statusp;
	memaddr *commandp;
	
	devreg stat;
	devreg cmd;
	
	unsigned int error = FALSE;
	
	if (term < DEV_PER_INT) {
		/* terminal is correct */
		/* compute device register field addresses */
		statusp = (devreg *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
		commandp = (devreg *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));
		
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


void print(char *str1, char *str2) {
        static char output[MAXPRINT];

        strcpy(output + 1, str1);
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");

        mytermprint(output,0);
}

int main(void)
{	
	/*esempio incremento variabile 5 volte poi HALT dal slide di davoli
	state_t* new_area = (state_t*) INT_NEWAREA;
	// Interrupt disabilitati, kernel mode e memoria virtuale spenta
	new_area->pc_epc = new_area->reg_t9 = (memaddr)timerHandler;
	new_area->status &= ~(STATUS_IEc | STATUS_KUc | STATUS_VMc);
	new_area->reg_sp = RAMTOP;
	int status = 0;
	status |= (STATUS_IEc | STATUS_INT_UNMASKED);
	setSTATUS(status);
	SET_IT(5000000);
	while(1); */
	
	pcb_t* p1 = NULL;
	
	/* Inizializzazione strutture dati */
	initPcbs();		
	initASL();
	initSemaphoreASL(0);
	initMutexSemaphore(0);
	initPIDs();
	initSchedQueue();
	initCurrentProcs();

	/* Inizializzazione per ogni new area */
	initNewOldAreas();

	/*Instanziare il PCB e lo stato del processo test*/
	p1 = allocaPcb(DEF_PRIORITY);		/* Allocare PCB */
	newAreaKIT((memaddr)test,(state_t*)&p1->p_s);
	p1->p_s.reg_sp = RAMTOP - FRAME_SIZE;			/* $SP = RAMPTOP - FRAMESIZE */
	

	/*Inserire il processo nella Ready Queue*/
	inserisciprocessoready(&readyQueue, p1);
	scheduler();						/*Richiamo lo scheduler in modalita START*/

	/*------------------DA FARE-------------------------------	
	//Inizializzare lo stato delle CPU
	//-All'avvio, uMPS2 avvia solo il processore 0
	//-Per avviare le altre CPU è necessario farlo esplicitamente tramite la funzione:	
	// initCpu(uint32_t cpuid, state_t *start_state, state_t *state_areas);
	//Invia un comando RESET al processore cpuid
	//Salva nella ROM l'indirizzo della New/Old Areas puntate da state-areas
	//Carica lo stato del processore da start-state.
	*/
	
	return 0;	
}
