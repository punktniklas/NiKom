#include "NiKomCompat.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "ServerFuncs.h"
#include <rexx/storage.h>
#include <proto/exec.h>
#ifdef HAVE_PROTO_ALIB_H
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <time.h>
#include <stdio.h>
#ifdef __GNUC__
/* In gcc access() is defined in unistd.h, while SAS/C has the
   prototype in stdio.h */
# include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#ifdef MWDEBUG
#include "/include/memwatch.h"
#endif
#include "FileAreaUtils.h"
#include "RexxUtils.h"
#include "StringUtils.h"
#include "UserDataUtils.h"

// TODO: Get rid of these prototypes
struct Fil *parsefil(char *,int);
void purgeOldTexts(int numberOfTexts);
void sparatext(struct NiKMess *);

extern struct System *Servermem;

struct Header infohead= { 0,0,0,0,{0},0,-1 };
struct MinList edit_list;

void freeeditlist(void);

extern int writefiles(int);
extern int updatefile(int,struct Fil *);

int readtexthead(int nummer,struct Header *head) {
        BPTR fh;
        int fil,pos;
        char filnamn[40];
        fil=nummer/512;
        pos=nummer%512;
        sprintf(filnamn,"NiKom:Moten/Head%d.dat",fil);
        if(!(fh=Open(filnamn,MODE_OLDFILE))) {
                return(1);
        }
        if(Seek(fh,pos*sizeof(struct Header),OFFSET_BEGINNING)==-1) {
                Close(fh);
                return(1);
        }
        if(!Read(fh,(void *)head,sizeof(struct Header))) {
                Close(fh);
                return(1);
        }
        Close(fh);
        return(0);
}

int writetexthead(int nummer,struct Header *head) {
        BPTR fh;
        int fil,pos;
        char filnamn[40];
        fil=nummer/512;
        pos=nummer%512;
        sprintf(filnamn,"NiKom:Moten/Head%d.dat",fil);
        if(!(fh=Open(filnamn,MODE_OLDFILE))) {
                return(1);
        }
        if(Seek(fh,pos*sizeof(struct Header),OFFSET_BEGINNING)==-1) {
                Close(fh);
                return(1);
        }
        if(!Write(fh,(void *)head,sizeof(struct Header))) {
                Close(fh);
                return(1);
        }
        Close(fh);
        return(0);
}

void rexxnodeinfo(struct RexxMsg *mess) {
  char str[100];
  int nodeId;
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  nodeId = atoi(mess->rm_Args[1]);
  if(nodeId < 0 || nodeId >= MAXNOD || Servermem->nodeInfo[nodeId].nodeType == 0) {
    SetRexxResultString(mess, "-1");
    return;
  }
  switch(mess->rm_Args[2][0]) {
  case 'a' : case 'A' :
    strcpy(str, Servermem->nodeInfo[nodeId].currentActivity);
    break;
  case 'c' : case 'C' :
    strcpy(str, Servermem->nodeInfo[nodeId].callerId);
    break;
  case 'd' : case 'D' :
    strcpy(str, Servermem->nodeInfo[nodeId].nodeIdStr);
    break;
  case 'g' : case 'G' :
    sprintf(str, "%ld", Servermem->nodeInfo[nodeId].action);
    break;
  case 'i' : case 'I' :
    sprintf(str, "%ld", Servermem->nodeInfo[nodeId].userLoggedIn);
    break;
  case 'm' : case 'M' :
    sprintf(str, "%ld", Servermem->nodeInfo[nodeId].currentConf);
    break;
  case 'p' : case 'P' :
    sprintf(str, "%ld", time(NULL) - Servermem->nodeInfo[nodeId].lastActiveTime);
    break;
  case 's' : case 'S' :
    sprintf(str, "%ld", Servermem->nodeInfo[nodeId].connectBps);
    break;
  case 't' : case 'T' :
    sprintf(str, "%ld", Servermem->nodeInfo[nodeId].nodeType);
    break;
  default :
    str[0] = '\0';
    break;
  }
  SetRexxResultString(mess, str);
}

