#include <exec/types.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <devices/console.h>
#include <devices/conunit.h>
#include <devices/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"

#define ERROR	128
#define OK	0
#define EKO	1
#define EJEKO	0
#define NOCARRIER	32

struct IOStdReq *conwritereq=NULL;
struct MsgPort *conwriteport=NULL;
struct IOStdReq *conreadreq=NULL;
struct MsgPort *conreadport=NULL;
struct timerequest *timerreq=NULL;
struct MsgPort *timerport=NULL;


extern int nodnr, ypos, xpos, ysize, xsize, inloggad, nodestate;
extern char pubscreen[];
extern struct Window *NiKwind;
extern struct System *Servermem;
extern struct MsgPort *nikomnodeport;
extern int radcnt;

int rxlinecount, timeout;
char outbuffer[1200], nodid[20];

static int typeaheadbuftkn;
static char coninput;    /* Behövs för SendIO()-anropet */
char typeaheadbuf[51];   /* För att buffra inkommande tecken under utskrift */
static BOOL consoleyes, timeryes;

int OpenIO(struct Window *iowin) {
	if(!(timerport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(timerreq=(struct timerequest *)CreateExtIO(timerport,(LONG)sizeof(struct timerequest))))
		return(FALSE);
	if(OpenTimer(timerreq)) return(FALSE);
	else timeryes=TRUE;

	if(!(conwriteport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(conwritereq=(struct IOStdReq *)CreateExtIO(conwriteport,(LONG)sizeof(struct IOStdReq))))
		return(FALSE);
	if(!(conreadport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(conreadreq=(struct IOStdReq *)CreateExtIO(conreadport,(LONG)sizeof(struct IOStdReq))))
		return(FALSE);
	if(OpenConsole(iowin))
		return(FALSE);
	else consoleyes=TRUE;

	return(TRUE);
}

void CloseIO(void) {
	if(consoleyes) CloseConsole();
	if(conreadreq) DeleteExtIO((struct IORequest *)conreadreq);
	if(conreadport) DeleteMsgPort(conreadport);
	if(conwritereq) DeleteExtIO((struct IORequest *)conwritereq);
	if(conwriteport) DeleteMsgPort(conwriteport);

	if(timeryes) CloseTimer(timerreq);
	if(timerreq) DeleteExtIO((struct IORequest *)timerreq);
	if(timerport) DeleteMsgPort(timerport);
}

BYTE OpenConsole (struct Window *window) {
	BYTE error;
	conwritereq->io_Data = (APTR) window;
	conwritereq->io_Length = sizeof(struct Window);
	error=OpenDevice("console.device",CONU_CHARMAP,(struct IORequest *)conwritereq,CONFLAG_DEFAULT);
	conreadreq->io_Device=conwritereq->io_Device;
	conreadreq->io_Unit=conwritereq->io_Unit;
	return(error);
}

BYTE OpenTimer(struct timerequest *treq) {
	BYTE error;
	error=OpenDevice(TIMERNAME,UNIT_VBLANK,(struct IORequest *)treq,0);
	if(!error) {
		treq->tr_node.io_Command=TR_ADDREQUEST;
		treq->tr_node.io_Message.mn_ReplyPort=timerport;
		treq->tr_time.tv_secs=0;
		treq->tr_time.tv_micro=10;
		if(DoIO((struct IORequest *)timerreq)) printf("Fel DoIO i OpenTimer()\n");
	}
	return(error);
}

void CloseConsole(void)
{
	if(!(CheckIO((struct IORequest *)conreadreq))) {
		AbortIO((struct IORequest *)conreadreq);
		WaitIO((struct IORequest *)conreadreq);
	}
	if(!(CheckIO((struct IORequest *)conwritereq))) {
		AbortIO((struct IORequest *)conwritereq);
		WaitIO((struct IORequest *)conwritereq);
	}
	CloseDevice((struct IORequest *)conwritereq);
}

void CloseTimer(struct timerequest *treq) {
	if(!(CheckIO((struct IORequest *)timerreq))) {
		AbortIO((struct IORequest *)timerreq);
		WaitIO((struct IORequest *)timerreq);
	}
	CloseDevice((struct IORequest *)treq);
}

char gettekn(void)
{
	struct IntuiMessage *mymess;
	struct NiKMess *nikmess;
	ULONG signals,conreadsig=1L << conreadport->mp_SigBit,windsig=1L << NiKwind->UserPort->mp_SigBit,
		nikomnodesig = 1L << nikomnodeport->mp_SigBit;
	char tkn=0,tmp[51];
	if(typeaheadbuftkn)
	{
		tkn=typeaheadbuf[0];
		strcpy(tmp,&typeaheadbuf[1]);
		strcpy(typeaheadbuf,tmp);
		typeaheadbuftkn--;
	}
	else
	{
		while(!tkn)
		{
			signals=Wait(conreadsig | windsig | nikomnodesig);
			if(signals & conreadsig)
				tkn=congettkn();

			if(signals & windsig)
			{
				mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
				ReplyMsg((struct Message *)mymess);
				cleanup(OK,"");
			}
			if(signals & nikomnodesig)
			{
				while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport))
				{
					handleservermess(nikmess);
					ReplyMsg((struct Message *)nikmess);
				}
				if(carrierdropped()) tkn = '\n';
			}
		}
	}
	return(tkn);
}

char congettkn(void)
{
	char temp;
	WaitIO((struct IORequest *)conreadreq);
	temp=coninput;
	conreqtkn();
	return((char)temp);
}

void eka(char tecken) {
	conwritereq->io_Command=CMD_WRITE;
	conwritereq->io_Data=(APTR)&tecken;
	conwritereq->io_Length=1;
	DoIO((struct IORequest *)conwritereq);
	if(tecken == '\n')
		incantrader();
}

void conreqtkn(void)
{
	conreadreq->io_Command= CMD_READ;
	conreadreq->io_Data = (APTR)&coninput;
	conreadreq->io_Length = 1;
	SendIO((struct IORequest *)conreadreq);
}

void putstring(char *pekare,int size, long flags)
{
/*	int x;
	char *tmppek, buffer[8193];

	tmppek = pekare;
	x = 0;
	while(x < size || x == -1)
	{
		if(size < 1 && tmppek[0] == 0)
		{
			if(tmppek != pekare)
			{
				conwritereq->io_Command=CMD_WRITE;
				conwritereq->io_Data=(APTR)pekare;
				conwritereq->io_Length=size;
				DoIO((struct IORequest *)conwritereq);
			}
			break;
		}
		if(size > 0 && x++ == size) break;
		if(tmppek[0] == '\n')
		{
			incantrader();
			tmppek[0] = 0;
			buffer[0] = NULL;
			strcpy(buffer, pekare);
			strcat(buffer, "\n");
			conwritereq->io_Command=CMD_WRITE;
			conwritereq->io_Data=(APTR)buffer;
			conwritereq->io_Length=size;
			DoIO((struct IORequest *)conwritereq);
			pekare = tmppek+1;
		}
		tmppek++;
	} */

	conwritereq->io_Command=CMD_WRITE;
	conwritereq->io_Data=(APTR)pekare;
	conwritereq->io_Length=size;
	DoIO((struct IORequest *)conwritereq);
}

int puttekn(char *pekare,int size) {
	int aborted=FALSE,x, abort;
	char *tmppek, buffer[1201], constring[1201];

	strncpy(constring,pekare,1199);
	constring[1199]=0;
	pekare = &constring[0];

	if(!(Servermem->inne[nodnr].flaggor & ANSICOLOURS)) StripAnsiSequences(pekare);

	if(size == -1 && !pekare[0]) return(aborted);

	tmppek = pekare;
	x = 0;
	while((x < size || size == -1) && !aborted)
	{
		if(size < 1 && tmppek[0] == 0)
		{
			if(tmppek != pekare)
			{
				if(sendtocon(pekare, size)) aborted = TRUE;
			}
			break;
		}
		if(size > 0 && x++ == size)
		{
			if(pekare != tmppek)
			{
				tmppek[1] = 0;
				if(sendtocon(pekare, -1)) aborted = TRUE;
			}
			break;
		}
		if(tmppek[0] == '\n')
		{
			buffer[0] = 0;
			if(tmppek != pekare)
			{
				tmppek[0] = 0;
				if(pekare[0] != 0)
					strcpy(buffer, pekare);

				strcat(buffer, "\n");
				pekare = tmppek + 1;
			}
			else
			{
				buffer[0] = '\n';
				buffer[1] = 0;
				pekare = tmppek + 1;
			}
			x++;

			if(!aborted)
				if(sendtocon(&buffer[0], -1)) aborted = TRUE;
			if(incantrader()) aborted = TRUE;
			tmppek = pekare - 1;
		}
		tmppek++;
	}

	return(aborted);
}

int checkcharbuffer(void)
{
	char tecken;
	ULONG conreadsig = 1L << conreadport->mp_SigBit;

	while(CheckSignal(conreadsig) & conreadsig)
	{
		tecken=congettkn();
		if(tecken && typeaheadbuftkn<50)
		{
			typeaheadbuf[typeaheadbuftkn++]=tecken;
			typeaheadbuf[typeaheadbuftkn]=0;
		}
	}

	return typeaheadbuftkn;
}

int sendtocon(char *pekare, int size)
{
	struct IntuiMessage *mymess;
	struct NiKMess *nikmess;
	int aborted = FALSE, paused=FALSE;
	ULONG signals,conwritesig = 1L << conwriteport->mp_SigBit,
		conreadsig = 1L << conreadport->mp_SigBit,windsig = 1L << NiKwind->UserPort->mp_SigBit,
		nikomnodesig = 1L << nikomnodeport->mp_SigBit;
	char console = 1, tecken;

	conwritereq->io_Command=CMD_WRITE;
	conwritereq->io_Data=(APTR)pekare;
	conwritereq->io_Length=size;
	SendIO((struct IORequest *)conwritereq);
	while(console)
	{
		signals = Wait(conwritesig | conreadsig | windsig | nikomnodesig);
		if(signals & conwritesig) {
			console=0;
			if(WaitIO((struct IORequest *)conwritereq)) printf("Error console\n");
		}
		if(signals & conreadsig) {
			if((tecken=congettkn()) == 3) {
				if(console) {
					AbortIO((struct IORequest *)conwritereq);
					WaitIO((struct IORequest *)conwritereq);
				}
				aborted=TRUE;
				console=0;
				putstring("^C\n\r",-1,0);
			} else if((tecken==' ' && (Servermem->inne[nodnr].flaggor & MELLANSLAG)) || tecken==19) paused=TRUE;
			else if(tecken && typeaheadbuftkn<50) {
				typeaheadbuf[typeaheadbuftkn++]=tecken;
				typeaheadbuf[typeaheadbuftkn]=0;
			}
		}
		if(signals & windsig) {
			mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
			ReplyMsg((struct Message *)mymess);
			cleanup(OK,"");
		}
		if(signals & nikomnodesig) {
			while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport)) {
				handleservermess(nikmess);
				ReplyMsg((struct Message *)nikmess);
			}
			if(carrierdropped()) aborted = TRUE;
		}
	}
	if(paused && gettekn()==3)
	{
		putstring("^C\n\r",-1,0);
		return(TRUE);
	}

	return(aborted);
}

