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

static void aslprint(char *str1){
        static char output[128];

        strcpy(output, str1);
        mytermprint(output,0);
}

static void aslprintint(char *str1, int numero){
		static char intero[30];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        aslprint(str1);
        aslprint(" -> ");
        itoa(numero,intero,10);
        aslprint(intero);
        aslprint("\n");
}

static void aslprintbin(char *str1, int numero){
		static char intero[64];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        aslprint(str1);
        aslprint(" -> ");
        itoa(numero,intero,2);
        aslprint(intero);
        aslprint("\n");
}

static void aslprinthex(char *str1, int numero){
		static char intero[64];

       /* strcpy(output + 1, str1); //#FIXME
        strcpy(output + strlen(str1) + 1, " -> ");
        strcpy(output + strlen(str1) + 3, str2);
        strcpy(output, "\n");*/

        aslprint(str1);
        aslprint(" -> 0x");
        itoa(numero,intero,16);
        aslprint(intero);
        aslprint("\n");
}

void stampasemafori(struct list_head* head){
	semd_t* item;
	aslprint("semafori allocati:\n");
	list_for_each_entry(item,head,s_next){
		aslprintint("semaforo attivo con key",item->s_key);
		aslprintint("con s_value",item->s_value);
		aslprinthex("che si trova all'indirizzo",item);
    }
}


semd_t semd_table[MAXPROC+MAX_DEVICES];
struct list_head semdFree_h;
struct list_head semd_h;

/*
 ########################################################
 #########  Funzioni per la gestione della ASL  #########
 ########################################################
 */
/*
initASL():inizializza le due liste necessarie per la gestione dei semafori.
*/
void initASL(void){
	int i;
	INIT_LIST_HEAD(&semdFree_h);
	INIT_LIST_HEAD(&semd_h);
	for(i=0; i<MAXPROC+MAX_DEVICES;i++ )
	        list_add(&semd_table[i].s_next,&semdFree_h);
}
/*
(puntatore a semaforo) getSemd(id del semaforo): restituisce il puntatore al semaforo
avente id=key della lista dei semafori attivi. se non esiste nessun elemento con tale id,
restituisce NULL.
*/
semd_t* getSemd(int key){
	semd_t* item;
	list_for_each_entry(item,&semd_h,s_next){
			if(item->s_key == key){
				return item;
			}
    }
    return NULL;
}


semd_t* mygetSemd(int key){
	semd_t* item;
	struct list_head* l_next;
	list_for_each_entry(item,&semd_h,s_next){
			if(item->s_key == key){
				//aslprintint("mygetsemd: trovato semaforo attivo con key",key);
				return item;
			}
    }
	//aslprintint("mygetsemd: alloco semaforo con key",key);
    l_next = list_next(&semdFree_h);
	list_del(l_next);
	list_add(l_next,&semd_h);
	item=container_of(l_next,semd_t,s_next);
	item->s_key = key;
	INIT_LIST_HEAD(&item->s_procQ);
	return item;
}


/*
(1 TRUE oppure 0 FALSE) insertBlocked(id del semaforo, puntatore al processo): inserisce
il processo puntato da p nella coda dei processi bloccati dal semaforo con id=key.
se il semaforo non è presente, allora viene allocato un nuovo semaforo.
in caso di errore restituisce 1, altrimenti 0.
*/
int insertBlocked(int key, pcb_t *p){
	semd_t* semd_found = getSemd(key);
	struct list_head* l_next;
	if(semd_found){
		p->p_semkey = key;
		list_add_tail(&p->p_next,&semd_found->s_procQ);
		return FALSE;
	}	
	if(list_empty(&semdFree_h))
		return TRUE;
	l_next = list_next(&semdFree_h);
	list_del(l_next);
	list_add(l_next,&semd_h);
	semd_found=container_of(l_next,semd_t,s_next);
	semd_found->s_key = key;
	INIT_LIST_HEAD(&semd_found->s_procQ);
	p->p_semkey = key;
	list_add_tail(&p->p_next,&semd_found->s_procQ);
	return FALSE;
}

/*
(puntatore al processo) removeBlocked(id del semaforo): ritorna il primo processo della coda associata al semaforo con id=key.
se questo semaforo non esiste, restituisce NULL. se non ci sono più processi bloccati, sposta il semaforo nella lista dei semafori liberi
*/

pcb_t* removeBlocked(int key){
	semd_t* semd_found = getSemd(key);
	pcb_t* first_pcb;
	struct list_head* l_next;
	if(!semd_found)
		return NULL;
	l_next = list_next(&semd_found->s_procQ);
	first_pcb=container_of(l_next,pcb_t,p_next);
	list_del(l_next);
	if(list_empty(&semd_found->s_procQ)){
		list_del(&semd_found->s_next);
		list_add(&semd_found->s_next,&semdFree_h);
	}
	return first_pcb;
}


/*
(puntatore al processo) outBlocked(puntatore al processo): restituisce il processo puntato da p RIMUOVENDOLO dalla lista dei processi bloccati da un semaforo.
se il processo non viene trovato, restituisce NULL.
*/
pcb_t* outBlocked(pcb_t *p){
	semd_t* semd_found = getSemd(p->p_semkey);
	pcb_t* item;
	list_for_each_entry(item,&semd_found->s_procQ,p_next){
		if(p == item){
			list_del(&item->p_next);
			return item;
		}
	}
	return NULL;
}
/*
(puntatore al processo) headBlocked(id del semaforo): restituisce il processo in testa SENZA RIMUOVERLO dalla lista dei processi bloccati dal semaforo con id=key.
se il semaforo non viene trovato oppure se non ha processi in attesa, restituisce NULL.
*/
pcb_t* headBlocked(int key){
	semd_t* semd_found = getSemd(key);
	struct list_head* l_next;
	/*controlla le due condizioni di fallimento*/
	if((!semd_found) || list_empty(&semd_found->s_procQ)) 
		return NULL;
	l_next = list_next(&semd_found->s_procQ);
	return container_of(l_next,pcb_t,p_next);
}
/*
outChildBlocked(puntatore al processo): elimina il processo puntato da p dalla coda del semaforo associato, rimuovendo anche i processi discendenti.
*/
void outChildBlocked(pcb_t *p){
	pcb_t* item;
	if(!list_empty(&p->p_child)){
		/*chiamata ricorsiva per ogni figlio*/
		list_for_each_entry(item,&p->p_child,p_sib){
				outChildBlocked(item);
		}
	}
	/*visita posticipata, quando si arriva qua i discendenti di p sono già stati eliminati*/
	outBlocked(p);
}	
