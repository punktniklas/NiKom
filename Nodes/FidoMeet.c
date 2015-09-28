#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"

#define ERROR	10
#define OK		0
#define EKO		1
#define EJEKO	0
#define KOM		1
#define EJKOM	0
#define BREVKOM	-1

extern struct System *Servermem;
extern char outbuffer[],*argument,inmat[];
extern int nodnr,senast_text_typ,senast_text_nr,senast_text_mote,nu_skrivs,inloggad,
	rad,mote2;
extern struct Header readhead,sparhead;
extern struct Inloggning Statstr;
extern struct MinList edit_list;

void fido_lasa(int tnr,struct Mote *motpek) {
	if(tnr<motpek->lowtext || tnr>motpek->texter) {
		puttekn("\r\n\nTexten finns inte!\r\n",-1);
		return;
	}
	fido_visatext(tnr,motpek);
}

int checkfidomote(struct Mote *motpek) {
  if(Servermem->unreadTexts[nodnr].lowestPossibleUnreadText[motpek->nummer]
     <= motpek->texter) {
    return TRUE;
  }
  return FALSE;
}

int countfidomote(struct Mote *motpek) {
  return motpek->texter
    - Servermem->unreadTexts[nodnr].lowestPossibleUnreadText[motpek->nummer]
    + 1;
}

int clearfidomote(struct Mote *motpek) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  int going=TRUE,promret;
  while(going) {
    if(unreadTexts->lowestPossibleUnreadText[motpek->nummer] > motpek->texter) {
      return -5;
    }
    if(unreadTexts->lowestPossibleUnreadText[motpek->nummer] < motpek->lowtext) {
      unreadTexts->lowestPossibleUnreadText[motpek->nummer] = motpek->lowtext;
    }
    if((promret=prompt(210))==-1) return(-1);
    else if(promret==-3) return(-3);
    else if(promret==-4) puttekn("\r\n\nFinns ingen nästa kommentar!\r\n\n",-1);
    else if(promret==-8) return(-8);
    else if(promret==-9) return(-9);
    else if(promret==-11) return(-11);
    else if(promret>=0) return(promret);
    else if(promret==-2 || promret==-6) {
      fido_visatext(unreadTexts->lowestPossibleUnreadText[motpek->nummer], motpek);
      unreadTexts->lowestPossibleUnreadText[motpek->nummer]++;
    }
  }
}

