#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "/Include/NiKomStr.h"
#include "/Include/NiKomLib.h"
#include "NiKomBase.h"
#include "funcs.h"

/*
struct UserGroup {
   struct MinNode grupp_node;
   char namn[41],flaggor,autostatus,nummer;
};
*/

/* Namn: CreateUserGroup
*
*  Parametrar: a0 - En pekare till en taglista.
*
*  Returvärden: tal >= 0 - numret på den nyligen skapade gruppen
*               -1       - Kunde inte allokera minne till gruppen.
*               -2       - Finns inte plats för fler grupper.
*               -3       - Finns redan en grupp med det namnet.
*               -4       - Kunde inte skriva ner gruppen på disk.
*
*  Beskrivning: Skapar en ny grupp. De tags man kan skicka med är:
*                  UG_Name        Pekare till namnet på gruppen.
*                  UG_Flags       Vilka flaggor gruppen ska ha.
*                                  Flaggor: SECRETGROUP - Gruppen är hemlig
*                  UG_Autostatus  Vid vilken status användare automatiskt ska
*                                 betraktas som medlemmar.
*/
/*
struct User {
   long tot_tid,forst_in,senast_in,read,skrivit,flaggor,textpek,brevpek,
      grupper,defarea,reserv2,chrset,reserv4,reserv5,upload,download,
      loggin,shell;
   char namn[41],gata[41],postadress[41],land[41],telefon[21],
      annan_info[61],losen[16],status,rader,protokoll,
      prompt[6],motratt[MAXMOTE/8],motmed[MAXMOTE/8],vote[MAXVOTE];
};
*/
int __saveds __asm LIBCreateUserGroup(register __a0 struct TagItem *taglist, register __a6 struct NiKomBase *NiKomBase) {
	struct UserGroup *allok, *letpek;
	int i;
	if(LIBFindUserGroup(GetTagData(UG_Name,"<Namnlös grupp>",taglist))>=0) return(-3);
	if(!(allok=AllocMem(sizeof(struct UserGroup),MEMF_CLEAR | MEMF_PUBLIC))) return(-1);
	strncpy(allok->namn,GetTagData(UG_Name,"<Namnlös grupp>",taglist),41);
	allok->namn[40] = 0;
	allok->flaggor = GetTagData(UG_Flags,0,taglist);
	allok->autostatus = GetTagData(UG_Autostatus,0,taglist);
	ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	i=0;
	for(letpek=(struct UserGroup *)NiKomBase->Servermem->grupp_list.mlh_Head;
		letpek->grupp_node.mln_Succ;
		letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ) {
		if(letpek->nummer>i) break;
		i++;
	}
	if(i>=32) {
		FreeMem(allok);
		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
		return(-2);
	}
	allok->nummer = i;
	if(!WriteGroup(allok)) {
		FreeMem(allok);
		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
		return(-4);
	}
	Insert((struct List *)&NiKomBase->Servermem->grupp_list,
		(struct Node *)allok,(struct Node *)letpek->grupp_node.mln_Pred);
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	return(allok->nummer);
}

/* Namn: DeleteUserGroup
*
*  Parametrar: d0 - Numret på gruppen som ska raderas.
*
*  Returvärden: 0 - Gruppen raderad.
*               1 - Finns ingen grupp med det numret.
*               2 - Kunde inte skriva på disken.
*
*  Beskrivning: Raderar den angivna gruppen.
*/

int __saveds __asm LIBDeleteUserGroup(register __d0 int group, register __a6 struct NiKomBase *NiKomBase) {
	struct UserGroup *pek;
	char save;
	ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	if(!(pek = GetGroupPek(group))) {
		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
		return(1);
	}
	save = pek->namn[0];
	pek->namn[0] = 0;
	if(!WriteGroup(pek)) {
		pek->namn[0] = save;
		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
		return(2);
	}
	Remove((struct Node *)pek);
	FreeMem(pek,sizeof(struct UserGroup));
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	return(0);
}


/* Namn: SetUserGroupAttrs
*
*  Parametrar: d0 - Numret på gruppen som ska ändras.
*              a0 - Pekare till en taglista.
*
*  Returvärden: 0 - Gruppen ändrad.
*               1 - Finns ingen grupp med det numret.
*
*  Beskrivning: Ändrar data i den angivna gruppen. Tags är samma som för
*               CreateUserGroup().
*/

