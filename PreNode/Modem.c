#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/serial.h>
#include <devices/console.h>
#include <devices/conunit.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NiKomstr.h"
#include "NiKomLib.h"
#include "PreNodeFuncs.h"
#include "Logging.h"
#include "Terminal.h"

#define ERROR	10
#define OK	0
#define RELOGIN 1
#define EKO	1
#define EJEKO	0
#define NOCARRIER	32

extern struct IOExtSer *serwritereq;
extern struct MsgPort *serwriteport;
extern struct IOExtSer *serreadreq;
extern struct MsgPort *serreadport;
extern struct IOExtSer *serchangereq;
extern struct MsgPort *serchangeport;
extern struct timerequest *timerreq;
extern struct MsgPort *timerport;
extern struct MsgPort *nikomnodeport;
extern struct Window *NiKwind;

extern int nodnr, dtespeed, nodestate, handskakning;
extern struct System *Servermem;
extern char outbuffer[];


char svara[17],init[81],hangup[32];
int plussa,autoanswer,highbaud,hst,hangupdelay;

void modemcmd(char *pekare,int size) {
	int err;
	serwritereq->IOSer.io_Command=CMD_WRITE;
	serwritereq->IOSer.io_Data=(APTR)pekare;
	serwritereq->IOSer.io_Length=size;
	if(err=DoIO((struct IORequest *)serwritereq)) writesererr(err);
}

void disconnect() {
	if(!plussa || carrierdropped()) {
		CloseSerial(serwritereq);
		Delay(100);
		dtespeed = highbaud;
		if(OpenSerial(serwritereq,serreadreq,serchangereq))
			cleanup(ERROR,"Couldn't open user defined device\n");
	} else {
		sendplus();
		Delay(50);
		sendat(hangup);
	}
}

void sendat(char *atstring) {
	int count=2,going=TRUE, err, i=0;
	char tkn;
	long signals,timersig=1L << timerport->mp_SigBit,sersig=1L << serreadport->mp_SigBit;
	serchangereq->IOSer.io_Command = SDCMD_QUERY;
	if(err=DoIO((struct IORequest *)serchangereq)) writesererr(err);
	conputtekn("\nSerial diagnosis\n----------------\n",-1);
	sprintf(outbuffer,"Data Set Ready:      %sactive\n",(serchangereq->io_Status & (1 << 3)) ? "not " : "");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Clear To Send:       %sactive\n",(serchangereq->io_Status & (1 << 4)) ? "not " : "");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Carrier Detect:      %sactive\n",(serchangereq->io_Status & (1 << 5)) ? "not " : "");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Ready To Send:       %sactive\n",(serchangereq->io_Status & (1 << 6)) ? "not " : "");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Data Terminal Ready: %sactive\n",(serchangereq->io_Status & (1 << 7)) ? "not " : "");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Hardware Overrun:    %s\n",(serchangereq->io_Status & (1 << 8)) ? "yes" : "no");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Break Sent:          %s\n",(serchangereq->io_Status & (1 << 9)) ? "yes" : "no");
	conputtekn(outbuffer,-1);
	sprintf(outbuffer,"Break Recieved:      %s\n",(serchangereq->io_Status & (1 << 10)) ? "yes" : "no");
	conputtekn(outbuffer,-1);

	while(going) {
		while(tknwaiting()) {
			gettekn();
			i++;
		}
		sprintf(outbuffer,"\nTog bort %d tecken",i);
		conputtekn(outbuffer,-1);
		conputtekn("\nInitiating modem\n",-1);
		putstring(atstring,-1,0);
		eka('\r');
		timerreq->tr_node.io_Command=TR_ADDREQUEST;
		timerreq->tr_node.io_Message.mn_ReplyPort=timerport;
		timerreq->tr_time.tv_secs=5;
		timerreq->tr_time.tv_micro=0;
		SendIO((struct IORequest *)timerreq);
		count = 2;
		while(count) {
			signals=Wait(timersig | sersig);
			if((signals & sersig) && CheckIO((struct IORequest *)serreadreq)) {
				tkn=sergettkn();
				coneka(tkn);
				if(tkn == '\n') count--;
/*				if(tkn == '\n') {
 *					count--;
 *					if(count == 1) {
 *						replybuf[0] = 0;
 *						bufcnt = 0;
 *					}
 *				} else {
 *					replybuf[bufcnt++] = tkn;
 *					replybuf[bufcnt] = 0;
 *				} */
			}
			if((signals & timersig) && CheckIO((struct IORequest *)timerreq)) {
				WaitIO((struct IORequest *)timerreq);
				break;
			}
		}
		if(!count) {
			going = FALSE;
			conputtekn("\nReady.\n",-1);

/*			if(!strcmp(replybuf,"OK\r")) {
 *				conputtekn("\nModem responded ok\n",-1);
 *				going=FALSE;
 *			} else {
 *				conputtekn("\nNo correct response from modem, trying again\n",-1);
 *				Delay(50);
 *			} */

		} else conputtekn("\nTimeout, trying again.\n",-1);
	}
	if(!CheckIO((struct IORequest *)timerreq)) {
		AbortIO((struct IORequest *)timerreq);
		WaitIO((struct IORequest *)timerreq);
	}
}

