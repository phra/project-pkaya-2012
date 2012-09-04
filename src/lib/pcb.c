#include "const.h"
#include "uMPStypes.h"
#include "listx.h"
#include "types11.h"
#include "utils.h"


#define TRANSMITTED	5
#define ACK	1
#define PRINTCHR	2
#define CHAROFFSET	8
#define STATUSMASK	0xFF
#define	TERM0ADDR	0x10000250
#define DEVREGSIZE 16       
#define READY     1
#define DEVREGLEN   4
#define BUSY      3

#define RECVSTATUS 0
#define RECVCOMMAND 1
#define TRANSTATUS    2
#define TRANCOMMAND   3


static U32 mytermstat(memaddr *stataddr) {
	return((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if 
 * print was successful, FALSE if not   */
static unsigned int mytermprint(char * str, unsigned int term) {

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

static void pcbprint(char *str1){
        static char output[128];

        strcpy(output, str1);
        mytermprint(output,0);
}

static void pcbprintint(char *str1, int numero){
		static char intero[30];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        pcbprint(str1);
        pcbprint(" -> ");
        itoa(numero,intero,10);
        pcbprint(intero);
        pcbprint("\n");
}

static void pcbprintbin(char *str1, int numero){
		static char intero[64];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        pcbprint(str1);
        pcbprint(" -> ");
        itoa(numero,intero,2);
        pcbprint(intero);
        pcbprint("\n");
}

static void pcbprinthex(char *str1, int numero){
		static char intero[64];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        pcbprint(str1);
        pcbprint(" -> 0x");
        itoa(numero,intero,16);
        pcbprint(intero);
        pcbprint("\n");
}

struct list_head pcbfree_h;
pcb_t pcbFree_table[MAXPROC];

/*
 ########################################################
 ##########  Funzioni per la gestione dei PCB  ##########
 ########################################################
 */

/*
initPbcs(): inizializza la lista pcbFree in modo da contenere tutti gli elementi di pcbFree_table.
*/
void initPcbs(void){
	int i;
	INIT_LIST_HEAD(&pcbfree_h);
	for(i=0; i<MAXPROC;i++ )
	        list_add(&pcbFree_table[i].p_next,&pcbfree_h);
}
/*
freePcb(puntatore a pcb_t): inserisce il processo puntato da p nella lista dei pcb liberi.
*/
void freePcb(pcb_t* p){
	list_add(&p->p_next,&pcbfree_h);
}
/*
(puntatore a pcb_t) allocPcb(): rimuove il primo elemento dalla lista dei pcb liberi e inizializza tutti i campi a NULL e lo restituisce.
in caso non ci siano pcb liberi, restituisce NULL.
*/
pcb_t *allocPcb(void){
	pcb_t* pcb_return;
	struct list_head* l_next = list_next(&pcbfree_h);
	if(list_empty(&pcbfree_h))
		return NULL;
	/*cerca il processo che ha come list_head l_next*/	
	pcb_return=container_of(l_next,pcb_t,p_next);
	list_del(l_next);
	/*impostiamo a 0 tutti i campi di pcb_t*/
	memset(pcb_return,0,(unsigned int)sizeof(pcb_t));
	/*inizializziamo le liste del processo*/
	INIT_LIST_HEAD(&pcb_return->p_child);
	INIT_LIST_HEAD(&pcb_return->p_sib);
	return pcb_return;
}

/*
   ########################################################
   ##  Funzioni per la gestione delle code di processi   ##
   ########################################################
 */

/*
mkEmptyProcQ(puntatore a list_head): inizializza la sentinella.
*/
void mkEmptyProcQ(struct list_head* head){
	INIT_LIST_HEAD(head);
}
/*
(1 TRUE oppure 0 FALSE) emptyProcQ(puntatore alla sentinella della lista): restituisce 1 se la lista è vuota altrimenti restituisce 0.
*/
int emptyProcQ(struct list_head* head){
	return list_empty(head);
}
/*
insertProcQ(puntatore alla sentinella, puntatore ad un processo): inserisce il processo nella lista a cui punta la sentinella,
tenendo conto della priorità dei processi.
*/
void insertProcQ(struct list_head* head, pcb_t *p){
	pcb_t* item;
	struct list_head* iter_head;
	int type = 0;
	if(emptyProcQ(head)){
		list_add(&p->p_next,head);
	}else{
		list_for_each_entry(item,head,p_next){
			if(p->priority > item->priority){
				type = 0;
				iter_head = &item->p_next;
				break;
			}else{
				iter_head = &item->p_next;	
				type = 1;
			}
       	}
       	/*decide se inserire il processo prima o dopo l'elemento puntato da iter_head*/
		if(type==0)
			/*in questo caso nella coda esiste un processo con priorità minore del processo che dobbiamo inserire */
			list_add_tail(&p->p_next,iter_head);
		else
			/*in questo caso invece non esiste un processo con priorità minore*/
			list_add(&p->p_next,iter_head);
	}
}
/*
(puntatore al processo) headProcQ(puntatore alla sentinella):restituisce l'elemento in testa della coda SENZA RIMUOVERLO. se la coda è vuota, restituisce NULL.
*/
pcb_t *headProcQ(struct list_head* head){
	if(emptyProcQ(head))
	                return NULL;
	struct list_head* first_el = list_next(head);
	return container_of(first_el,pcb_t,p_next);
}

/*
(puntatore al processo) removeProcQ(puntatore alla sentinella): restiuisce l'elemento in testa della coda RIMUOVENDOLO. se la coda è vuota, restituisce NULL.
*/
pcb_t* removeProcQ(struct list_head* head){
	struct list_head* first_head;
	if(emptyProcQ(head))
		return NULL;
	first_head = list_next(head);
	list_del(first_head);
	return container_of(first_head,pcb_t,p_next);
}
/*
(puntatore al processo) outProcQ(puntatore alla sentinella, puntatore al processo): restiuisce l'elemento puntato da p, RIMUOVENDOLO dalla coda puntata da head. 
se il processo non è presente, restituisce NULL.
*/
pcb_t* outProcQ(struct list_head* head, pcb_t *p){
	pcb_t* item;
	list_for_each_entry(item,head,p_next){
		if(item==p){
			list_del(&item->p_next);
			return item;
		}
	}
	return NULL;
}

/*
  ########################################################
  ## Funzioni per la gestione degli alberi di processi  ##
  ########################################################
 */
/*
(1 TRUE oppure 0 FALSE) emptyChild(puntatore al processo): restituisce 1 se il processo puntato da p non ha figli, altrimenti 0.
*/
int emptyChild(pcb_t *p){
	return list_empty(&p->p_child);
}

/*
insertChild(puntatore al processo, puntatore al processo): inserisce il processo p come figlio di prnt.
*/
void insertChild(pcb_t *prnt, pcb_t *p){
	/*imposta p figlio di prnt*/
	list_add_tail(&p->p_sib,&prnt->p_child);
	/*imposta prnt padre di p*/
	p->p_parent = prnt;
}
/*
(puntatore al processo) removeChild(puntatore al processo): restituisce il primo figlio del processo puntato da p RIMUOVENDOLO. se p non ha figli, restituisce NULL.
*/
pcb_t* removeChild(pcb_t *p){
	struct list_head* first_head;
	if(emptyChild(p))
		 return NULL;
	first_head = list_next(&p->p_child);
	list_del(first_head);
	return container_of(first_head,pcb_t,p_sib);
}
/*
(puntatore al processo) outChild(puntatore al processo):restituisce il processo puntato da p RIMUOVENDOLO dalla lista dei figli del padre. se il processo p non ha un padre, restituisce NULL.
*/
pcb_t* outChild(pcb_t* p){
	if(emptyChild(p->p_parent))
		return NULL;
	list_del(&p->p_sib);
	return p;
}