void rexxnextfile(struct RexxMsg *mess) {
	struct Fil *pek;
	int area;
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	area=atoi(mess->rm_Args[2]);
	if(area<0 || area>MAXAREA || !Servermem->areor[area].namn[0]) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!(pek=parsefil(mess->rm_Args[1],area))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	pek=(struct Fil *)pek->f_node.mln_Pred;
	if(!pek->f_node.mln_Pred) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(pek->namn,strlen(pek->namn))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void rexxnextpatternfile(struct RexxMsg *mess)
{
	struct Fil *pek;
	int area;
	char buffer[101];

	if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3])) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	area=atoi(mess->rm_Args[2]);

	if(area<0 || area>MAXAREA || !Servermem->areor[area].namn[0]) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	if(!(pek=parsefil(mess->rm_Args[1],area))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	pek=(struct Fil *)pek->f_node.mln_Pred;
	for(;pek->f_node.mln_Pred;pek=(struct Fil *)pek->f_node.mln_Pred)
	{
		if(ParsePatternNoCase(mess->rm_Args[3], buffer, 100) == 1)
		{
			if(MatchPatternNoCase(buffer, pek->namn))
			{
				if(!(mess->rm_Result2=(long)CreateArgstring(pek->namn ,strlen(pek->namn))))
				{
					printf("Kunde inte allokera en ArgString\n");
					mess->rm_Result1=0;
				}
				return;
			}
		}
		else
		{
			if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
			{
				printf("Kunde inte allokera en ArgString\n");
				mess->rm_Result1=0;
			}
			return;
		}
	}

	if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
	{
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
	}
	return;
}

void rexxraderafil(struct RexxMsg *mess) {
        struct Fil *pek;
        char filnamn[110];
        int area;
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        area=atoi(mess->rm_Args[2]);
        if(area<0 || area>MAXAREA || !Servermem->areor[area].namn[0]) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(!(pek=parsefil(mess->rm_Args[1],area))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        Remove((struct Node *)pek);
        if(writefiles(area)) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-5",strlen("-5"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                FreeMem(pek,sizeof(struct Fil));
                return;
        }

        sprintf(filnamn,"%s%s",Servermem->areor[area].dir[pek->dir],pek->namn);
        if(!DeleteFile(filnamn)) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-4"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                FreeMem(pek,sizeof(struct Fil));
                return;
        }

        sprintf(filnamn,"%slongdesc/%s.long",Servermem->areor[area].dir[pek->dir],pek->namn);
        if(pek->flaggor & FILE_LONGDESC && !DeleteFile(filnamn)) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-5",strlen("-5"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                FreeMem(pek,sizeof(struct Fil));
                return;
        }

        FreeMem(pek,sizeof(struct Fil));
        if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