void sendplus(void) {
	int count=2,going=TRUE;
	char tkn;
	long signals,timersig=1L << timerport->mp_SigBit,sersig=1L << serreadport->mp_SigBit;
	Delay(100);
	while(going) {
		eka('+');
		eka('+');
		eka('+');
		timerreq->tr_node.io_Command=TR_ADDREQUEST;
		timerreq->tr_node.io_Message.mn_ReplyPort=timerport;
		timerreq->tr_time.tv_secs=5;
		timerreq->tr_time.tv_micro=0;
		SendIO((struct IORequest *)timerreq);
		while(count) {
			signals=Wait(timersig | sersig);
			if((signals & sersig) && CheckIO((struct IORequest *)serreadreq)) {
				if((tkn=sergettkn())==10) count--;
				coneka(tkn);
			}
			if((signals & timersig) && CheckIO((struct IORequest *)timerreq)) {
				WaitIO((struct IORequest *)timerreq);
				if(carrierdropped()) return;
				break;
			}
		}
		if(!count) going=FALSE;
	}
	if(!CheckIO((struct IORequest *)timerreq)) {
		AbortIO((struct IORequest *)timerreq);
		WaitIO((struct IORequest *)timerreq);
	}
}

char wc_gettkn(int seropen) {
	struct IntuiMessage *mymess;
	struct NiKMess *nikmess;
	ULONG signals, windsig=1L << NiKwind->UserPort->mp_SigBit, serreadsig=1L << serreadport->mp_SigBit,
		nikomnodesig = 1L << nikomnodeport->mp_SigBit;
	char tkn=0;
	if(seropen) signals = Wait(windsig | serreadsig | nikomnodesig | SIGBREAKF_CTRL_C);
	else signals = Wait(windsig | nikomnodesig | SIGBREAKF_CTRL_C);

	if((signals & serreadsig) && CheckIO((struct IORequest *)serreadreq)) {
		tkn=sergettkn();
	}
	if((signals & windsig) || (signals & SIGBREAKF_CTRL_C)) {
		if(signals & windsig) {
			mymess=(struct IntuiMessage *)GetMsg(NiKwind->UserPort);
			ReplyMsg((struct Message *)mymess);
		}
		if(seropen) {
			AbortIO((struct IORequest *)serreadreq);
			WaitIO((struct IORequest *)serreadreq);
		}
		cleanup(OK,"");
	}
	if(signals & nikomnodesig) {
		while(nikmess = (struct NiKMess *) GetMsg(nikomnodeport)) {
conputtekn("%%% Har fått ett meddelande %%%\n",-1);
			handleservermess(nikmess);
			ReplyMsg((struct Message *)nikmess);
		}
		tkn = 0;
	}
	return(tkn);
}


