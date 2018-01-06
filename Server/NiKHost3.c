#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include "NiKomStr.h"
#include "ServerFuncs.h"
#include "NiKomLib.h"
#include "nikom_protos.h"
#include "nikom_pragmas.h"
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "UserDataUtils.h"

#include "RexxUtils.h"

extern struct System *Servermem;

void rxsendnodemess(struct RexxMsg *mess) {
	int avs, nodnr, ret;
	char *str, retstr[5];
	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	nodnr = atoi(mess->rm_Args[1]);
	if(nodnr != -1 && (nodnr < 0 || nodnr >= MAXNOD) && Servermem->nodtyp[nodnr] == 0) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	avs = atoi(mess->rm_Args[2]);
	if(avs != -1 && !userexists(avs)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	str = mess->rm_Args[3];
	if(strlen(str) > MAXSAYTKN-1) str[MAXSAYTKN-1] = 0;
	ret = SendNodeMessage(nodnr, avs, str);
	sprintf(retstr, "%d",ret - 1);
	mess->rm_Result1=0;
	if(!(mess->rm_Result2=(long)CreateArgstring(retstr,strlen(retstr))))
		printf("Kunde inte allokera en ArgString\n");
}

void rexxstatusinfo(struct RexxMsg *mess)
{
	int status;
	char str[100];
	if(!mess->rm_Args[1] || !mess->rm_Args[2]) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}

	status = atoi(mess->rm_Args[1]);
	if(status < 0 || status > 100)
	{
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}

	switch(mess->rm_Args[2][0])
	{
		case 'r' : case 'R' :
			sprintf(str,"%d",Servermem->cfg->uldlratio[status]);
			break;

		case 't' : case 'T' :
			sprintf(str,"%d",Servermem->cfg->maxtid[status]);
			break;
		default :
			str[0] = 0;
	}

	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

/*

SystemInfo(status, subjekt)

Returnerar:

-2	Finns inget sådant subjekt.

*/

void rexxsysteminfo(struct RexxMsg *mess)
{
	char str[101];
	if(!mess->rm_Args[1])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}

	switch(mess->rm_Args[1][0])
	{
		case 'f' : case 'F' :
			sprintf(str,"%ld",Servermem->cfg->defaultflags);
			break;

		case 'd' : case 'D' :
			sprintf(str,"%ld",Servermem->cfg->diskfree);
			break;

		case 'm' : case 'M' :
			sprintf(str,"%ld",Servermem->cfg->logmask);
			break;

		case 'l' : case 'L' :
			sprintf(str,"%d",Servermem->cfg->logintries);
			break;

		case 'r' : case 'R' :
			sprintf(str,"%d",(int) Servermem->cfg->defaultrader);
			break;

		case 's' : case 'S' :
			sprintf(str,"%d",(int) Servermem->cfg->defaultstatus);
			break;

		case 'n' : case 'N' :
			strcpy(str,Servermem->cfg->ny);
			break;

		case 'u' : case 'U' :
			strcpy(str,Servermem->cfg->ultmp);
			break;

		default:
			strcpy(str,"-2");
			break;
	}

	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

/*
Arearight(användarnummer, areanr)

Returnerar:
0	Användaren har inga rättigheter i den aktuella arean.
1	Användaren har rättigheter i den aktuella arean.
-1	Om användaren inte finns.
-2	Arean är raderad
-3	Arean finns inte.
*/

void rexxarearight(struct RexxMsg *mess) {
  int userId, areanr;
  struct User *user;
  
  if(!mess->rm_Args[1] || !mess->rm_Args[2]) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  
  userId = atoi(mess->rm_Args[1]);
  areanr = atoi(mess->rm_Args[2]);
  
  if(areanr < 0 || areanr >= Servermem->info.areor) {
    SetRexxResultString(mess, "-3");
    return;
  }
  
  if(!Servermem->areor[areanr].namn[0]) {
    SetRexxResultString(mess, "-2");
    return;
  }
  
  if(!userexists(userId)) {
    SetRexxResultString(mess, "-1");
    return;
  }

  if(!(user = GetUserData(userId))) {
    SetRexxResultString(mess, "-1");
    return;
  }
  
  SetRexxResultString(mess, arearatt(areanr, userId, user) ? "1" : "0");
}

int arearatt(int area, int usrnr, struct User *usr) {
	int x=0;
	if(usr->status>=Servermem->cfg->st.bytarea) return(1);
	if(Servermem->areor[area].mote==-1 || MayBeMemberConf(Servermem->areor[area].mote, usrnr, usr)) x++;
	if(!Servermem->areor[area].grupper || (Servermem->areor[area].grupper & usr->grupper)) x++;
	return(x==2 ? 1 : 0);
}

void rexxmarktextread(struct RexxMsg *mess) {
  rexxmarktext(mess, 0);
}

void rexxmarktextunread(struct RexxMsg *mess) {
  rexxmarktext(mess, 1);
}
  
void rexxmarktext(struct RexxMsg *mess, int desiredUnreadStatus) {
  int textNr, userId, i, needToSave = FALSE;
  struct UnreadTexts *unreadTexts = NULL;
  static struct UnreadTexts unreadTextsBuf;

  if(!mess->rm_Args[1] || !mess->rm_Args[2]) {
    SetRexxErrorResult(mess, 1);
    return;
  }

  userId = atoi(mess->rm_Args[1]);
  textNr = atoi(mess->rm_Args[2]);

  if(!userexists(userId)) {
    SetRexxResultString(mess, "-1");
    return;
  }

  if(textNr < Servermem->info.lowtext || textNr > Servermem->info.hightext) {
    SetRexxResultString(mess, "-2");
    return;
  }
  
  if(GetConferenceForText(textNr) == -1) {
    SetRexxResultString(mess, "-5");
    return;
  }
    

  for(i=0; i < MAXNOD; i++) {
    if(Servermem->inloggad[i] == userId) {
      unreadTexts = &Servermem->unreadTexts[i];
      break;
    }
  }
  if(unreadTexts == NULL) {
    unreadTexts = &unreadTextsBuf;
    if(!ReadUnreadTexts(unreadTexts, userId)) {
      SetRexxResultString(mess, "-4");
      return;
    }
    needToSave = TRUE;
  }

  if(desiredUnreadStatus == (IsTextUnread(textNr, unreadTexts) ? 1 : 0)) {
    SetRexxResultString(mess, "-3");
    return;
  }

  ChangeUnreadTextStatus(textNr, desiredUnreadStatus, unreadTexts);
  if(needToSave) {
    WriteUnreadTexts(unreadTexts, userId);
  }

  SetRexxResultString(mess, "0");
}

void rexxconsoletext(struct RexxMsg *mess)
{
	int nodnr, varde;

	if(!mess->rm_Args[1] || !mess->rm_Args[2])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}

	nodnr = atoi(mess->rm_Args[1]);
	varde = atoi(mess->rm_Args[2]);

	Servermem->watchserial[nodnr] = varde;

	if(!(mess->rm_Result2=(long)CreateArgstring("0", 1)))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void rexxcheckuserpassword(struct RexxMsg *mess) {
  int userId;
  struct User *user;

  if(!mess->rm_Args[1] || !mess->rm_Args[2]) {
    SetRexxErrorResult(mess, 1);
    return;
  }

  userId = atoi(mess->rm_Args[1]);
  if(!(user = GetUserData(userId))) {
    SetRexxResultString(mess, "-1");
    return;
  }

  SetRexxResultString(mess, CheckPassword(mess->rm_Args[2], user->losen) ? "1" : "0");
}
