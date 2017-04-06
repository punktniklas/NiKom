#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

/*
 * Namn:        SetNodeState
 *
 * Parametrar:  d0 - Vilken nod som ska beröras. -1 för samtliga noder
 *              d1 - Vilket läge, enligt definitioner i NiKomLib.h
 *
 * Returvärden: 0 om det misslyckades, annars icke-noll. Ett mislyckande beror
 *              förmodligen på att noden inte fanns.
 *
 * Beskrivning: Skickar ett meddelande till Servern att noden eller noderna ska
 *              försättas i angivet läge. Vilka lägen som finns är definierade
 *              i NiKomLib.h.
 */

int __saveds __asm LIBSetNodeState(register __d0 int node, register __d1 int state) {
	return(sendservermess(NIKMESS_SETNODESTATE,-1,node,state,0L));
}

/*
 * Name:        SendNodeMessage
 *
 * Parametrar:  d0 - Vilken nod meddelandet ska skickas till. -1 för samtliga noder.
 *              d1 - Från vilken användare.
 *              a0 - En pekare till strängen som ska skickas.
 *
 * Returvärden: 0 för misslyckande. 1 om mottagaren inte har olästa meddelanden.
 *              2 om användaren har olästa meddelanden. När meddelandet ska skickas till
 *              alla noder returneras det antal noder som meddelandet gått fram till.
 *				-1 om noden inte finns!
 *
 * Beskrivning: Skickar meddelandet som ett säg-meddelande till den angivna noden/noderna.
 *              Om man sätter avsändare till -1 tolkas det som ett systemmeddelande.
 */

int __saveds __asm LIBSendNodeMessage(register __d0 int node, register __d1 int from, register __a0 char *str,
	register __a6 struct NiKomBase *NiKomBase) {
	int i, ret = 0;

	if(node > MAXNOD) return(-1);
	if(node != -1) {
		if(NiKomBase->Servermem->nodtyp[node] == 0) return(0);
		else return(linksaystring(node, from, str, NiKomBase));
	} else {
		for(i = 0; i < MAXNOD; i++) {
			if(NiKomBase->Servermem->nodtyp[i] == 0) continue;
			if(linksaystring(i,from,str, NiKomBase)) ret++;
		}
		return(ret);
	}
}

int linksaystring(int node, int from, char *str, struct NiKomBase *NiKomBase) {
	struct SayString *ss, *pek;
	int ret;
	if(ss = AllocMem(sizeof(struct SayString),MEMF_CLEAR | MEMF_PUBLIC)) {
		ss->fromuser = from;
		ss->timestamp = time(NULL);
		strncpy(ss->text,str,MAXSAYTKN - 1);
		ss->text[MAXSAYTKN - 1] = 0;
		Forbid();
		if(NiKomBase->Servermem->say[node]) {
			pek = NiKomBase->Servermem->say[node];
			while(pek->NextSay) pek = pek->NextSay;
			pek->NextSay = ss;
			ret = 2;
		} else {
			NiKomBase->Servermem->say[node] = ss;
			ret = 1;
		}
		Permit();
		return(ret);
	}
	return(0);
}

struct MsgPort *SafePutToPort(message,portname)
struct NiKMess *message;
char *portname;
{
	struct MsgPort *port;
	Forbid();
	port=(struct MsgPort *)FindPort(portname);
	if(port) PutMsg(port,(struct Message *)message);
	Permit();
	return(port);
}

long sendservermess(short kommando, long nod, long data, long extdata1, long extdata2) {
	struct NiKMess mess;
	struct MsgPort *repport;
	if(repport = CreateMsgPort()) {
		memset(&mess,0,sizeof(struct NiKMess));
		mess.stdmess.mn_Node.ln_Type = NT_MESSAGE;
		mess.stdmess.mn_Length = sizeof(struct NiKMess);
		mess.stdmess.mn_ReplyPort = repport;
		mess.kommando=kommando;
		mess.data=data;
		mess.nod=nod;
		mess.extdata1 = extdata1;
		mess.extdata2 = extdata2;
		if(SafePutToPort(&mess,"NiKomPort")) {
			WaitPort(repport);
			GetMsg(repport);
		}
		else mess.data = 0L;
		DeleteMsgPort(repport);
	}
	return(mess.data);
}
