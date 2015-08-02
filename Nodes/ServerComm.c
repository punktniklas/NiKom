#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <exec/ports.h>
#include <stdio.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include <proto/exec.h>
#include <exec/exec.h>

static struct NiKMess *servermess;
static struct MsgPort *serverport, *NiKomPort, *permitport;

int nodnr = -1, nodestate;
struct System *Servermem;

int initnode(int type) {
	if(!(serverport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(servermess=(struct NiKMess *)AllocMem(sizeof(struct NiKMess), MEMF_PUBLIC | MEMF_CLEAR)))
		return(FALSE);

	servermess->stdmess.mn_Node.ln_Type = NT_MESSAGE;
	servermess->stdmess.mn_Length = sizeof(struct NiKMess);
	servermess->stdmess.mn_ReplyPort=serverport;

	if(type == NODSPAWNED) {
		servermess->kommando = GETADRESS;
		servermess->nod = nodnr;
	} else {
		servermess->kommando=NYNOD;
		servermess->data=type;
	}

	if(!(NiKomPort=SafePutToPort(servermess,"NiKomPort"))) {
		printf("Hittar inte NiKServer\n");
		return(FALSE);
	}
	WaitPort(serverport);
	GetMsg(serverport);
	if(!(Servermem=(struct System *)servermess->data))
		return(FALSE);
	nodnr = servermess->nod;
	if(!(permitport=(struct MsgPort *)FindPort("NiKomPermit")))
		return(FALSE);
	return(TRUE);
}

void shutdownnode(int type) {
	if((type != NODSPAWNED) && (nodnr!=-1)) {
		Servermem->inloggad[nodnr]=-1;
		Servermem->action[nodnr]=0;
		servermess->kommando=NODSLUTAR;
		servermess->nod=nodnr;
		PutMsg(NiKomPort,(struct Message *)servermess);
		WaitPort(serverport);
		GetMsg(serverport);
	}
	if(servermess) FreeMem(servermess,sizeof(struct NiKMess));
	if(serverport) DeleteMsgPort(serverport);
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

void NiKForbid(void) {
	servermess->kommando=FORBID;
	PutMsg(NiKomPort,(struct Message *)servermess);
	WaitPort(serverport);
	GetMsg(serverport);
}

void NiKPermit(void) {
	PutMsg(permitport,(struct Message *)servermess);
	WaitPort(serverport);
	GetMsg(serverport);
}

long sendservermess(short kommando, long data) {
	servermess->kommando=kommando;
	servermess->data=data;
	servermess->nod=nodnr;
	PutMsg(NiKomPort,(struct Message *)servermess);
	WaitPort(serverport);
	GetMsg(serverport);
	return(servermess->data);
}

void handleservermess(struct NiKMess *mess) {
	if(mess->kommando == NIKMESS_SETNODESTATE) nodestate = mess->data;
}
