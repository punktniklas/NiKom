#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <devices/serial.h>
#include <devices/console.h>
#include <devices/conunit.h>
#include <devices/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "BasicIO.h"

#define ERROR	128
#define OK	0
#define NOCARRIER	32

struct IOStdReq *conwritereq=NULL;
struct MsgPort *conwriteport=NULL;
struct IOStdReq *conreadreq=NULL;
struct MsgPort *conreadport=NULL;
struct IOExtSer *serwritereq=NULL;
struct MsgPort *serwriteport=NULL;
struct IOExtSer *serreadreq=NULL;
struct MsgPort *serreadport=NULL;
struct IOExtSer *serchangereq=NULL;
struct MsgPort *serchangeport=NULL;
struct timerequest *timerreq=NULL,*inactivereq=NULL;
struct MsgPort *timerport=NULL,*inactiveport=NULL;


extern int nodnr, ypos, xpos, ysize, xsize, inloggad, nodestate;
extern char pubscreen[];
extern struct Window *NiKwind;
extern struct System *Servermem;
extern struct MsgPort *nikomnodeport;

/* Dessa används bara i PreNoden */
extern char svara[], init[], hangup[], nodid[];
extern int highbaud, hst, getty, hangupdelay, gettybps, autoanswer, plussa;
extern int radcnt;

int dtespeed, handskakning = 2, rxlinecount;
char outbuffer[1200];
char zinitstring[51];

static int typeaheadbuftkn;
static int unit;                /* Unit-nummer, från SerNode.cfg */
static char coninput, serinput; /* Behövs för SendIO()-anropet */
char typeaheadbuf[51];   /* För att buffra inkommande tecken under utskrift */
static char device[31];         /* Devicenamnet, från SerNode.cfg */
static BOOL consoleyes, serialyes, timeryes;

