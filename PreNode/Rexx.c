#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "NiKomStr.h"
#include "PreNodeFuncs.h"
#include "Terminal.h"
#include "RexxUtils.h"
#include "BasicIO.h"

#define EKO		1
#define EJEKO	0

extern struct System *Servermem;
extern int nodnr,inloggad,rxlinecount;
extern char outbuffer[],inmat[],typeaheadbuf[];
extern struct MsgPort *rexxport;

int sendrexxrc;

int sendrexx(int komnr) {
	char macronamn[200],*rexxargs1;
	int going=TRUE;
	struct RexxMsg *nikrexxmess,*tempmess;
	struct MsgPort *rexxmastport;
	sendrexxrc=-5;
	sprintf(macronamn,"NiKom:rexx/ExtKom%d %d %d",komnr,nodnr,inloggad);
	if(!(nikrexxmess=(struct RexxMsg *)CreateRexxMsg(rexxport,"nik",rexxport->mp_Node.ln_Name))) {
		puttekn("\r\n\nKunde inte allokera ett RexxMsg!\r\n\n",-1);
		return(-5);
	}
	if(!(nikrexxmess->rm_Args[0]=(STRPTR)CreateArgstring(macronamn,strlen(macronamn)))) {
		DeleteRexxMsg(nikrexxmess);
		puttekn("\r\n\nKunde inte allokera en ArgString1\r\n\n",-1);
		return(-5);
	}
	nikrexxmess->rm_Action=RXCOMM | RXFB_TOKEN;
	Forbid();
	if(!(rexxmastport=(struct MsgPort *)FindPort("REXX"))) {
		Permit();
		puttekn("\r\n\nRexxMast är inte igång!\r\n\n",-1);
		return(-5);
	}
	PutMsg(rexxmastport,(struct Message *)nikrexxmess);
	Permit();
	while(going) {
		WaitPort(rexxport);
		tempmess=(struct RexxMsg *)GetMsg(rexxport);
		if(tempmess->rm_Node.mn_Node.ln_Type==NT_REPLYMSG) {
			DeleteArgstring(nikrexxmess->rm_Args[0]);
			if(nikrexxmess->rm_Result1) {
				sprintf(outbuffer,"\r\n\nRexx: Return-code %d\r\n\n",nikrexxmess->rm_Result1);
				puttekn(outbuffer,-1);
			}
			DeleteRexxMsg(nikrexxmess);
			rxlinecount=TRUE;
			return(sendrexxrc);
		}
		if(!strnicmp(tempmess->rm_Args[0],"sendstring",10)) rexxsendstring(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"getstring",9)) rexxgetstring(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"sendtextfile",12)) sendfile(rexxargs1=hittaefter(tempmess->rm_Args[0]));
		else if(!strnicmp(tempmess->rm_Args[0],"getchar",7)) rexxgettekn(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"chkbuffer",9)) rexxchkbuffer(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"yesno",5)) rexxyesno(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"setlinecount",12)) rxsetlinecount(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"sendchar",8)) rxsendchar(tempmess);
		else if(!strnicmp(tempmess->rm_Args[0],"sendrawfile",8)) rxsendrawfile(tempmess);
		else {
			sprintf(outbuffer,"\r\n\nKan inte hantera: %s\r\n",tempmess->rm_Args[0]);
			puttekn(outbuffer,-1);
			tempmess->rm_Result1=5;
			tempmess->rm_Result2=NULL;
		}
		ReplyMsg((struct Message *)tempmess);
	}
	if(ImmediateLogout()) return(-8);
	return(sendrexxrc);
}

void rxsendrawfile(struct RexxMsg *mess) {
	BPTR fh;
	int antal;
	char retstr[2] = "x", *filnamn = hittaefter(mess->rm_Args[0]);
	if(!(fh = Open(filnamn,MODE_OLDFILE))) {
		sprintf(outbuffer,"\n\n\rKan inte öppna filen %s\n\r",filnamn);
		puttekn(outbuffer,-1);
		retstr[0]='0';
	} else {
		while(antal = Read(fh,outbuffer,99)) {
			if(antal == -1) break;
			outbuffer[antal] = 0;
			putstring(outbuffer,-1,0);
		}
		Close(fh);
		retstr[0] = '1';
	}
	if(mess->rm_Action & 1L<<RXFB_RESULT) {
		if(!(mess->rm_Result2=(long)CreateArgstring(retstr,1)))
			puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
	}
	mess->rm_Result1=0;
}