void fido_visatext(int text,struct Mote *motpek) {
	int x,length;
	struct FidoText *ft;
	struct FidoLine *fl;
	char filnamn[20],fullpath[100];
	Servermem->inne[nodnr].read++;
	Servermem->info.lasta++;
	Statstr.read++;
	sprintf(filnamn,"%d.msg",text - motpek->renumber_offset);
	strcpy(fullpath,motpek->dir);
	AddPart(fullpath,filnamn,99);
	if(Servermem->inne[nodnr].flaggor & SHOWKLUDGE) ft=ReadFidoTextTags(fullpath,TAG_DONE);
	else ft=ReadFidoTextTags(fullpath,RFT_NoKludges,TRUE,RFT_NoSeenBy,TRUE,TAG_DONE);
	if(!ft) {
		puttekn("\n\n\rKunde inte läsa texten\n\r",-1);
		return;
	}

	if(Servermem->inne[nodnr].flaggor & CLEARSCREEN)
		eka('\f');
	else
		puttekn("\r\n\n", -1);

	sprintf(outbuffer,"Text %d  Möte: %s    %s\r\n",text,motpek->namn,ft->date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Skriven av %s (%d:%d/%d.%d)\r\n",ft->fromuser,ft->fromzone,ft->fromnet,ft->fromnode,ft->frompoint);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Till: %s\r\n",ft->touser);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Ärende: %s\r\n",ft->subject);
	puttekn(outbuffer,-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(outbuffer);
		for(x=0;x<length-2;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);
	for(fl=(struct FidoLine *)ft->text.mlh_Head;fl->line_node.mln_Succ;fl=(struct FidoLine *)fl->line_node.mln_Succ) {
		if(fl->text[0] == 1) {
			puttekn("^A",-1);
			if(puttekn(&fl->text[1],-1)) break;
		} else if(puttekn(fl->text,-1)) break;
		if(puttekn("\r\n",-1)) break;
	}
	sprintf(outbuffer,"\n(Slut på text %d av %s)\r\n",text,ft->fromuser);
	puttekn(outbuffer,-1);
	FreeFidoText(ft);
	senast_text_typ=TEXT;
	senast_text_nr=text;
	senast_text_mote=motpek->nummer;
}

void makefidodate(char *str) {
	struct DateTime dt;
	char datebuf[14],timebuf[10];
	DateStamp(&dt.dat_Stamp);
	dt.dat_Format = FORMAT_DOS;
	dt.dat_Flags = 0;
	dt.dat_StrDay = NULL;
	dt.dat_StrDate = datebuf;
	dt.dat_StrTime = timebuf;
	DateToStr(&dt);
	dt.dat_StrDate[2] = ' ';
	dt.dat_StrDate[6] = ' ';
	if(dt.dat_StrDate[4] == 'k') dt.dat_StrDate[4] = 'c'; /* Okt -> Oct */
	if(dt.dat_StrDate[5] == 'j') dt.dat_StrDate[5] = 'y'; /* Maj -> May */
	sprintf(str,"%s  %s",dt.dat_StrDate,dt.dat_StrTime);
}

void makefidousername(char *str,int anv) {
	char tmpusername[50];
	int x;
	strcpy(tmpusername,getusername(anv));
	for(x=0;tmpusername[x];x++) if(tmpusername[x] == '#' || tmpusername[x] == '(' || tmpusername[x] == ',' || tmpusername[x] == '/') break;
	tmpusername[x]=0;
	while(tmpusername[--x]==' ') {
		tmpusername[x]=0;
		if(!x) break;
	}
	sprattgok(tmpusername);
	strcpy(str,tmpusername);
}

struct FidoDomain *getfidodomain(int nr,int zone) {
	int x,found = FALSE;
	char *foo;
	for(x=0;x<10;x++) {
		if(!Servermem->fidodata.fd[x].domain[0]) break;
		if(nr && nr == Servermem->fidodata.fd[x].nummer) found = TRUE;
		else {
			foo=Servermem->fidodata.fd[x].zones;
			while(foo[0] && !found) {
				if(atoi(foo) == zone) found=TRUE;
				else foo=hittaefter(foo);
			}
		}
		if(found) break;
	}
	if(!found) return(NULL);
	else return(&Servermem->fidodata.fd[x]);
}


int fido_skriv(int komm,int komtill) {
	int length=0,x=0,editret,nummer;
	struct MinNode *first,*last;
	struct FidoText ft,*komft;
	struct FidoDomain *fd;
	struct Mote *motpek;
	struct FidoLine *fl;
	char filnamn[15],fullpath[100],msgid[50];
	Servermem->action[nodnr] = SKRIVER;
	Servermem->varmote[nodnr] = mote2;
	motpek = getmotpek(mote2);
	memset(&ft,0,sizeof(struct FidoText));
	if(komm) {
		strcpy(fullpath,motpek->dir);
		sprintf(filnamn,"%d.msg",komtill - motpek->renumber_offset);
		AddPart(fullpath,filnamn,99);
		komft = ReadFidoTextTags(fullpath,RFT_HeaderOnly,TRUE,TAG_DONE);
		if(!komft) {
			puttekn("\n\n\rTexten finns inte.\n\r",-1);
			return(0);
		}
		strcpy(ft.touser,komft->fromuser);
		if(!strncmp(komft->subject,"Re:",3)) strcpy(ft.subject,komft->subject);
		else sprintf(ft.subject,"Re: %s",komft->subject);
		strcpy(msgid,komft->msgid);
		FreeFidoText(komft);
	}
	makefidousername(ft.fromuser,inloggad);
	makefidodate(ft.date);
	fd = getfidodomain(motpek->domain,0);
	if(!fd) {
		puttekn("\n\n\rHmm.. Det här mötets domän finns inte.\n\r",-1);
		return(0);
	}
	ft.fromzone = fd->zone;
	ft.fromnet = fd->net;
	ft.fromnode = fd->node;
	ft.frompoint = fd->point;
	ft.attribut = FIDOT_LOCAL;
	sprintf(outbuffer,"\r\n\nMöte: %s    %s\r\n",motpek->namn,ft.date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Skriven av %s (%d:%d/%d.%d)\r\n",ft.fromuser,ft.fromzone,ft.fromnet,ft.fromnode,ft.frompoint);
	puttekn(outbuffer,-1);
	if(!komm) {
		puttekn("Till: (Return='All') ",-1);
		if(getstring(EKO,35,NULL)) return(1);
		if(!inmat[0]) strcpy(ft.touser,"All");
		else {
			strcpy(ft.touser,inmat);
			sprattgok(ft.touser);
		}
		puttekn("\n\rÄrende: ",-1);
		if(getstring(EKO,71,NULL)) return(1);
		strcpy(ft.subject,inmat);
		puttekn("\r\n",-1);
	} else {
		sprintf(outbuffer,"Till: %s\n\r",ft.touser);
		puttekn(outbuffer,-1);
		sprintf(outbuffer,"Ärende: %s\n\r",ft.subject);
		puttekn(outbuffer,-1);
	}
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(ft.subject);
		for(x=0;x<length+8;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);
	editret = edittext(NULL);
	if(editret==1) return(1);
	if(editret==2) return(0);
	Servermem->inne[nodnr].skrivit++;
	Servermem->info.skrivna++;
	Statstr.write++;
	NewList((struct List *)&ft.text);
	first =  edit_list.mlh_Head;
	last = edit_list.mlh_TailPred;
	ft.text.mlh_Head = first;
	ft.text.mlh_TailPred = last;
	last->mln_Succ = (struct MinNode *)&ft.text.mlh_Tail;
	first->mln_Pred = (struct MinNode *)&ft.text;
	fl = AllocMem(sizeof(struct FidoLine),MEMF_CLEAR);
	AddTail((struct List *)&ft.text,(struct Node *)fl);
	fl = AllocMem(sizeof(struct FidoLine),MEMF_CLEAR);
	GetNiKomVersion(NULL,NULL,filnamn);
	sprintf(fl->text,"--- NiKom %s",filnamn);
	AddTail((struct List *)&ft.text,(struct Node *)fl);
	fl = AllocMem(sizeof(struct FidoLine),MEMF_CLEAR);
	sprintf(fl->text," * Origin: %s (%d:%d/%d.%d)",motpek->origin,ft.fromzone,ft.fromnet,ft.fromnode,ft.frompoint);
	AddTail((struct List *)&ft.text,(struct Node *)fl);
	if(!komm) nummer = motpek->renumber_offset + WriteFidoTextTags(&ft,WFT_MailDir,motpek->dir,
	                                                                   WFT_Domain,fd->domain,
	                                                                   WFT_CharSet,motpek->charset,
	                                                                   TAG_DONE);
	else nummer = motpek->renumber_offset + WriteFidoTextTags(&ft,WFT_MailDir,motpek->dir,
	                                                              WFT_Domain,fd->domain,
	                                                              WFT_Reply,msgid,
	                                                              WFT_CharSet,motpek->charset,
	                                                              TAG_DONE);
	if(motpek->texter < nummer) motpek->texter = nummer;
	sprintf(outbuffer,"Texten fick nummer %d\r\n\n",nummer);
	puttekn(outbuffer,-1);
	if(Servermem->cfg.logmask & LOG_BREV) {
          LogEvent(USAGE_LOG, INFO, "%s skriver text %d i %s",
                   getusername(inloggad), nummer, motpek->namn);
	}
	while(first=(struct MinNode *)RemHead((struct List *)&ft.text)) FreeMem(first,sizeof(struct EditLine));
	NewList((struct List *)&edit_list);
	return(0);
}

void fido_endast(struct Mote *motpek,int antal) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  unreadTexts->lowestPossibleUnreadText[motpek->nummer] = motpek->texter - antal + 1;
  if(unreadTexts->lowestPossibleUnreadText[motpek->nummer] < motpek->lowtext) {
    unreadTexts->lowestPossibleUnreadText[motpek->nummer] = motpek->lowtext;
  }
}

void fidolistaarende(struct Mote *motpek,int dir) {
	int from, x,bryt;
	struct FidoText *ft;
	char fullpath[100],filnamn[20];
	if(dir>0) from = motpek->lowtext;
	else from = motpek->texter;
	puttekn("\r\n\nNamn                              Text   Datum  Ärende",-1);
	puttekn("\r\n-------------------------------------------------------------------------\r\n",-1);
	for(x=from;x>=motpek->lowtext && x<=motpek->texter;x+=dir) {
		strcpy(fullpath,motpek->dir);
		sprintf(filnamn,"%d.msg",x - motpek->renumber_offset);
		AddPart(fullpath,filnamn,99);
		ft = ReadFidoTextTags(fullpath,RFT_HeaderOnly,TRUE,TAG_DONE);
		if(!ft) continue;
		ft->date[6]=0;
		ft->subject[27]=0;
		sprintf(outbuffer,"%-34s%5d %s %s\r\n",ft->fromuser,x,ft->date,ft->subject);
		bryt=puttekn(outbuffer,-1);
		FreeFidoText(ft);
		if(bryt) break;
	}
}