void createtext(struct RexxMsg *mess) {
        int av,komtill,x,len,motnr;
        BPTR fh;
        struct Mote *motpek;
        struct EditLine *el;
        struct NiKMess nmess;
        struct Header sparhead,komhead;
        char str[20],rlbuf[99];
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3]) || (!mess->rm_Args[4]) || (!mess->rm_Args[5])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        if((!mess->rm_Args[2][0]) && (!mess->rm_Args[1][0])) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
                        printf("Kunde inte allokera en ArgString\n");
                 mess->rm_Result1=0;
                return;
        }
        if((!mess->rm_Args[4][0]) && (!mess->rm_Args[1][0])) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(!userexists(av=atoi(mess->rm_Args[3]))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(mess->rm_Args[2][0]) {
                motnr=atoi(mess->rm_Args[2]);
                motpek=getmotpek(motnr);
                if(!motpek) {
                        if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-4"))))
                                printf("Kunde inte allokera en ArgString\n");
                        mess->rm_Result1=0;
                        return;
                }
        } else motnr=-1;
        if(mess->rm_Args[1][0]) {
                komtill=atoi(mess->rm_Args[1]);
                if(komtill>Servermem->info.hightext || komtill<Servermem->info.lowtext) {
                        if(!(mess->rm_Result2=(long)CreateArgstring("-5",strlen("-5"))))
                                printf("Kunde inte allokera en ArgString\n");
                        mess->rm_Result1=0;
                        return;
                }
        } else komtill=-1;
        memset(&sparhead,0,sizeof(struct Header));
        if(!(fh=Open(mess->rm_Args[5],MODE_OLDFILE))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-6",strlen("-6"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        NewList((struct List *)&edit_list);
        for(x=0;;x++) {
                if(!FGets(fh,rlbuf,MAXFULLEDITTKN+1)) break;
                if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) break;
                strcpy(el->text,rlbuf);
                len=strlen(el->text);
                if(el->text[len-1]=='\n') el->text[len-1]=0;
                AddTail((struct List *)&edit_list,(struct Node *)el);
        }
        Close(fh);
        sparhead.rader=x;
        for(x=0;x<MAXKOM;x++) {
                sparhead.kom_i[x]=-1;
                sparhead.kom_av[x]=-1;
        }
        if(komtill>=0 && readtexthead(komtill,&komhead)) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-7",strlen("-7"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(motnr==-1) sparhead.mote=komhead.mote;
        else sparhead.mote=motnr;
        if(mess->rm_Args[4][0]) strncpy(sparhead.arende,mess->rm_Args[4],40);
        else strncpy(sparhead.arende,komhead.arende,40);
        sparhead.person=av;
        sparhead.textoffset=(long)&edit_list;
        if(komtill==-1) {
                sparhead.kom_till_per=-1;
                sparhead.kom_till_nr=-1;
        } else {
                sparhead.kom_till_per=komhead.person;
                sparhead.kom_till_nr=komhead.nummer;
        }
        sparhead.tid=time(NULL);
        nmess.data=(long)&sparhead;
        nmess.nod=-1;
        sparatext(&nmess);
        freeeditlist();
        sprintf(str,"%ld",nmess.data);
        if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
        if(komtill!=-1) {
                x=0;
                while(komhead.kom_i[x]!=-1 && x<MAXKOM) x++;
                if(x==MAXKOM) return;
                komhead.kom_i[x]=nmess.data;
                komhead.kom_av[x]=sparhead.person;
                writetexthead(komtill,&komhead);
        }
}

int updatenextletter(int user) {
        BPTR fh;
        long nr;
        char nrstr[20],filnamn[50];
        sprintf(filnamn,"NiKom:Users/%d/%d/.nextletter",user/100,user);
        if(!(fh=Open(filnamn,MODE_OLDFILE))) {
                printf("Kunde inte öppna .nextfile\n");
                return(-1);
        }
        memset(nrstr,0,20);
        if(Read(fh,nrstr,19)==-1) {
                printf("Kunde inte läsa .nextfile\n");
                Close(fh);
                return(-1);
        }
        nr=atoi(nrstr);
        sprintf(nrstr,"%ld",nr+1);
        if(Seek(fh,0,OFFSET_BEGINNING)==-1) {
                printf("Kunde inte söka i .nextfile\n");
                Close(fh);
                return(-1);
        }
        if(Write(fh,nrstr,strlen(nrstr))==-1) {
                printf("Kunde inte skriva .nextfile\n");
                Close(fh);
                return(-1);
        }
        Close(fh);
        return(nr);
}

