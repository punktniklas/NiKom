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

int __saveds AASM LIBSetNodeState(register __d0 int node AREG(d0), register __d1 int state AREG(d1)) {
	return(sendservermess(NIKMESS_SETNODESTATE,-1,node,state,0L));
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
	repport = CreateMsgPort();
	if(repport) {
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