int __saveds __asm SetUserGroupAttrs(register __d0 int group, register __a0 struct TagItem *taglist,
	register __a6 struct NiKomBase *NiKomBase) {
	struct UserGroup *pek;
	ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	if(!(pek = GetGroupPek(group))) {
		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
		return(1);
	}
	if(FindTagItem(UG_Name,taglist)) {
		strncpy(pek->namn,GetTagData(UG_Name,"",taglist),41);
		pek->namn[40] = 0;
	}
	if(FindTagItem(UG_Flags,taglist)) pek->flaggor = GetTagData(UG_Flags,0,taglist);
	if(FindTagItem(UG_Autostatus,taglist)) pek->autostatus = GetTagData(UG_Autostatus,0,taglist);
	WriteGroup(pek);
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	return(0);
}


/* Namn: GetUserGroupAttrs
*
*  Parametrar: d0 - Numret på gruppen som man vill ha information om.
*              d1 - Ett tag-värde som talar om vilken information man vill ha.
*
*  Returvärden: Det begärda data som en _pekare_. NULL om gruppen inte finns
*               eller om tagvärdet är odefinierat.
*
*  Beskrivning: Plockar ur data ur gruppstrukturen. Tagvärdet bestämmer
*               vilken data. UG_Name ger en strängpekare. Både UG_Flags
*               och UG_Autostatus ger pekare till en char som innehåller
*               värdet.
*/

void * __saveds __asm GetUserGroupAttrs(register __d0 int group, register __d1 int tag,
	register __a6 struct NiKomBase *NiKomBase) {
	struct UserGroup *pek;
	void *retval;
	ObtainSharedSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	if(!(pek = GetGroupPek(group))) {
		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
		return(NULL);
	}
	switch(tag) {
	case UG_Name :
		retval = pek->namn;
		break;
	case UG_Flags :
		retval = &pek->flaggor;
		break;
	case UG_Autostatus :
		retval = &pek->autostatus;
		break;
	default :
		retval = NULL;
	}
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	return(retval);
}


/* Namn: SetGroupAccessList
*
*  Parametrar: d0 - Numret på gruppen som ska adderas.
*              a0 - Pekare till en GAL
*
*  Returvärden: Inga
*
*  Beskrivning: Sätter den angivna gruppen i GAL:en. (Group Access List)
*/


void __saveds __asm SetGroupAccessList(register __d0 int group,
	register __a0 GAL *gal, register __a6 struct NiKomBase *NiKomBase)
{
	BAMSET(gal,group);
}


/* Namn: ClearGroupAccessList
*
*  Parametrar: d0 - Numret på gruppen som ska subtraheras.
*              a0 - Pekare till en GAL
*
*  Returvärden: Inga
*
*  Beskrivning: Rensar den angivna gruppen i GAL:en. (Group Access List)
*/

void __saveds __asm ClearGroupAccessList(register __d0 int group, register __a0 GAL *gal,
	register __a6 struct NiKomBase *NiKomBase) {
	BAMCLEAR(gal,group);
}


/* Namn: TestGroupAccessList
*
*  Parametrar: d0 - Numret på gruppen som ska testar.
*              a0 - Pekare till en GAL
*
*  Returvärden: 0 om gruppen inte är satt i GAL:en. Annars icke-0.
*
*  Beskrivning: Testar om den angivna gruppen är att i GAL:en. (Group Access List)
*/

int __saveds __asm TestGroupAccesList(register __d0 int group, register __a0 GAL *gal,
	register __a6 struct NiKomBase *NiKomBase) {
	BAMTEST(gal,group);
}


/* Namn: InitGroupAccessList
*
*  Parametrar: a0 - Pekare till en GAL
*
*  Returvärden: Inga
*
*  Beskrivning: Nollställer en GAL så att inga grupper är markerade.
*/

void __saveds __asm InitGroupAcessList(register __a0 GAL *gal, register __a6 struct NiKomBase *NiKomBase) {
	*gal = 0;
}