void createletter(struct RexxMsg *mess) {
        BPTR fhin,fhout,lock=0;
        int fromuser,tempto,x,letternr,mot;
        long tid;
        struct tm *ts;
        char tostring[100]="",tempstr[15],orgfilename[50],bugbuf[80],*motstr;
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3]) || (!mess->rm_Args[4])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        if(!userexists(fromuser=atoi(mess->rm_Args[1]))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        for(x=4;x<16;x++) {
                if(!mess->rm_Args[x]) break;
                if(!userexists(tempto=atoi(mess->rm_Args[x]))) {
                        if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-1"))))
                                printf("Kunde inte allokera en ArgString\n");
                       mess->rm_Result1=0;
                        return;
                }
                sprintf(tempstr," %d",tempto);
               strcat(tostring,tempstr);
        }
        if(!(fhin=Open(mess->rm_Args[3],MODE_OLDFILE))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-1"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        tempto=atoi(&tostring[1]);
        letternr=updatenextletter(tempto);
        if(letternr==-1) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-1"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                Close(fhin);
                return;
        }
        sprintf(orgfilename,"NiKom:Users/%d/%d/%d.letter",tempto/100,tempto,letternr);
        if(!(fhout=Open(orgfilename,MODE_NEWFILE))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-5",strlen("-1"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                Close(fhin);
                return;
        }
        strcpy(bugbuf,"System-ID: NiKom\n");
        FPuts(fhout,bugbuf);
        sprintf(bugbuf,"From: %d\n",fromuser);
        FPuts(fhout,bugbuf);
        sprintf(bugbuf,"To:%s\n",tostring);
        FPuts(fhout,bugbuf);
        time(&tid);
        ts=localtime(&tid);
        sprintf(bugbuf,"Date: %4d%02d%02d %02d:%02d\n", ts->tm_year + 1900,
                ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min);
        FPuts(fhout,bugbuf);
        sprintf(bugbuf,"Subject: %s\n",mess->rm_Args[2]);
        FPuts(fhout,bugbuf);
        while(FGets(fhin,bugbuf,80)) if(FPuts(fhout,bugbuf)==-1) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-6",strlen("-1"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                Close(fhin); Close(fhout);
                return;
        }
        Close(fhout);
        Close(fhin);
        motstr=FindNextWord(&tostring[1]);
        if(motstr[0]) {
                if(!(lock=Lock(orgfilename,ACCESS_READ))) {
                        printf("Kunde inte få ett lock för brevet\n\r");
                        if(!(mess->rm_Result2=(long)CreateArgstring("-7",strlen("-1"))))
                                printf("Kunde inte allokera en ArgString\n");
                        mess->rm_Result1=0;
                        return;
                }
        }
        while(motstr[0]) {
                mot=atoi(motstr);
                if((letternr=updatenextletter(mot))==-1) {
                        UnLock(lock);
                        if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-1"))))
                                printf("Kunde inte allokera en ArgString\n");
                        mess->rm_Result1=0;
                        return;
                }
                sprintf(bugbuf,"NiKom:Users/%d/%d/%d.letter",mot/100,mot,letternr);
                if(!MakeLink(bugbuf,lock,FALSE)) {
                        if(!(mess->rm_Result2=(long)CreateArgstring("-8",strlen("-1"))))
                                printf("Kunde inte allokera en ArgString\n");
                        mess->rm_Result1=0;
                        UnLock(lock);
                        return;
                }
                motstr=FindNextWord(motstr);
        }
        if(lock) UnLock(lock);
        if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

void textinfo(struct RexxMsg *mess) {
        int nummer,foo;
        struct tm *ts;
        char str[100];
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        nummer=atoi(mess->rm_Args[1]);
        if(nummer<Servermem->info.lowtext || nummer>Servermem->info.hightext) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }

        if(GetConferenceForText(nummer) == -1) {
          if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
            printf("Kunde inte allokera en ArgString\n");
          mess->rm_Result1=0;
          return;
        }

        if(nummer!=infohead.nummer) readtexthead(nummer,&infohead);
        switch(mess->rm_Args[2][0]) {
                case 'a' : case 'A' :
                        foo=atoi(&mess->rm_Args[2][1]);
                        if(foo>=0 && foo<MAXKOM) sprintf(str,"%ld",infohead.kom_av[foo]);
                        else strcpy(str,"-1");
                        break;
                case 'f' : case 'F' :
                        sprintf(str,"%ld",infohead.person);
                        break;
                case 'i' : case 'I' :
                        foo=atoi(&mess->rm_Args[2][1]);
                        if(foo>=0 && foo<MAXKOM) sprintf(str,"%ld",infohead.kom_i[foo]);
                        else strcpy(str,"-1");
                        break;
                case 'k' : case 'K' :
                        sprintf(str,"%ld",infohead.kom_till_nr);
                        break;
                case 'm' : case 'M' :
                  sprintf(str, "%d", GetConferenceForText(nummer));
                  break;
                case 'p' : case 'P' :
                        sprintf(str,"%ld",infohead.kom_till_per);
                        break;
                case 'r' : case 'R' :
                        strcpy(str,infohead.arende);
                        break;
                case 't' : case 'T' :
                        ts=localtime(&infohead.tid);
                        sprintf(str,"%4d%02d%02d %02d:%02d", ts->tm_year + 1900,
                                ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
                                ts->tm_min);
                        break;
                default :
                        str[0]=0;
                        break;
        }
        if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

