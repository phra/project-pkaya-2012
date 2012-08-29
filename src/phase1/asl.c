#include "const.h"
#include "uMPStypes.h"
#include "listx.h"
#include "types11.h"

struct list_head semdFree_h;
semd_t semd_table[MAXPROC+MAX_DEVICES];
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