int carrierdropped(void) {
	if(nodestate & (NIKSTATE_NOCARRIER | NIKSTATE_LOGOUT)) return(TRUE);
	return(FALSE);
}


void getnodeconfig(char *configname) {
	FILE *fp;
	char buffer[100];
	int len;
	if(!(fp=fopen(configname,"r"))) {
		printf("Kunde inte öppna %s.\n",configname);
		cleanup(ERROR,"");
	}
	while(fgets(buffer,99,fp)) {
		len = strlen(buffer);
		if(buffer[len - 1] == '\n') buffer[len - 1] = 0;
		if(!strncmp(buffer,"SCREEN",6)) {
			strncpy(pubscreen,&buffer[7],39);
			pubscreen[39] = 0;
		}
		else if(!strncmp(buffer,"YPOS",4)) ypos=atoi(&buffer[5]);
		else if(!strncmp(buffer,"XPOS",4)) xpos=atoi(&buffer[5]);
		else if(!strncmp(buffer,"YSIZE",5)) ysize=atoi(&buffer[6]);
		else if(!strncmp(buffer,"XSIZE",5)) xsize=atoi(&buffer[6]);
		else if(!strncmp(buffer,"NODEID",6)) {
			strncpy(nodid,&buffer[7],19);
			nodid[19] = 0;
		}
	}
	fclose(fp);
}