/* Namn: MatchingGroups
*
*  Parametrar: d0 - Numret på användaren som ska kollas.
*              a0 - Pekare till en GAL
*
*  Returvärden: Ickenoll om användaren är medlem i en eller flera grupper
*               som finns markerade i GAL:en. Annars noll.
*
*  Beskrivning: Testar om användaren har tillgång till en eller flera angivna
*               grupper.
*/

int __saveds __asm MatchingGroups(register __d0 int user, register __a0 GAL *gal) {
	GAL *usergal, newgal;
	struct UserGroup *letpek;
	int userstatus;
	usergal = GetUserAttrs(user,U_GAL);
	newgal = *usergal;
	userstatus = GetUserAttrs(user,U_Status);
	ObtainSharedSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	for(letpek=(struct UserGroup *)NiKomBase->Servermem->grupp_list.mlh_Head;
		letpek->grupp_node.mln_Succ;
		letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ) {
		if(userstatus >= letpek->autostatus) SetGroupAccessList(letpek->nummer,*newgal);
	}
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	return(*gal & newgal);
}


/* Namn: IsMemberGroup
*
*  Parametrar: d0 - Numret på användaren som ska kollas.
*              d1 - Numret på gruppen.
*
*  Returvärden: Ickenoll om användaren är medlem i gruppen. Annars noll.
*
*  Beskrivning: Testar om användaren är medlem i den angivna gruppen.
*/

int IsMemberGroup(int user, int group) {
	GAL *usergal;
	int autostatus, userstatus;
	struct UserGroup *pek;
	usergal = GetUserAttrs(user, U_GAL);
	if(TestGroupAccessList(group,usergal)) return(TRUE);
	ObtainSharedSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	pek = GetGroupPek(group);
	autostatus = pek->autostatus;
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_GROUPS]);
	userstatus = GetUserAttrs(user,U_Status);
	if(userstatus >= autostatus) return(TRUE);
	return(FALSE);
}

int MaySeeGroup(int user, int group)
int NextUserGroup(int group)

int _saveds_ __asm LIBFindUserGroup(register __a0 char *groupname, register __a6 struct NiKomBase *NiKomBase))
{
	struct UserGroup *pek;
	
	for(pek=(struct UserGroup *)NiKomBase->Servermem->grupp_list.mlh_Head;
		pek->grupp_node.mln_Succ;
		pek=(struct UserGroup *)letpek->grupp_node.mln_Succ)
	{
		if(!stricmp(groupname,pek->namn))
			return 1;
	}
	
	return 0;
}

/************************* Interna funktioner i biblioteket ******************************/

/* Namn: GetGroupPek
*
*  Parametrar: group - Numret på gruppen man vill ha en pekare på.
*
*  Returvärden: Pekaren till gruppen eller NULL om ingen sådan grupp finns.
*
*
*  Beskrivning: Skriver ner gruppen på disk.
*
*  OBS! Det antas att det redan finns åtminstone en delad semafor på grupperna!
*/

static struct UserGroup *GetGroupPek(int group) {
	struct UserGroup *pek;
	for(pek=(struct UserGroup *)NiKomBase->Servermem->grupp_list.mlh_Head;
		pek->grupp_node.mln_Succ;
		pek=(struct UserGroup *)letpek->grupp_node.mln_Succ)
		if(pek->nummer == group) return(pek);
	return(NULL);
}


/* Namn: WriteGroup
*
*  Parametrar: group - En pekare till gruppen som ska sparas.
*
*  Returvärden: 0 - Det gick dåligt.
*               1 - Det gick bra.
*
*  Beskrivning: Skriver ner gruppen på disk.
*
*  OBS! Det antas att det redan finns en semafor på grupperna!
*/

static int WriteGroup(struct UserGroup *group) {
	BPTR fh;
	if(!(fh=Open("NiKom:DatoCfg/Grupper.dat",MODE_OLDFILE))) {
		return(0);
	}
	if(Seek(fh,group->nummer*sizeof(struct UserGroup),OFFSET_BEGINNING)==-1) {
		Close(fh);
		return(0);
	}
	if(Write(fh,(void *)group,sizeof(struct UserGroup))==-1) {
		Close(fh);
		return(0);
	}
	Close(fh);
	return(1);
}