int OpenIO(struct Window *iowin) {
	if(!(timerport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(timerreq=(struct timerequest *)CreateExtIO(timerport,(LONG)sizeof(struct timerequest))))
		return(FALSE);
	if(!(inactiveport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(inactivereq=(struct timerequest *)CreateExtIO(inactiveport,(LONG)sizeof(struct timerequest))))
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

	if(!(serwriteport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(serwritereq=(struct IOExtSer *)CreateExtIO(serwriteport,(LONG)sizeof(struct IOExtSer))))
		return(FALSE);
	if(!(serreadport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(serreadreq=(struct IOExtSer *)CreateExtIO(serreadport,(LONG)sizeof(struct IOExtSer))))
		return(FALSE);
	if(!(serchangeport=(struct MsgPort *)CreateMsgPort()))
		return(FALSE);
	if(!(serchangereq=(struct IOExtSer *)CreateExtIO(serchangeport,(LONG)sizeof(struct IOExtSer))))
		return(FALSE);
	if(OpenSerial(serwritereq,serreadreq,serchangereq))
		return(FALSE);
	else serialyes=TRUE;
	return(TRUE);
}

void CloseIO(void) {
	if(serialyes && !(nodestate & NIKSTATE_CLOSESER)) CloseSerial(serwritereq);
	if(serreadreq) DeleteExtIO((struct IORequest *)serreadreq);
	if(serreadport) DeleteMsgPort(serreadport);
	if(serwritereq) DeleteExtIO((struct IORequest *)serwritereq);
	if(serwriteport) DeleteMsgPort(serwriteport);
	if(serchangereq) DeleteExtIO((struct IORequest *)serchangereq);
	if(serchangeport) DeleteMsgPort(serchangeport);

	if(consoleyes) CloseConsole();
	if(conreadreq) DeleteExtIO((struct IORequest *)conreadreq);
	if(conreadport) DeleteMsgPort(conreadport);
	if(conwritereq) DeleteExtIO((struct IORequest *)conwritereq);
	if(conwriteport) DeleteMsgPort(conwriteport);

	if(timeryes) CloseTimer(timerreq);
	if(inactivereq) DeleteExtIO((struct IORequest *)inactivereq);
	if(inactiveport) DeleteMsgPort(inactiveport);
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

BYTE OpenSerial(writereq,readreq,changereq)
struct IOExtSer *writereq,*readreq,*changereq;
{
	BYTE error;
	int sererr;

	if(handskakning == 2)
		writereq->io_SerFlags= SERF_XDISABLED | SERF_7WIRE | SERF_SHARED;
	else
	{
		if(handskakning == 1)
			writereq->io_SerFlags= SERF_SHARED;
		else
			writereq->io_SerFlags= SERF_XDISABLED | SERF_SHARED;
	}

	if(error=OpenDevice(device,unit,(struct IORequest *)writereq,0))
		cleanup(ERROR,"Kunde inte öppna devicet\n");
	writereq->io_Baud=dtespeed;
	writereq->io_RBufLen=16384;
	writereq->io_ReadLen=8;
	writereq->io_WriteLen=8;
	writereq->io_StopBits=1;

	if(handskakning == 2)
		writereq->io_SerFlags= SERF_XDISABLED | SERF_7WIRE | SERF_SHARED;
	else
	{
		if(handskakning == 1)
			writereq->io_SerFlags= SERF_SHARED;
		else
			writereq->io_SerFlags= SERF_XDISABLED | SERF_SHARED;
	}

	writereq->IOSer.io_Command = SDCMD_SETPARAMS;
	if(sererr=DoIO((struct IORequest *)writereq)) writesererr(sererr);
	CopyMem(writereq,readreq,sizeof(struct IOExtSer));
	readreq->IOSer.io_Message.mn_ReplyPort = serreadport;
	CopyMem(writereq,changereq,sizeof(struct IOExtSer));
	changereq->IOSer.io_Message.mn_ReplyPort = serchangeport;
	return(error);
}

BYTE OpenTimer(struct timerequest *treq) {
	BYTE error;
	error=OpenDevice(TIMERNAME,UNIT_VBLANK,(struct IORequest *)treq,0);
	if(!error) {
		inactivereq->tr_node.io_Device=treq->tr_node.io_Device;
		inactivereq->tr_node.io_Unit=treq->tr_node.io_Unit;
		treq->tr_node.io_Command=TR_ADDREQUEST;
		treq->tr_node.io_Message.mn_ReplyPort=timerport;
		treq->tr_time.tv_secs=0;
		treq->tr_time.tv_micro=10;
		if(DoIO((struct IORequest *)timerreq)) printf("Fel DoIO i OpenTimer()\n");
		inactivereq->tr_node.io_Command=TR_ADDREQUEST;
		inactivereq->tr_node.io_Message.mn_ReplyPort=inactiveport;
		inactivereq->tr_time.tv_secs=0;
		inactivereq->tr_time.tv_micro=10;
		if(DoIO((struct IORequest *)inactivereq)) printf("Fel DoIO i OpenTimer()\n");
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

void CloseSerial(struct IOExtSer *writereq) {
	if(!(CheckIO((struct IORequest *)serreadreq))) {
		AbortIO((struct IORequest *)serreadreq);
		WaitIO((struct IORequest *)serreadreq);
	}
	if(!(CheckIO((struct IORequest *)serwritereq))) {
		AbortIO((struct IORequest *)serwritereq);
		WaitIO((struct IORequest *)serwritereq);
	}
	CloseDevice((struct IORequest *)writereq);
}

void CloseTimer(struct timerequest *treq) {
	if(!(CheckIO((struct IORequest *)timerreq))) {
		AbortIO((struct IORequest *)timerreq);
		WaitIO((struct IORequest *)timerreq);
	}
	if(!(CheckIO((struct IORequest *)inactivereq))) {
		AbortIO((struct IORequest *)inactivereq);
		WaitIO((struct IORequest *)inactivereq);
	}
	CloseDevice((struct IORequest *)treq);
}

/* Följande define saknades i devices/serial.h */
#define SerErr_UnitBusy 16

void writesererr(int errcode) {
	char *errstr;
	switch(errcode) {
		case SerErr_DevBusy:
			errstr="Device in use";
			break;
		case SerErr_BaudMismatch:
			errstr="Baud rate not supported by hardware";
			break;
		case SerErr_BufErr:
			errstr="Failed to allocate new readbuffer";
			break;
		case SerErr_InvParam:
			errstr="Bad parameter";
			break;
		case SerErr_LineErr:
			errstr="Hardware data overrun";
			break;
		case SerErr_ParityErr:
			errstr="Parity Error";
			break;
		case SerErr_TimerErr:
			errstr="Timeout";
			break;
		case SerErr_BufOverflow:
			errstr="Read buffer overflowed";
			break;
		case SerErr_NoDSR:
			errstr="No Data Set Ready";
			break;
		case SerErr_DetectedBreak:
			errstr="Break detected";
			break;
		case SerErr_UnitBusy:
			errstr="Selected unit already in use";
			break;
		default:
			errstr="Unknown errorcode";
			break;
	}
	printf("Serial error on node %d (%s, unit %d): %s\n",nodnr,device,unit,errstr);
}

char gettekn(void) {
  struct IntuiMessage *mymess;
  struct NiKMess *nikmess;
  ULONG signals,
    conreadsig = 1L << conreadport->mp_SigBit,
    windsig = 1L << NiKwind->UserPort->mp_SigBit,
    serreadsig = 1L << serreadport->mp_SigBit,
    inactivesig=1L << inactiveport->mp_SigBit,
    nikomnodesig = 1L << nikomnodeport->mp_SigBit;
  char ch = '\0', tmp[51];

  if(ImmediateLogout()) {
    return 0;
  }

  if(typeaheadbuftkn > 0) {
    ch = typeaheadbuf[0];
    strcpy(tmp, &typeaheadbuf[1]);
    strcpy(typeaheadbuf, tmp);
    typeaheadbuftkn--;
    return ch;
  }

  while(!ch) {
    signals = Wait(conreadsig | windsig | serreadsig | inactivesig | nikomnodesig
                   | SIGBREAKF_CTRL_C);
    if((signals & conreadsig) && CheckIO((struct IORequest *)conreadreq)) {
      ch = congettkn();
      UpdateInactive();
    }
    if((signals & serreadsig) && CheckIO((struct IORequest *)serreadreq)) {
      ch = sergettkn();
      ConvChrsToAmiga(&ch,1,Servermem->inne[nodnr].chrset);
      UpdateInactive();
      QueryCarrierDropped();
    }
    if(signals & windsig) {
      mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
      ReplyMsg((struct Message *)mymess);
      AbortIO((struct IORequest *)conreadreq);
      WaitIO((struct IORequest *)conreadreq);
      AbortIO((struct IORequest *)serreadreq);
      WaitIO((struct IORequest *)serreadreq);
      AbortIO((struct IORequest *)inactivereq);
      WaitIO((struct IORequest *)inactivereq);
      cleanup(OK,"");
    }
    if(signals & SIGBREAKF_CTRL_C) {
      AbortIO((struct IORequest *)conreadreq);
      WaitIO((struct IORequest *)conreadreq);
      AbortIO((struct IORequest *)serreadreq);
      WaitIO((struct IORequest *)serreadreq);
      AbortIO((struct IORequest *)inactivereq);
      WaitIO((struct IORequest *)inactivereq);
      cleanup(OK,"");
    }
    if((signals & inactivesig) && CheckIO((struct IORequest *)inactivereq)) {
      WaitIO((struct IORequest *)inactivereq);
      nodestate |= NIKSTATE_INACTIVITY;
    }
    if(signals & nikomnodesig) {
      while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport)) {
        handleservermess(nikmess);
        ReplyMsg((struct Message *)nikmess);
      }
    }

    if(ImmediateLogout()) {
      return 0;
    }
  }
  return(ch);
}

char congettkn(void) {
  char temp;
  WaitIO((struct IORequest *)conreadreq);
  temp = coninput;
  conreqtkn();
  return temp ;
}

char sergettkn(void) {
  char temp;
  int err;
  if(err = WaitIO((struct IORequest *)serreadreq)) {
    writesererr(err);
  }
  temp = serinput;
  serreqtkn();
  return temp;
}

void eka(char tecken) {
	int err;
	char sertkn;
	conwritereq->io_Command=CMD_WRITE;
	conwritereq->io_Data=(APTR)&tecken;
	conwritereq->io_Length=1;
	if(DoIO((struct IORequest *)conwritereq)) printf("DoIO i eka() (1)\n");
	sertkn=tecken;
	ConvChrsFromAmiga(&sertkn,1,Servermem->inne[nodnr].chrset);
	serwritereq->IOSer.io_Command=CMD_WRITE;
	serwritereq->IOSer.io_Data=(APTR)&sertkn;
	serwritereq->IOSer.io_Length=1;
	if(err=DoIO((struct IORequest *)serwritereq)) writesererr(err);

	if(tecken == '\n')
		incantrader();
}

void sereka(char tecken)
{
	int err;
	char sertkn;
	sertkn=tecken;
	ConvChrsFromAmiga(&sertkn,1,Servermem->inne[nodnr].chrset);
	serwritereq->IOSer.io_Command=CMD_WRITE;
	serwritereq->IOSer.io_Data=(APTR)&sertkn;
	serwritereq->IOSer.io_Length=1;
	if(err=DoIO((struct IORequest *)serwritereq)) writesererr(err);

	if(tecken == '\n')
		incantrader();
}

void coneka(char tecken) {
	conwritereq->io_Command=CMD_WRITE;
	conwritereq->io_Data=(APTR)&tecken;
	conwritereq->io_Length=1;
	if(DoIO((struct IORequest *)conwritereq)) printf("DoIO i coneka()\n");
}

void conreqtkn(void)
{
	conreadreq->io_Command= CMD_READ;
 	conreadreq->io_Data = (APTR)&coninput;
	conreadreq->io_Length = 1;
	SendIO((struct IORequest *)conreadreq);
}

void serreqtkn(void) {
	serreadreq->IOSer.io_Command=CMD_READ;
	serreadreq->IOSer.io_Data=(APTR)&serinput;
	serreadreq->IOSer.io_Length=1;
	SendIO((struct IORequest *)serreadreq);
}

void putstring(char *pekare,int size, long flags) {
	int err;
	char serpekare[200];

	strncpy(serpekare,pekare,199);
	serpekare[199] = 0;
	if(!(flags & PS_NOCONV)) ConvChrsFromAmiga(serpekare,0,Servermem->inne[nodnr].chrset);
	serwritereq->IOSer.io_Command=CMD_WRITE;
	serwritereq->IOSer.io_Data=serpekare;
	serwritereq->IOSer.io_Length=strlen(serpekare);
	SendIO((struct IORequest *)serwritereq);
	conwritereq->io_Command=CMD_WRITE;
	conwritereq->io_Data=(APTR)pekare;
	conwritereq->io_Length=size;
	DoIO((struct IORequest *)conwritereq);

	if(err=WaitIO((struct IORequest *)serwritereq)) writesererr(err);
}

void serputstring(char *pekare,int size, long flags)
{
	char serpekare[200];

	strncpy(serpekare,pekare,199);
	serpekare[199] = 0;
	if(!(flags & PS_NOCONV)) ConvChrsFromAmiga(serpekare,0,Servermem->inne[nodnr].chrset);
	serwritereq->IOSer.io_Command=CMD_WRITE;
	serwritereq->IOSer.io_Data=serpekare;
	serwritereq->IOSer.io_Length=strlen(serpekare);
	DoIO((struct IORequest *)serwritereq);
}

int ImmediateLogout(void) {
  return nodestate & (NIKSTATE_NOCARRIER | NIKSTATE_LOGOUT | NIKSTATE_INACTIVITY);
}

int ConnectionLost(void) {
  return nodestate & NIKSTATE_NOCARRIER;
}

void QueryCarrierDropped(void) {
  int err;
  serchangereq->IOSer.io_Command=SDCMD_QUERY;
  if(err = DoIO((struct IORequest *)serchangereq)) {
    writesererr(err);
  }
  if(serchangereq->io_Status & NOCARRIER) {
    nodestate |= NIKSTATE_NOCARRIER;
  }
}

int tknwaiting(void) {
	return(CheckIO((struct IORequest *)serreadreq)
	       || CheckIO((struct IORequest *)conreadreq));
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
		if(!strncmp(buffer,"INITSTRING",10)) {
			strncpy(init,&buffer[11],80);
			init[80] = 0;
		}
		else if(!strncmp(buffer,"ANSWERSTRING",12)) {
			if(buffer[13]=='-') autoanswer=TRUE;
			else {
				strncpy(svara,&buffer[13],16);
				svara[16] = 0;
			}
		}
		else if(!strncmp(buffer,"HANGUPSTRING",12)) {
			if(buffer[13]=='-') plussa=FALSE;
			else {
				strncpy(hangup,&buffer[13],31);
				hangup[31] = 0;
			}
		}
		else if(!strncmp(buffer,"HIGHBAUDRATE",12)) highbaud=atoi(&buffer[13]);
		else if(!strncmp(buffer,"FIXED",5)) {
			if(buffer[6]=='y' || buffer[6]=='Y') hst=TRUE;
			else hst=FALSE;
		}
		else if(!strncmp(buffer,"DEVICE",6)) {
			strncpy(device,&buffer[7],30);
			device[30] = 0;
		}
		else if(!strncmp(buffer,"UNIT",4)) unit=atoi(&buffer[5]);
		else if(!strncmp(buffer,"ZINIT",5)) {
			strncpy(zinitstring,&buffer[6],50);
			zinitstring[50] = 0;
		}
		else if(!strncmp(buffer,"HANGUPDELAY",11)) hangupdelay=atoi(&buffer[12]);
		else if(!strncmp(buffer,"SCREEN",6)) {
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
		else if(!strncmp(buffer,"INACTIVETIME",12)) Servermem->maxinactivetime[nodnr]=atoi(&buffer[13]);
		else if(!strncmp(buffer,"HANDSKAKNING",12)) handskakning = atoi(&buffer[13]);
	}
	fclose(fp);
	if(getty) highbaud=gettybps;
}

void UpdateInactive(void) {
  nodestate &= ~NIKSTATE_INACTIVITY;
  AbortInactive();

  inactivereq->tr_node.io_Command = TR_ADDREQUEST;
  inactivereq->tr_node.io_Message.mn_ReplyPort = inactiveport;
  inactivereq->tr_time.tv_secs = Servermem->maxinactivetime[nodnr] * 60;
  inactivereq->tr_time.tv_micro = 0;
  SendIO((struct IORequest *)inactivereq);
}

void AbortInactive(void) {
  if(!CheckIO((struct IORequest *)inactivereq)) {
    AbortIO((struct IORequest *)inactivereq);
    WaitIO((struct IORequest *)inactivereq);
  }
}

#define FIFOEVENT_FROMUSER  1
#define FIFOEVENT_FROMFIFO  2
#define FIFOEVENT_CLOSEW    4
#define FIFOEVENT_NOCARRIER 8

int getfifoevent(struct MsgPort *fifoport, char *puthere) {
  struct IntuiMessage *mymess;
  struct NiKMess *nikmess;
  ULONG signals,
    conreadsig = 1L << conreadport->mp_SigBit,
    windsig = 1L << NiKwind->UserPort->mp_SigBit,
    serreadsig = 1L << serreadport->mp_SigBit,
    fifosig = 1L << fifoport->mp_SigBit,
    nikomnodesig = 1L << nikomnodeport->mp_SigBit;
  int event = 0;

  if(ImmediateLogout()) {
    return FIFOEVENT_NOCARRIER;
  }

  while(!event) {
    signals = Wait(conreadsig | windsig | serreadsig | fifosig
                   | nikomnodesig | SIGBREAKF_CTRL_C);
    if((signals & conreadsig) && CheckIO((struct IORequest *)conreadreq)) {
      *puthere = congettkn();
      event |= FIFOEVENT_FROMUSER;
    }
    if((signals & serreadsig) && CheckIO((struct IORequest *)serreadreq)) {
      *puthere = sergettkn();
      QueryCarrierDropped();
      event |= FIFOEVENT_FROMUSER;
    }
    if(signals & windsig) {
      mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
      ReplyMsg((struct Message *)mymess);
      AbortIO((struct IORequest *)conreadreq);
      WaitIO((struct IORequest *)conreadreq);
      AbortIO((struct IORequest *)serreadreq);
      WaitIO((struct IORequest *)serreadreq);
      event |= FIFOEVENT_CLOSEW;
    }
    if(signals & SIGBREAKF_CTRL_C) {
      AbortIO((struct IORequest *)conreadreq);
      WaitIO((struct IORequest *)conreadreq);
      AbortIO((struct IORequest *)serreadreq);
      WaitIO((struct IORequest *)serreadreq);
      event |= FIFOEVENT_CLOSEW;
    }
    if(signals & fifosig) {
      event |= FIFOEVENT_FROMFIFO;
    }
    if(signals & nikomnodesig) {
      while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport)) {
        handleservermess(nikmess);
        ReplyMsg((struct Message *)nikmess);
      }
    }
    if(ImmediateLogout()) {
      event |= FIFOEVENT_NOCARRIER;
    }
  }
  if(*puthere=='+') putstring(" \b",-1,0);
  return(event);
}

void abortserial(void) {
	AbortIO((struct IORequest *)serreadreq);
	WaitIO((struct IORequest *)serreadreq);
}

int checkcharbuffer(void)
{
	char tecken;
/*	ULONG conreadsig = 1L << conreadport->mp_SigBit;
	ULONG serreadsig = 1L << serreadport->mp_SigBit; */

	if(typeaheadbuftkn<50)
	{
		while(tknwaiting())
		{
			if(CheckIO((struct IORequest *)conreadreq))
			{
				tecken=congettkn();
				if(tecken && typeaheadbuftkn<50)
				{
					typeaheadbuf[typeaheadbuftkn++]=tecken;
					typeaheadbuf[typeaheadbuftkn]=0;
				}
			}

			if(CheckIO((struct IORequest *)serreadreq))
			{
				tecken=sergettkn();
				if(tecken && typeaheadbuftkn<50)
				{
					typeaheadbuf[typeaheadbuftkn++]=tecken;
					typeaheadbuf[typeaheadbuftkn]=0;
				}
			}
		}
	}

	return typeaheadbuftkn;
}

int puttekn(char *pekare,int size)
{
	int aborted=FALSE, x1, x2, consize, sersize, nyrad;
	char serpekare[1200],localconstring[1200];
	char serbuf[1200], conbuf[1200], *concurpek, *sercurpek, *constartpek, *serstartpek;
	char conready, serready, conlineready, serlineready;

	if(ConnectionLost()) {
          return TRUE;
        }

	if(size == -1 && pekare[0] == 0)
		return(0);

/* Följande kanske ser lite dubiöst ut men det är gjort så att man inte ska behöva
 * anropa noansi() mer än nödvändigt. Det går mycket snabbare att kopiera en redan
 * konverterad sträng än att konvertera den igen */

	strncpy(localconstring,pekare,1199);
	localconstring[1199]=0;
	if(Servermem->cfg.cfgflags & NICFG_LOCALCOLOURS) {
		strncpy(serpekare,pekare,1199);
		if(!(Servermem->inne[nodnr].flaggor & ANSICOLOURS)) StripAnsiSequences(serpekare);
	} else {
		StripAnsiSequences(localconstring);
		if(Servermem->inne[nodnr].flaggor & ANSICOLOURS) strncpy(serpekare,pekare,1199);
		else strncpy(serpekare,localconstring,1199);
	}
	serpekare[1199]=0;

	ConvChrsFromAmiga(serpekare,0,Servermem->inne[nodnr].chrset);

	concurpek = constartpek = &localconstring[0];
	sercurpek = serstartpek = &serpekare[0];
	consize = sersize = -1;

	x1 = 0;
	x2 = 0;
	conready = 0;
	serready = 0;

	while((x1 < consize || consize == -1) && (x2 < sersize || sersize == -1) && !aborted && (!conready || !serready))
	{
		conbuf[0] = serbuf[0] = 0;
		conlineready = 0;
		serlineready = 0;

		/* Start Console */
		while((x1 < consize || consize == -1) && !conready && !conlineready)
		{
			if(consize < 1 && concurpek[0] == 0)
			{
				if(constartpek[0] != 0 && concurpek != constartpek)
				{
					strcpy(conbuf, constartpek);
				}
				conready = 1;
			}
			else if(size > 0 && x1++ == size)
			{
				if(concurpek != constartpek)
				{
					strncpy(conbuf, constartpek, concurpek - constartpek);
				}
				conready = 1;
			}
			else if(concurpek[0] == '\n')
			{
				conbuf[0] = 0;
				if(concurpek != constartpek)
				{
					concurpek[0] = 0;
					if(constartpek[0] != 0)
						strcpy(conbuf, constartpek);

					strcat(conbuf, "\n");
					constartpek = concurpek + 1;
				}
				else
				{
					conbuf[0] = '\n';
					conbuf[1] = 0;
					constartpek = concurpek + 1;
				}

				concurpek = constartpek - 1;
				conlineready = 1;
			}

			concurpek++;
		}
		/* End Console */

		nyrad = 0;
		/* Start Serial */
		while((x2 < sersize || sersize == -1) && !serlineready && !serready)
		{
			if(sersize < 1 && sercurpek[0] == 0)
			{
				if(serstartpek[0] != 0 && sercurpek != serstartpek)
				{
					strcpy(serbuf, serstartpek);
				}
				serready = 1;
			}
			else if(sersize > 0 && x2++ == sersize)
			{
				if(sercurpek != serstartpek)
				{
					strncpy(serbuf, serstartpek, sercurpek - serstartpek);
				}
				serready = 1;
			}
			else if(sercurpek[0] == '\n')
			{
				serbuf[0] = 0;
				if(sercurpek != serstartpek)
				{
					sercurpek[0] = 0;
					if(serstartpek[0] != 0)
						strcpy(serbuf, serstartpek);

					strcat(serbuf, "\n");
					serstartpek = sercurpek + 1;
				}
				else
				{
					serbuf[0] = '\n';
					serbuf[1] = 0;
					serstartpek = sercurpek + 1;
				}

				nyrad = 1;
				sercurpek = serstartpek - 1;
				serlineready = 1;
			}

			sercurpek++;
		}
		/* Serial end */

		if(sendtosercon(conbuf, serbuf, -1, -1)) {
                  aborted = TRUE;
                }
		if(nyrad && incantrader()) {
                  aborted = TRUE;
                }
	}

	return(aborted || ConnectionLost());
}

int sendtosercon(char *conpek, char *serpek, int consize, int sersize) {
  struct IntuiMessage *mymess;
  struct NiKMess *nikmess;

  char tecken;
  int aborted = FALSE, paused=FALSE;
  int console=1,serial=1,err;
  ULONG signals,
    conwritesig = 1L << conwriteport->mp_SigBit,
    conreadsig = 1L << conreadport->mp_SigBit,
    windsig = 1L << NiKwind->UserPort->mp_SigBit,
    serwritesig = 1L << serwriteport->mp_SigBit,
    serreadsig = 1L << serreadport->mp_SigBit,
    nikomnodesig = 1L << nikomnodeport->mp_SigBit;

  if(CheckIO((struct IORequest *)serreadreq)) {
    AbortIO((struct IORequest *)serreadreq);
    WaitIO((struct IORequest *)serreadreq);
  }
  if(CheckIO((struct IORequest *)serwritereq)) {
    AbortIO((struct IORequest *)serwritereq);
    WaitIO((struct IORequest *)serwritereq);
  }

  if(serpek[0] != 0) {
    serwritereq->IOSer.io_Command=CMD_WRITE;
    serwritereq->IOSer.io_Data=(APTR)serpek;
    serwritereq->IOSer.io_Length=sersize;
    SendIO((struct IORequest *)serwritereq);
  } else {
    serial = 0;
  }

  if(conpek[0] != 0) {
    conwritereq->io_Command=CMD_WRITE;
    conwritereq->io_Data=(APTR)conpek;
    conwritereq->io_Length=consize;
    SendIO((struct IORequest *)conwritereq);
  } else {
    console = 0;
  }

  while(console || serial) {
    signals = Wait(conwritesig | conreadsig | windsig | serwritesig
                   | serreadsig | nikomnodesig);
    if(signals & conwritesig) {
      console = 0;
      if(WaitIO((struct IORequest *)conwritereq)) {
        printf("Error console\n");
      }
    }
    if((signals & serwritesig) && CheckIO((struct IORequest *)serwritereq)) {
      serial = 0;
      if(err=WaitIO((struct IORequest *)serwritereq)) {
        writesererr(err);
      }
    }
    if((signals & conreadsig) && CheckIO((struct IORequest *)conreadreq)) {
      if((tecken = congettkn()) == 3) {
        if(serial) {
          AbortIO((struct IORequest *)serwritereq);
          WaitIO((struct IORequest *)serwritereq);
        }
        if(console) {
          AbortIO((struct IORequest *)conwritereq);
          WaitIO((struct IORequest *)conwritereq);
        }
        aborted = TRUE;
        console = serial = 0;
        putstring("^C\n\r",-1,0);
      } else if((tecken==' ' && (Servermem->inne[nodnr].flaggor & MELLANSLAG))
                || tecken==19) {
        paused=TRUE;
      } else if(tecken && typeaheadbuftkn < 50) {
        typeaheadbuf[typeaheadbuftkn++]=tecken;
        typeaheadbuf[typeaheadbuftkn]=0;
      }
      UpdateInactive();
    }
    if((signals & serreadsig) && CheckIO((struct IORequest *)serreadreq)) {
      if((tecken = sergettkn()) == 3) {
        if(serial) {
          AbortIO((struct IORequest *)serwritereq);
          WaitIO((struct IORequest *)serwritereq);
        }
        if(console) {
          AbortIO((struct IORequest *)conwritereq);
          WaitIO((struct IORequest *)conwritereq);
        }
        aborted = TRUE;
        console = serial = 0;
        putstring("^C\n\r",-1,0);
      } else if((tecken==' ' && (Servermem->inne[nodnr].flaggor & MELLANSLAG))
                || tecken==19) {
        paused=TRUE;
      } else if(tecken && typeaheadbuftkn<50) {
        typeaheadbuf[typeaheadbuftkn++]=tecken;
        typeaheadbuf[typeaheadbuftkn]=0;
      }
      UpdateInactive();
      QueryCarrierDropped();
    }
    if(signals & windsig) {
      mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
      ReplyMsg((struct Message *)mymess);
      AbortIO((struct IORequest *)conreadreq);
      WaitIO((struct IORequest *)conreadreq);
      AbortIO((struct IORequest *)serreadreq);
      WaitIO((struct IORequest *)serreadreq);
      AbortIO((struct IORequest *)inactivereq);
      WaitIO((struct IORequest *)inactivereq);
      if(!CheckIO((struct IORequest *)conwritereq)) {
        AbortIO((struct IORequest *)conwritereq);
        WaitIO((struct IORequest *)conwritereq);
      }
      if(!CheckIO((struct IORequest *)serwritereq)) {
        AbortIO((struct IORequest *)serwritereq);
        WaitIO((struct IORequest *)serwritereq);
      }
      cleanup(OK,"");
    }
    if(signals & nikomnodesig) {
      while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport)) {
        handleservermess(nikmess);
        ReplyMsg((struct Message *)nikmess);
      }
    }
  }

  if(paused && gettekn()==3) {
    putstring("^C\n\r",-1,0);
    aborted = TRUE;
  }

  return(aborted);
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
		}
	}
	if(paused && gettekn()==3)
	{
		putstring("^C\n\r",-1,0);
		return(TRUE);
	}

	return(aborted);
}