void abortinactive(void) {
}

int updateinactive(void) {
	return(0);
}

#define FIFOEVENT_FROMUSER  1
#define FIFOEVENT_FROMFIFO  2
#define FIFOEVENT_CLOSEW    4
#define FIFOEVENT_NOCARRIER 8

int getfifoevent(struct MsgPort *fifoport, char *puthere)
{
	struct IntuiMessage *mymess;
	struct NiKMess *nikmess;
	ULONG signals,conreadsig=1L << conreadport->mp_SigBit,windsig=1L << NiKwind->UserPort->mp_SigBit,
		fifosig=1L << fifoport->mp_SigBit, nikomnodesig = 1L << nikomnodeport->mp_SigBit;
	int event=0;
	while(!event) {
		signals = Wait(conreadsig | windsig | fifosig | nikomnodesig | SIGBREAKF_CTRL_C);
		if((signals & conreadsig) && CheckIO((struct IORequest *)conreadreq)) {
			*puthere=congettkn();
			event|=FIFOEVENT_FROMUSER;
		}
		if(signals & windsig) {
			mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
			ReplyMsg((struct Message *)mymess);
			AbortIO((struct IORequest *)conreadreq);
			WaitIO((struct IORequest *)conreadreq);
			event|=FIFOEVENT_CLOSEW;
		}
		if(signals & SIGBREAKF_CTRL_C) {
			AbortIO((struct IORequest *)conreadreq);
			WaitIO((struct IORequest *)conreadreq);
			event|=FIFOEVENT_CLOSEW;
		}
		if(signals & fifosig) {
			event|=FIFOEVENT_FROMFIFO;
		}
		if(signals & nikomnodesig) {
			while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport)) {
				handleservermess(nikmess);
				ReplyMsg((struct Message *)nikmess);
			}
			if(carrierdropped()) event |= FIFOEVENT_NOCARRIER;
		}
	}
	return(event);
}

int serputtekn(char *pekare,int size)
{
	return(0);
}

int sendtoser(char *pekare, int size)
{
	return(0);
}

void sereka(char tecken)
{

}