void waitconnect(void) {
	int bufcnt,len,err, oldstate;
	UBYTE buf[50],temp, svarabuf[20];
	char CallerIDHost[81], CallerIDHostIP[81];
	coneka(15);
	Delay(25);
	nodestate = nodestate & (NIKSTATE_CLOSESER | NIKSTATE_NOANSWER);
	if(nodestate & NIKSTATE_CLOSESER) CloseSerial(serwritereq);
	else sendat(init);
	sprintf(outbuffer,"Current state: Serial %s, calls are %saccepted\n",
		(nodestate & NIKSTATE_CLOSESER) ? "closed" : "open",
		(nodestate & NIKSTATE_NOANSWER) ? "not " : "");
	conputtekn(outbuffer,-1);
	CallerIDHost[0] = CallerIDHostIP[0] = NULL;

	for(;;) {
		memset(buf,0,50);
		bufcnt=0;
		oldstate = nodestate;
		for(;;) {
			temp = wc_gettkn(!(nodestate & NIKSTATE_CLOSESER));
			if(temp == 0 || temp == 10) break;
			coneka(temp);
			buf[bufcnt++] = temp;
			if(bufcnt >= 50) break;
		}
		if(temp == 0) {
			nodestate = nodestate & (NIKSTATE_CLOSESER | NIKSTATE_NOANSWER);
			if((oldstate & NIKSTATE_CLOSESER) && !(nodestate & NIKSTATE_CLOSESER)) {
				if(OpenSerial(serwritereq,serreadreq,serchangereq))
					cleanup(ERROR,"Couldn't open user defined device\n");
				serreqtkn();
				sendat(init);
			} else if(!(oldstate & NIKSTATE_CLOSESER) && (nodestate & NIKSTATE_CLOSESER))
				CloseSerial(serwritereq);
			sprintf(outbuffer,"Current state: Serial %s, calls are %saccepted\n",
				(nodestate & NIKSTATE_CLOSESER) ? "closed" : "open",
				(nodestate & NIKSTATE_NOANSWER) ? "not " : "");
			conputtekn(outbuffer,-1);
		} else {
			coneka((char)10);
			if(!strncmp(buf,"RING",4) && !autoanswer && !(nodestate & NIKSTATE_NOANSWER))
			{
				sprintf(svarabuf,"%s\r",svara);
				modemcmd(svarabuf,-1);
				Servermem->inloggad[nodnr] = -2;
			}
			else if(!strncmp(buf,"  HOST NAME",11))
			{
				buf[strlen(buf)-1] = 0;
				strncpy(CallerIDHost, &buf[17], 80);
				CallerIDHost[80] = 0;
			}
			else if(!strncmp(buf,"  HOST IP ADDR",14))
			{
				buf[strlen(buf)-1] = 0;
				strncpy(CallerIDHostIP, &buf[17], 80);
				CallerIDHostIP[80] = 0;
			}
			else if(!strncmp(buf,"CONNECT",7) && !(nodestate & NIKSTATE_NOANSWER)) {
				if(buf[7] == '\n') Servermem->connectbps[nodnr] = 300;
				else Servermem->connectbps[nodnr] = atoi(hittaefter(buf));
				if(strcmp(CallerIDHost,"???") || CallerIDHostIP[0])
				{
					if(strcmp(CallerIDHost,"???"))
						strcpy(Servermem->CallerID[nodnr], CallerIDHost);
					else
						strcpy(Servermem->CallerID[nodnr], CallerIDHostIP);
				}

				if(Servermem->cfg.logmask & LOG_CONNECTION) {
                                  len=strlen(buf);
                                  if(buf[len-1]=='\r') buf[len-1]=0;
                                  if(Servermem->CallerID[nodnr]) {
                                    LogEvent(USAGE_LOG, INFO, "%s (nod %d) CallerID %s",
                                             buf, nodnr, Servermem->CallerID[nodnr]);
                                  } else {
                                    LogEvent(USAGE_LOG, INFO, "%s (nod %d)",buf,nodnr);
                                  }
				}
				AbortIO((struct IORequest *)serreadreq);
				WaitIO((struct IORequest *)serreadreq);
				serchangereq->IOSer.io_Command=CMD_CLEAR;
				if(err=DoIO((struct IORequest *)serchangereq)) writesererr(err);
				serchangereq->IOSer.io_Command=CMD_FLUSH;
				if(err=DoIO((struct IORequest *)serchangereq)) writesererr(err);
				if(!hst) {
					if(!(strncmp(&buf[8],"2400",4))) serchangereq->io_Baud=2400;
					else if(!(strncmp(&buf[8],"1200",4))) serchangereq->io_Baud=1200;
					else if(!(strncmp(&buf[8],"600",3))) serchangereq->io_Baud=600;
					else if(!(strncmp(&buf[8],"300",3))) serchangereq->io_Baud=300;
					else if(!(strncmp(&buf[8],"4800",4))) serchangereq->io_Baud=4800;
					else if(!(strncmp(&buf[8],"7200",4))) serchangereq->io_Baud=7200;
					else if(!(strncmp(&buf[8],"9600",4))) serchangereq->io_Baud=9600;
					else if(!(strncmp(&buf[8],"12000",4))) serchangereq->io_Baud=12000;
					else if(!(strncmp(&buf[8],"14400",5))) serchangereq->io_Baud=14400;
					else if(!(strncmp(&buf[8],"19200",4))) serchangereq->io_Baud=19200;
					else if(!(strncmp(&buf[8],"38400",4))) serchangereq->io_Baud=38400;
					else if(!(strncmp(&buf[8],"57600",4))) serchangereq->io_Baud=57600;
					else if(!(strncmp(&buf[8],"115200",4))) serchangereq->io_Baud=115200;
					else serchangereq->io_Baud=300;
					serchangereq->IOSer.io_Command=SDCMD_SETPARAMS;
					serchangereq->io_RBufLen=16384;
					serchangereq->io_ReadLen=8;
					serchangereq->io_WriteLen=8;
					serchangereq->io_StopBits=1;
					serchangereq->io_SerFlags = SERF_XDISABLED | SERF_7WIRE | SERF_SHARED;
					if(handskakning == 2)
						serchangereq->io_SerFlags= SERF_XDISABLED | SERF_7WIRE | SERF_SHARED;
					else
					{
						if(handskakning == 1)
							serchangereq->io_SerFlags= SERF_SHARED;
						else
							serchangereq->io_SerFlags= SERF_XDISABLED | SERF_SHARED;
					}

					if(err=DoIO((struct IORequest *)serchangereq)) writesererr(err);
					dtespeed = serchangereq->io_Baud;
				}
				serreqtkn();
				sprintf(outbuffer,"Connect-speed: %d, DTE-speed: %d\n\n",Servermem->connectbps[nodnr],dtespeed);
				conputtekn(outbuffer,-1);
				nodestate = nodestate & (NIKSTATE_CLOSESER | NIKSTATE_NOANSWER);
				return;
			} else if(!strncmp(buf,"NO CARRIER",10)) {
				if(Servermem->cfg.logmask & LOG_NOCONNECT) {
                                  LogEvent(USAGE_LOG, INFO, "RING på nod %d, men ingen CONNECT", nodnr);
				}
				Servermem->inloggad[nodnr] = -1;
				sendat(init);
			}
		}
	}
}