int serputtekn(char *pekare,int size)
{
	int aborted=FALSE,x;
	char *tmppek, buffer[1200], serstring[1201];

	strncpy(serstring,pekare,1199);
	serstring[1199]=0;
	pekare = &serstring[0];

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
				if(sendtoser(pekare, size)) aborted = TRUE;
			}
			break;
		}
		if(size > 0 && x++ == size)
		{
			if(pekare != tmppek)
			{
				tmppek[1] = 0;
				if(sendtoser(pekare, -1)) aborted = TRUE;
			}
			break;
		}
		if(tmppek[0] == '\n')
		{
			buffer[0] = 0;
			if(tmppek != pekare)
			{
				tmppek[0] = 0;
				if(pekare[0] != NULL)
					strcpy(buffer, pekare);

				strcat(buffer, "\n");
				pekare = ++tmppek;
			}
			else
			{
				buffer[0] = '\n';
				buffer[1] = NULL;
				pekare = ++tmppek;
			}

			if(sendtoser(&buffer[0], -1)) aborted = TRUE;
			tmppek = pekare - 1;
		}
		tmppek++;
	}

	return(aborted || ConnectionLost());
}

int sendtoser(char *pekare, int size) {
  struct IntuiMessage *mymess;
  struct NiKMess *nikmess;
  int aborted = FALSE, paused=FALSE;
  ULONG signals,
    serwritesig = 1L << serwriteport->mp_SigBit,
    serreadsig = 1L << serreadport->mp_SigBit,
    windsig = 1L << NiKwind->UserPort->mp_SigBit,
    nikomnodesig = 1L << nikomnodeport->mp_SigBit;
  char serial = 1, tecken;

  serwritereq->IOSer.io_Command=CMD_WRITE;
  serwritereq->IOSer.io_Data=(APTR)pekare;
  serwritereq->IOSer.io_Length=size;
  SendIO((struct IORequest *)serwritereq);
  while(serial) {
    signals = Wait(serwritesig | serreadsig | windsig | nikomnodesig);
    if(signals & serwritesig) {
      serial = 0;
      if(WaitIO((struct IORequest *)serwritereq)) {
        printf("Error serial\n");
      }
    }
    if(signals & serreadsig) {
      if((tecken = sergettkn()) == 3) {
        if(serial) {
          AbortIO((struct IORequest *)serwritereq);
          WaitIO((struct IORequest *)serwritereq);
        }
        aborted = TRUE;
        serial = 0;
        serputstring("^C\n\r",-1,0);
      } else if((tecken == ' ' && (Servermem->inne[nodnr].flaggor & MELLANSLAG))
                || tecken == 19) {
        paused = TRUE;
      } else if(tecken && typeaheadbuftkn<50) {
        typeaheadbuf[typeaheadbuftkn++] = tecken;
        typeaheadbuf[typeaheadbuftkn] = 0;
      }
      UpdateInactive();
      QueryCarrierDropped();
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
    }
  }
  if(paused && sergettkn()==3) {
    serputstring("^C\n\r",-1,0);
    return(TRUE);
  }
  return(aborted);
}
