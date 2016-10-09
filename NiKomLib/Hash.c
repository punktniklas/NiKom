#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "NiKomLib.h"

struct NiKHashNode {
	int id;
	void *data;
	struct NiKHashNode *nextnode;
};

struct NiKHash {
	int tablesize;
	struct NiKHashNode **table;
};

/* Namn: NewNiKHash
*
*  Parametrar: d0 - Storlek p� hashtabellen
*
*  Returv�rden: En pekare till hashtabellen om det lyckades, annars NULL.
*
*  Beskrivning: Skapar en ny hashtabellen med den angivna storleken.
*               En h�gre storlek ger snabbare �tkomster, men drar mer
*               minne. En storlek p� mellan en 1/10 och 1/5 av antalet
*               element man t�nker lagra �r kanske lagom.
*/

NiKHash * __saveds __asm LIBNewNiKHash(register __d0 int tablesize) {
	NiKHash *tmptable;
	if(!(tmptable = (NiKHash *) AllocMem(sizeof(NiKHash),MEMF_CLEAR))) return(NULL);
	tmptable->tablesize = tablesize;
	if(!(tmptable->table = (struct NiKHashNode **) AllocMem(sizeof(struct NiKHashNode *) * tablesize,MEMF_CLEAR))) {
		FreeMem(tmptable,sizeof(NiKHash));
		return(NULL);
	}
	return(tmptable);
}


/* Namn: DeleteNiKHash
*
*  Parametrar: a0 - Pekare till hashtabellen
*
*  Returv�rden: Inga
*
*  Beskrivning: Frig�r hashtabellen. Det utg�s fr�n att den �r t�md p�
*               element innan DeleteNiKHash anropas.
*/

void __saveds __asm LIBDeleteNiKHash(register __a0 NiKHash *hashtable) {
	FreeMem(hashtable->table, sizeof(struct NiKHashNode *) * hashtable->tablesize);
	FreeMem(hashtable,sizeof(NiKHash));
}


/* Namn: InsertNiKHash
*
*  Parametrar: a0 - Pekare till hashtabellen
*              d0 - Id p� datat som ska stoppas in.
*              a1 - Datat som ska stoppas in.
*
*  Returv�rden: TRUE om det lyckades, annars FALSE
*
*  Beskrivning: Stoppar in angivet data med angivet id i hashtabellen.
*/

int __saveds __asm LIBInsertNiKHash(register __a0 NiKHash *hashtable, register __d0 int id,
	register __a1 void *data) {
	int hashvalue;
	struct NiKHashNode *pek, *newnode;
	hashvalue = id % hashtable->tablesize;
	pek = hashtable->table[hashvalue];
	if(!(newnode = (struct NiKHashNode *) AllocMem(sizeof(struct NiKHashNode),MEMF_CLEAR))) return(0);
	newnode->id = id;
	newnode->data = data;
	if(pek == NULL || pek->id > id) {
		newnode->nextnode = pek;
		hashtable->table[hashvalue] = newnode;
		return(1);
	}
	while(pek->nextnode != NULL && pek->nextnode->id < id)
		pek = pek->nextnode;
	newnode->nextnode = pek->nextnode;
	pek->nextnode = newnode;
	return(1);
}


/* Namn: GetNiKHashData
*
*  Parametrar: a0 - Pekare till hashtabellen
*              d0 - Id p� datat som ska plockas fram.
*
*  Returv�rden: Datat om det finns, annars NULL.
*
*  Beskrivning: Plockar fram datat till angivet ID.
*/

void * __saveds __asm LIBGetNiKHashData(register __a0 NiKHash *hashtable, register __d0 int id) {
	int hashvalue;
	struct NiKHashNode *pek;
	hashvalue = id % hashtable->tablesize;
	pek = hashtable->table[hashvalue];
	while(pek) {
		if(pek->id == id) return(pek->data);
		pek = pek->nextnode;
	}
	return(NULL);
}


/* Namn: RemoveNiKHashData
*
*  Parametrar: a0 - Pekare till hashtabellen
*              d0 - Id p� datat som ska tas ur.
*
*  Returv�rden: Datat som tagit bort om det finns, annars NULL.
*
*  Beskrivning: Tar bort datat med angivet ID ur hashtabellen och
*               returnerar det.
*/

void * __saveds __asm LIBRemoveNiKHashData(register __a0 NiKHash *hashtable, register __d0 int id) {
	int hashvalue;
	struct NiKHashNode *pek, *tmppek;
	void *data;
	hashvalue = id % hashtable->tablesize;
	pek = hashtable->table[hashvalue];
	if(pek == NULL) return(NULL);
	if(pek->id == id) {
		hashtable->table[hashvalue] = pek->nextnode;
	} else {
		while(pek->nextnode && pek->nextnode->id != id)
			pek = pek->nextnode;
		if(!pek->nextnode) return(NULL);
		tmppek = pek->nextnode;
		pek->nextnode = pek->nextnode->nextnode;
		pek = tmppek;
	}
	data = pek->data;
	FreeMem(pek,sizeof(struct NiKHashNode));
	return(data);
}