void nextunread(struct RexxMsg *mess) {
  int textId, confId, userId, nextUnread;
  struct Mote *conf;
  char str[100];
  struct UnreadTexts *unreadTexts, unreadTextsBuf;
  
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  textId = atoi(mess->rm_Args[1]);
  if(textId < Servermem->info.lowtext || textId > Servermem->info.hightext) {
    SetRexxResultString(mess, "-1");
    return;
  }
  confId = atoi(mess->rm_Args[2]);
  conf = getmotpek(confId);
  if(!conf) {
    SetRexxResultString(mess, "-2");
    return;
  }
  userId = atoi(mess->rm_Args[3]);
  if(!userexists(userId)) {
    SetRexxResultString(mess, "-3");
    return;
  }

  if(GetLoggedInUser(userId, &unreadTexts)) {
    if(!ReadUnreadTexts(&unreadTextsBuf, userId)) {
      SetRexxResultString(mess, "-4");
      return;
    }
    unreadTexts = &unreadTextsBuf;
  }
  
  nextUnread = FindNextUnreadText(textId + 1, confId, unreadTexts);
  if(nextUnread == -1) {
    SetRexxResultString(mess, "-5");
    return;
  }
  sprintf(str, "%d", nextUnread);
  SetRexxResultString(mess, str);
}

void freeeditlist(void) {
        struct EditLine *el;
        while((el=(struct EditLine *)RemHead((struct List *)&edit_list)))
                FreeMem(el,sizeof(struct EditLine));
        NewList((struct List *)&edit_list);
}

struct Mote *getmotpek(int motnr) {
        struct Mote *letpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        for(;letpek->mot_node.mln_Succ;letpek=(struct Mote *)letpek->mot_node.mln_Succ)
                if(letpek->nummer==motnr) return(letpek);
        return(NULL);
}

char *getmotnamn(int motnr) {
        struct Mote *motpek=getmotpek(motnr);
        if(!motpek) return(NULL);
        return(motpek->namn);
}

int motesratt(int mote,struct User *usr) {
	struct Mote *motpek;
	motpek=getmotpek(mote);
	if(!motpek) return(0);
	if(usr->status >= Servermem->cfg->st.medmoten) return(1);
	if(motpek->status & SUPERHEMLIGT) return(1);
	if(motpek->status & AUTOMEDLEM && !(motpek->status & SKRIVSTYRT)) return(1);
	if(BAMTEST(usr->motratt,mote)) return(1);
	if(motpek->grupper & usr->grupper) return(1);
	return(0);
}

int motesmed(int mote,struct User *usr) {
        struct Mote *motpek;
        motpek=getmotpek(mote);
        if(!motpek) return(0);
        if(motpek->status & SUPERHEMLIGT) return(1);
        if(motpek->status & AUTOMEDLEM) return(1);
        if(!motesratt(mote,usr)) return(0);
        if(BAMTEST(usr->motmed,mote)) return(1);
        return(0);
}

void meetright(struct RexxMsg *mess) {
  struct Mote *motpek;
  int confId, userId;
  struct User *user;
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  userId = atoi(mess->rm_Args[1]);
  confId = atoi(mess->rm_Args[2]);
  if(!userexists(userId)) {
    SetRexxResultString(mess, "-1");
    return;
  }
  if(!(user = GetUserData(userId))) {
    SetRexxResultString(mess, "-2");
    return;
  }
  if(!(motpek = getmotpek(confId))) {
    SetRexxResultString(mess, "-3");
    return;
  }
  SetRexxResultString(mess, motesratt(confId, user) ? "1" : "0");
}

void meetmember(struct RexxMsg *mess) {
  struct Mote *motpek;
  int confId, userId;
  struct User *user;
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  userId = atoi(mess->rm_Args[1]);
  confId = atoi(mess->rm_Args[2]);
  if(!userexists(userId)) {
    SetRexxResultString(mess, "-1");
    return;
  }
  if(!(user = GetUserData(userId))) {
    SetRexxResultString(mess, "-3");
    return;
  }
  if(!(motpek = getmotpek(confId))) {
    SetRexxResultString(mess, "-3");
    return;
  }
  SetRexxResultString(mess, motesmed(confId, user) ? "1" : "0");
}