void rexxsendstring(struct RexxMsg *mess) {
	char *str,ret,retstr[2];
	str=hittaefter(mess->rm_Args[0]);
	ret=puttekn(str,-1);
	if(ret) retstr[0]='1';
	else retstr[0]='0';
	retstr[1]=0;
	mess->rm_Result1=0;
	if(mess->rm_Action & 1L<<RXFB_RESULT) {
		if(!(mess->rm_Result2=(long)CreateArgstring(retstr,1)))
			puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
	}
}

void rexxgetstring(struct RexxMsg *tempmess) {
	char *rexxargs1,*rexxargs2;
	char defaultstring[257];
	int ra1=0,ra2=0;

	defaultstring[0] = NULL;
	rexxargs1=hittaefter(tempmess->rm_Args[0]);
	if(rexxargs1[0]) {
		if(!(ra1=atoi(rexxargs1))) ra1=50;
		rexxargs2=hittaefter(rexxargs1);
		if(rexxargs2[0] && !strncmp(rexxargs2,"NOECHO",6)) ra2=EJEKO;
		else
		{
			ra2=EKO;
			if(rexxargs2[0]) strcpy(defaultstring,rexxargs2);
		}
	} else {
		ra1=50;
		ra2=EKO;
	}
	if(defaultstring[0] != NULL)
	{
		if(getstring(ra2,ra1,defaultstring))
		{
			tempmess->rm_Result1=100;
			tempmess->rm_Result2=NULL;
			return;
		}
	}
	else
	{
		if(getstring(ra2,ra1,NULL)) {
			tempmess->rm_Result1=100;
			tempmess->rm_Result2=NULL;
			return;
		}
	}
	tempmess->rm_Result1=0;
	if(tempmess->rm_Action & 1L<<RXFB_RESULT) {
		if(!(tempmess->rm_Result2=(long)CreateArgstring(inmat,strlen(inmat))))
			puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
	}
}

void rexxgettekn(struct RexxMsg *mess) {
	UBYTE foo[2];
	foo[0]=gettekn();
	foo[1]=0;
	if(ImmediateLogout()) {
		mess->rm_Result1=100;
		mess->rm_Result2=NULL;
	} else {
		mess->rm_Result1=0;
		if(mess->rm_Action & 1L<<RXFB_RESULT) {
			if(!(mess->rm_Result2=(long)CreateArgstring(foo,1)))
				puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
		}
	}
}

void rexxchkbuffer(struct RexxMsg *mess) {
	char foo[5];
	sprintf(foo,"%d",strlen(typeaheadbuf));
	mess->rm_Result1=0;
	if(mess->rm_Action & 1L<<RXFB_RESULT) {
		if(!(mess->rm_Result2=(long)CreateArgstring(foo,strlen(foo))))
			puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
	}
}

void rexxyesno(struct RexxMsg *mess) {
  int isYes;
  char ja,nej,def,*pek;
  pek=hittaefter(mess->rm_Args[0]);
  if(!pek[0]) {
    ja='j'; nej='n'; def=1;
  } else {
    ja=pek[0];
    pek=hittaefter(pek);
    if(!pek[0]) {
      nej='n'; def=1;
    } else {
      nej=pek[0];
      pek=hittaefter(pek);
      if(!pek[0]) def=1;
      else def=pek[0]-'0';
    }
  }

  if(GetYesOrNo(NULL, ja, nej, NULL, NULL, def == 1, &isYes)) {
    SetRexxErrorResult(mess, 100);
    return;
  }
  SetRexxResultString(mess, isYes ? "1" : "0");
}

void rxsetlinecount(struct RexxMsg *mess) {
	char *arg=hittaefter(mess->rm_Args[0]);
	if(!stricmp(arg,"ON")) rxlinecount=TRUE;
	else if(!stricmp(arg,"OFF")) rxlinecount=FALSE;
	else {
		mess->rm_Result1=10;
		mess->rm_Result2=NULL;
		return;
	}
	mess->rm_Result1=0;
	mess->rm_Result2=NULL;
}

void rxsendchar(struct RexxMsg *mess) {
	char *arg;
	arg=hittaefter(mess->rm_Args[0]);
	if(arg[0]!='/' || arg[1]==0) {
		mess->rm_Result1=10;
		mess->rm_Result2=NULL;
		return;
	}
	eka(arg[1]);
	mess->rm_Result1=0;
	mess->rm_Result2=NULL;
}