void chgmeetright(struct RexxMsg *mess) {
  struct Mote *motpek;
  int confId, userId, needsWrite;
  struct User *user;
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  userId = atoi(mess->rm_Args[1]);
  confId = atoi(mess->rm_Args[2]);
  if(mess->rm_Args[3][0] != '-' && mess->rm_Args[3][0] != '+') {
    SetRexxResultString(mess, "-1");
    return;
  }
  if(!userexists(userId)) {
    SetRexxResultString(mess, "-2");
    return;
  }
  if(!(user = GetUserDataForUpdate(userId, &needsWrite))) {
    SetRexxResultString(mess, "-3");
    return;
  }
  if(!(motpek = getmotpek(confId))) {
    SetRexxResultString(mess, "-4");
    return;
  }
  if(mess->rm_Args[3][0] == '-') {
    BAMCLEAR(user->motratt,confId);
  } else {
    BAMSET(user->motratt,confId);
  }
  if(needsWrite) {
    if(!WriteUser(userId, user, FALSE)) {
      SetRexxResultString(mess, "-5");
      return;
    }
  }
  SetRexxResultString(mess, "0");
}

/*
** movefile - flyttar en fil från en area till en annan
** AREXXDOC:

MoveFile(filnamn,area,ny area)

Flyttar filinformationen (inte den fysiska filen) från area
till nyarea. Du måste själv flytta filen fysiskt _innan_ du
anropar denna funktion.
Ex:
    fromdir = AreaInfo(area,'d'FileInfo(fil,'i',area))
    todir   = AreaInfo(nyarea,'d1')
    address command 'move 'fromdir||fil todir
    say MoveFile(fil,area,nyarea)

Felvärden:
1 - Inte tillräckligt med argument

Returvärden:
0   - Allt ok.
-1  - Gamla arean finns inte
-2  - Filen finns inte
-3  - Nya arean finns inte
-4  - Filen finns inte fysiskt i nya arean
-5  - Fel vid skrivning av datafiler

** END AREXXDOC
*/
void movefile(struct RexxMsg *mess) {
        int area,nyarea,x;
        char filnamn[100];
        struct Fil *filpek;
        struct MinNode *sokpek;
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        area=atoi(mess->rm_Args[2]);
        if(area<0 || area>=Servermem->info.areor || !Servermem->areor[area].namn) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(!(filpek=parsefil(mess->rm_Args[1],area))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        nyarea=atoi(mess->rm_Args[3]);
        if(nyarea<0 || nyarea>=Servermem->info.areor || !Servermem->areor[nyarea].namn) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        for(x=0;x<16;x++) {
                sprintf(filnamn,"%s%s",Servermem->areor[nyarea].dir[x],mess->rm_Args[1]);
                if(!access(filnamn,0)) break;
        }
        if(x==16) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-3"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        sprintf(filnamn,"%s%s",Servermem->areor[nyarea].dir[x],mess->rm_Args[1]);

        /* Ok, dags att flytta filinformationen */
        ObtainSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

        /* Ta bort den filen */
        Remove((struct Node *)filpek);
        filpek->dir = x;

        /* Lägg in filen sorterad i nya arean */
        for(sokpek=Servermem->areor[nyarea].ar_list.mlh_Head;sokpek;sokpek=sokpek->mln_Succ) {
            if(((struct Fil *)sokpek)->tid>filpek->tid) break;
        }
        if(sokpek) Insert((struct List *)&Servermem->areor[nyarea].ar_list,(struct Node *)filpek,(struct Node *)sokpek->mln_Pred);
        else AddTail((struct List *)&Servermem->areor[nyarea].ar_list,(struct Node *)filpek);

        /* Klart, skriv filinformationen */
        ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

        if(writefiles(area) || writefiles(nyarea)) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-5",strlen("-8"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

void chgfile(struct RexxMsg *mess) {
        struct Fil *filpek;
        FILE *infil, *utfil;
        long tmpflaggor;
        int area,user;
        char fromfile[100],tofile[100], str[256];
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3]) || (!mess->rm_Args[4])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        area=atoi(mess->rm_Args[4]);
        if(area<0 || area>=Servermem->info.areor || !Servermem->areor[area].namn) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(!(filpek=parsefil(mess->rm_Args[1],area))) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
                printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        switch(mess->rm_Args[3][0]) {
                case 'b' : case 'B' :
                        strncpy(filpek->beskr,mess->rm_Args[2],70);
                        filpek->beskr[70] = 0;
                        break;
                case 'd' : case 'D' :
                        filpek->downloads=atoi(mess->rm_Args[2]);
                        break;
                case 'f' : case 'F' :
                        tmpflaggor=atoi(mess->rm_Args[2]);
                        if((filpek->flaggor & FILE_NOTVALID) && !(tmpflaggor & FILE_NOTVALID))
                                filpek->validtime=time(NULL);
                        filpek->flaggor=tmpflaggor;
                        break;
                case 'i' : case 'I' :
                        filpek->dir=atoi(mess->rm_Args[2]);
                        break;
                case 'n' : case 'N' :
                        sprintf(fromfile,"%s%s",Servermem->areor[area].dir[filpek->dir],filpek->namn);
                        sprintf(tofile,"%s%s",Servermem->areor[area].dir[filpek->dir],mess->rm_Args[2]);
                        if(!Rename(fromfile,tofile)) {
                                if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
                                        printf("Kunde inte allokera en ArgString\n");
                                mess->rm_Result1=0;
                                return;
                        }
                        strcpy(filpek->namn,mess->rm_Args[2]);
                        break;
                case 'r' : case 'R' :
                        filpek->status=atoi(mess->rm_Args[2]);
                        break;
                case 's' : case 'S' :
                        filpek->size=atoi(mess->rm_Args[2]);
                        break;
                case 't' : case 'T' :   {
                    char *tid = mess->rm_Args[2];
                    int l = strlen(tid);
                    int yh;
                    struct tm ts;

                    ts = *localtime(&filpek->tid);

                    /* eventuellt århundrade först */
                    if((l==8) || (l==14))
                    {
                        yh = (tid[0]-'0') * 1000 + (tid[1]-'0') * 100;
                        tid += 2;
                        l -= 2;
                    } else
                    {
                        yh = 1900;
                    }
                    if((l==6) || (l==12))
                    {
                        /* yymmdd */

                        ts.tm_year = (yh - 1900) + (tid[0]-'0') * 10 + (tid[1]-'0');
                        ts.tm_mon  = (tid[2]-'0') * 10 + (tid[3]-'0') - 1;
                        ts.tm_mday = (tid[4]-'0') * 10 + (tid[5]-'0');
                        tid += 7;
                        l -= 7;
                    }
                    if(l==5)
                    {
                        /* hh:mm */
                        ts.tm_hour = (tid[0]-'0') * 10 + (tid[1]-'0');
                        ts.tm_min  = (tid[3]-'0') * 10 + (tid[4]-'0');
                        ts.tm_sec  = 0;
                    }
                    filpek->tid = mktime(&ts);
                    break;              }
                case 'u' : case 'U' :
                        user=atoi(mess->rm_Args[2]);
                        if(userexists(user)) filpek->uppladdare=atoi(mess->rm_Args[2]);
                        else {
                                if(!(mess->rm_Result2=(long)CreateArgstring("-5",strlen("-5"))))
                                        printf("Kunde inte allokera en ArgString\n");
                                mess->rm_Result1=0;
                                return;
                        }
                        break;
			case 'l' : case 'L' :
				if(mess->rm_Args[2][0] == '\0')
				{
					filpek->flaggor |= !FILE_LONGDESC;
				}
				else
				{
					if(!(infil = fopen(mess->rm_Args[2],"r")))
					{
						if(!(mess->rm_Result2=(long)CreateArgstring("-9",strlen("-9"))))
							printf("Kunde inte allokera en ArgString\n");
						mess->rm_Result1=0;
						return;
					}

					sprintf(str,"%slongdesc/%s.long",Servermem->areor[area].dir[filpek->dir],filpek->namn);
					if(!(utfil = fopen(str,"w")))
					{
						fclose(infil);
						if(!(mess->rm_Result2=(long)CreateArgstring("-10",strlen("-10"))))
							printf("Kunde inte allokera en ArgString\n");
						mess->rm_Result1=0;
						return;
					}

					while(!(feof(infil)))
					{
						if(fgets(str,100,infil))
							fputs(str,utfil);
					}
					filpek->flaggor |= FILE_LONGDESC;

					fclose(infil);
					fclose(utfil);
				}
				break;
        }
        if(updatefile(area,filpek)) {
                if(!(mess->rm_Result2=(long)CreateArgstring("-8",strlen("-8"))))
                        printf("Kunde inte allokera en ArgString\n");
                mess->rm_Result1=0;
                return;
        }
        if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

int parsegrupp(char *skri) {
        int going=TRUE,going2=TRUE,found=-1;
        char *faci,*skri2;
        struct UserGroup *letpek;
        if(skri[0]==0 || skri[0]==' ') return(-3);
        letpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;
        while(letpek->grupp_node.mln_Succ && going) {
                faci=letpek->namn;
                skri2=skri;
                going2=TRUE;
                if(matchar(skri2,faci)) {
                        while(going2) {
                                skri2=FindNextWord(skri2);
                                faci=FindNextWord(faci);
                                if(skri2[0]==0) {
                                        found=letpek->nummer;
                                        going2=going=FALSE;
                                }
                                else if(faci[0]==0) going2=FALSE;
                                else if(!matchar(skri2,faci)) {
                                        faci=FindNextWord(faci);
                                        if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
                                }
                        }
                }
                letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ;
        }
        return(found);
}

void keyinfo(struct RexxMsg *mess) {
        char str[40];
        int nr;
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        switch(mess->rm_Args[2][0]) {
                case 'n' : case 'N' :
                        nr=atoi(mess->rm_Args[1]);
                        if(nr < 0 || nr > Servermem->cfg->noOfFileKeys)
                                sprintf(str,"%d",-1);
                        else
                        {
                                sprintf(str,"%s",Servermem->cfg->fileKeys[nr]);
                                str[strlen(str)-1] = '\0';
						}
                        break;
                default :
                        str[0]=0;
                        break;
        }
        if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

void getdir(struct RexxMsg *mess) {
        char str[100];
        char nycklar[MAXNYCKLAR/8];
        int nyckel, cnt, dirnr, areanum;
        if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
                mess->rm_Result1=1;
                mess->rm_Result2=0;
                return;
        }
        switch(mess->rm_Args[1][0]) {
                case 'd': case 'D' :      // 'd', areanamn, nyckel1, nyckel2 osv
                        if((areanum=atoi(mess->rm_Args[2]))<0 ||
                                areanum>=Servermem->info.areor ||
                                !Servermem->areor[areanum].namn[0])
                        {
                                sprintf(str,"%d",-1); // Ingen area
                                break;
                        }

                        for(cnt=0;cnt<MAXNYCKLAR;cnt++)
                                BAMCLEAR(nycklar,cnt);
                        for(cnt=3;cnt<16 && mess->rm_Args[cnt];cnt++)
                        {
                                if((nyckel=parsenyckel(mess->rm_Args[cnt]))!=-1)
                                {
                                        if(BAMTEST(nycklar,nyckel)) BAMCLEAR(nycklar,nyckel);
                                        else BAMSET(nycklar,nyckel);
                                }
                        }
                        dirnr = ChooseDirectoryInFileArea(areanum, nycklar, 0);
                        if(dirnr<0 || dirnr>15)
                        {
                                str[0]=0;
                                break;
                        }
                       strcpy(str,Servermem->areor[areanum].dir[dirnr]);
                        break;
                default :
                        str[0]=0;
                        break;
        }
        if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
                printf("Kunde inte allokera en ArgString\n");
        mess->rm_Result1=0;
}

void rexxPurgeOldTexts(struct RexxMsg *mess) {
  int numberOfTexts;
  if(!mess->rm_Args[1]) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  numberOfTexts = atoi(mess->rm_Args[1]);
  if(numberOfTexts <= 0) {
    SetRexxResultString(mess, "-1");
    return;
  }
  if(numberOfTexts >= (Servermem->info.hightext - Servermem->info.lowtext)) {
    SetRexxResultString(mess, "-2");
    return;
  }
  if(numberOfTexts % 512) {
    SetRexxResultString(mess, "-3");
    return;
  }
  purgeOldTexts(numberOfTexts);
  SetRexxResultString(mess, "0");
}
