#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Terminal.h"
#include "StringUtils.h"

#include "Brev.h"

#define EKO		1
#define BREVKOM	-1


extern struct System *Servermem;
extern char outbuffer[],*argument,inmat[];
extern int inloggad,nodnr,senast_text_typ,senast_text_nr,senast_text_mote,senast_brev_nr,senast_brev_anv,nu_skrivs;
extern int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf;
extern struct Inloggning Statstr;
extern struct MinList edit_list;
extern struct Header readhead;

struct ReadLetter brevread,brevspar;
char crashmail;

int getzone(char *adr) {
	int x;
	for(x=0;adr[x]!=':' && adr[x]!=' ' && adr[x]!=0;x++);
	if(adr[x]==':') return(atoi(adr));
	else return(0);
}

int getnet(char *adr) {
	int x;
	char *pek;
	for(x=0;adr[x]!=':' && adr[x]!=' ' && adr[x]!=0;x++);
	if(adr[x]==':') pek=&adr[x+1];
	else pek=adr;
	return(atoi(pek));
}

int getnode(char *adr) {
	int x;
	for(x=0;adr[x]!='/' && adr[x]!=' ' && adr[x]!=0;x++);
	return(atoi(&adr[x+1]));
}

int getpoint(char *adr) {
	int x;
	for(x=0;adr[x]!='.' && adr[x]!=' ' && adr[x]!=0;x++);
	if(adr[x]=='.') return(atoi(&adr[x+1]));
	else return(0);
}

int brev_kommentera(void) {
	BPTR fh;
	int nummer,editret,anv;
	char *brevpers,filnamn[50];
	if(argument[0]) {
          if(!IzDigit(argument[0])) {
            SendString("\r\n\nFinns inget sådant brev.\r\n");
            return 0;
          }
		nummer=atoi(argument);
		brevpers=hittaefter(argument);
		if(!brevpers[0]) anv=inloggad;
		else {
			anv=parsenamn(brevpers);
			if(anv==-1) {
				puttekn("\n\n\rAnvändaren finns inte!\n\r",-1);
				return(0);
			}
		}
		sprintf(filnamn,"NiKom:Users/%d/%d/%d.letter",anv/100,anv,nummer);
		if(!(fh=Open(filnamn,MODE_OLDFILE))) {
			puttekn("\r\n\nBrevet finns inte!\r\n\n",-1);
			return(0);
		}
		readletterheader(fh,&brevread);
		Close(fh);
		if(!strncmp(brevread.systemid,"Fido",4)) {
			if(anv!=inloggad) {
				puttekn("\n\n\rDu har inte rätt att kommentera det brevet\n\r",-1);
				return(0);
			}
		} else {
			if(anv!=inloggad && inloggad!=atoi(brevread.from) && Servermem->inne[nodnr].status<Servermem->cfg.st.brev) {
				puttekn("\r\n\nDu har inte skrivit det brevet!\r\n\n",-1);
				return(0);
			}
		}
		senast_text_typ=BREV;
		senast_brev_nr=nummer;
		senast_brev_anv=anv;
	}
	if(!strncmp(brevread.systemid,"Fido",4)) {
		nu_skrivs=BREV_FIDO;
		return(fido_brev(NULL,NULL,NULL));
	} else {
		nu_skrivs=BREV;
		editret=initbrevheader(BREVKOM);
	}
	if(editret==1) return(1);
	if(editret==2) return(0);
	if((editret=edittext(NULL))==1) return(1);
	else if(!editret)	sparabrev();
	return(0);
}

void brev_lasa(int tnr) {
	int brevanv;
	char *arg2;
	arg2=hittaefter(argument);
	if(arg2[0]) {
		brevanv=parsenamn(arg2);
		if(brevanv==-1) {
			puttekn("\n\n\rFinns ingen sådan användare!\n\r",-1);
			return;
		}
	} else brevanv=inloggad;
	if(tnr<getfirstletter(brevanv) || tnr>=getnextletter(brevanv)) {
		puttekn("\r\n\nBrevet finns inte!\r\n",-1);
		return;
	}
	visabrev(tnr,brevanv);
}

int HasUnreadMail(void) {
  BPTR lock;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/%d.letter", inloggad/100, inloggad,
          Servermem->inne[nodnr].brevpek);
  if(lock = Lock(filename, ACCESS_READ)) {
    UnLock(lock);
    return TRUE;
  }
  return FALSE;
}

void NextMail(void) {
  if(!HasUnreadMail()) {
    SendString("\r\n\nDu har inga olästa brev.\r\n\n");
    return;
  }
  g_lastKomTextType = BREV;
  g_lastKomTextNr = Servermem->inne[nodnr].brevpek;
  g_lastKomTextConf = -1;
  visabrev(Servermem->inne[nodnr].brevpek++, inloggad);
}

int countmail(int user,int brevpek) {
	return(getnextletter(user)-brevpek);
}

void varmail(void) {
	int antal;
	antal=countmail(inloggad,Servermem->inne[nodnr].brevpek);
	sprintf(outbuffer,"\r\n\nDu befinner dig i %s.\r\n",Servermem->cfg.brevnamn);
	puttekn(outbuffer,-1);
	if(!antal) puttekn("Du har inga olästa brev.\r\n\n",-1);
	else if(antal==1) puttekn("Du har 1 oläst brev.\r\n\n",-1);
	else {
		sprintf(outbuffer,"Du har %d olästa brev.\r\n\n",antal);
		puttekn(outbuffer,-1);
	}
}

void visabrev(int brev,int anv) {
	BPTR fh;
	int x,length=0,tillmig=FALSE;
	char filnamn[40],*mottagare,textbuf[100],*vemskrev;
	sprintf(filnamn,"NiKom:Users/%d/%d/%d.letter",anv/100,anv,brev);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		sprintf(outbuffer,"\n\n\rKunde inte öppna %s\n\r",filnamn);
		puttekn(outbuffer,-1);
		return;
	}
	readletterheader(fh,&brevread);
	if(!strncmp(brevread.systemid,"Fido",4)) {
		visafidobrev(&brevread,fh,brev,anv);
		return;
	}
	mottagare=brevread.to;
	while(mottagare[0]) {
		if(inloggad==atoi(mottagare)) {
			tillmig=TRUE;
			break;
		}
		mottagare=hittaefter(mottagare);
	}
	if(!tillmig && inloggad!=atoi(brevread.from) && Servermem->inne[nodnr].status<Servermem->cfg.st.brev) {
		puttekn("\n\n\rDu har inte rätt att läsa det brevet!\n\r",-1);
		return;
	}
	Servermem->inne[nodnr].read++;
	Servermem->info.lasta++;
	Statstr.read++;
	sprintf(outbuffer,"\r\n\nText %d  i %s hos %s\r\n",brev,Servermem->cfg.brevnamn,getusername(anv));
	puttekn(outbuffer,-1);
	if(!strncmp(brevread.systemid,"NiKom",5)) sprintf(outbuffer,"Lokalt brev,  %s\n\r",brevread.date);
	else sprintf(outbuffer,"<Okänd brevtyp>  %s\n\r",brevread.date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Avsändare: %s\r\n",getusername(atoi(brevread.from)));
	puttekn(outbuffer,-1);
	if(brevread.reply[0]) {
		vemskrev=hittaefter(hittaefter(brevread.reply));
		sprintf(outbuffer,"Kommentar till en text av %s\r\n",getusername(atoi(vemskrev)));
		puttekn(outbuffer,-1);
	}
	mottagare=brevread.to;
	while(mottagare[0]) {
		sprintf(outbuffer,"Mottagare: %s\n\r",getusername(atoi(mottagare)));
		puttekn(outbuffer,-1);
		mottagare=hittaefter(mottagare);
	}
	sprintf(outbuffer,"Ärende: %s\r\n",brevread.subject);
	puttekn(outbuffer,-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(outbuffer);
		for(x=0;x<length-2;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);

	while(FGets(fh,textbuf,99)) {
		if(puttekn(textbuf,-1)) break;
		eka('\r');
	}
	Close(fh);
	sprintf(outbuffer,"\r\n(Slut på text %d av %s)\r\n",brev,getusername(atoi(brevread.from)));
	puttekn(outbuffer,-1);
	senast_text_typ=BREV;
	senast_brev_nr=brev;
	senast_brev_anv=anv;
}

void visafidobrev(struct ReadLetter *brevread, BPTR fh, int brev, int anv) {
	int length,x;
	char textbuf[100];
	if(anv!=inloggad && Servermem->inne[nodnr].status<Servermem->cfg.st.brev) {
		puttekn("\n\n\rDu har inte rätt att läsa det brevet!\n\r",-1);
		return;
	}
	Servermem->inne[nodnr].read++;
	Servermem->info.lasta++;
	Statstr.read++;
	sprintf(outbuffer,"\r\n\nText %d  i %s hos %s\r\n",brev,Servermem->cfg.brevnamn,getusername(anv));
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Fido-nätbrev,  %s\n\r",brevread->date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Avsändare: %s\r\n",brevread->from);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Mottagare: %s\n\r",brevread->to);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Ärende: %s\r\n",brevread->subject);
	puttekn(outbuffer,-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(outbuffer);
		for(x=0;x<length-2;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);

	while(FGets(fh,textbuf,99)) {
		if(textbuf[0]==1) {
			if(!(Servermem->inne[nodnr].flaggor & SHOWKLUDGE)) continue;
			puttekn("^A",-1);
			if(puttekn(&textbuf[1],-1)) break;
		} else {
			if(puttekn(textbuf,-1)) break;
		}
		eka('\r');
	}
	Close(fh);
	sprintf(outbuffer,"\r\n(Slut på text %d av %s)\r\n",brev,brevread->from);
	puttekn(outbuffer,-1);
	senast_text_typ=BREV;
	senast_brev_nr=brev;
	senast_brev_anv=anv;
}

int recisthere(char *str,int rec) {
	char *pek=str;
	while(pek[0]) {
		if(rec==atoi(pek)) return(1);
		pek=hittaefter(pek);
	}
	return(0);
}

int initbrevheader(int tillpers) {
	int length=0,x=0,lappnr;
	long tid,tempmott;
	struct tm *ts;
	struct User usr;
	char filnamn[40],*mottagare,tempbuf[10],*vemskrev;
	Servermem->action[nodnr] = SKRIVER;
	Servermem->varmote[nodnr] = -1;
	memset(&brevspar,0,sizeof(struct ReadLetter));
	if(tillpers==-1) {
		strcpy(brevspar.to,brevread.from);
		mottagare=brevread.to;
		while(mottagare[0]) {
			tempmott=atoi(mottagare);
			if(tempmott==inloggad || recisthere(brevspar.to,tempmott)) {
				mottagare=hittaefter(mottagare);
				continue;
			}
			sprintf(tempbuf," %d",tempmott);
			strcat(brevspar.to,tempbuf);
			mottagare=hittaefter(mottagare);
		}
		sprintf(brevspar.reply,"%d %d %s",senast_brev_anv,senast_brev_nr,brevread.from);
	} else {
		sprintf(brevspar.to,"%d",tillpers);
	}
	sprintf(brevspar.from,"%d",inloggad);
	readuser(atoi(brevspar.to),&usr);
	if(usr.flaggor & LAPPBREV) {
		puttekn("\r\n\n",-1);
		lappnr=atoi(brevspar.to);
		sprintf(filnamn,"NiKom:Users/%d/%d/Lapp",lappnr/100,lappnr);
		if(!access(filnamn,0)) sendfile(filnamn);
		puttekn("\r\n",-1);
	}
	time(&tid);
	ts=localtime(&tid);
	sprintf(brevspar.date,"%02d%02d%02d %02d:%02d", ts->tm_year % 100,
                ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min);
	strcpy(brevspar.systemid,"NiKom");
	sprintf(outbuffer,"\r\n\nMöte: %s\r\n",Servermem->cfg.brevnamn);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Lokalt brev,  %s\n\r",brevspar.date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Avsändare: %s\r\n",getusername(inloggad));
	puttekn(outbuffer,-1);
	if(brevspar.reply[0]) {
		vemskrev=hittaefter(hittaefter(brevspar.reply));
		sprintf(outbuffer,"Kommentar till en text av %s\r\n",getusername(atoi(vemskrev)));
		puttekn(outbuffer,-1);
	}
	mottagare=brevspar.to;
	while(mottagare[0]) {
		sprintf(outbuffer,"Mottagare: %s\n\r",getusername(atoi(mottagare)));
		puttekn(outbuffer,-1);
		mottagare=hittaefter(mottagare);
	}
	puttekn("Ärende: ",-1);
	if(tillpers==-1) {
		strcpy(brevspar.subject,brevread.subject);
		puttekn(brevspar.subject,-1);
	} else {
		if(getstring(EKO,40,NULL)) return(1);
		if(!inmat[0]) {
			eka('\n');
			return(2);
		}
		strcpy(brevspar.subject,inmat);
	}
	puttekn("\r\n",-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(brevspar.subject);
		for(x=0;x<length+8;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);
	return(0);
}

void sprattgok(char *str) {
	int x;
	for(x=0;str[x];x++) {
		switch(str[x]) {
			case 'å' : str[x] = 'a'; break;
			case 'ä' : str[x] = 'a'; break;
			case 'ö' : str[x] = 'o'; break;
			case 'Å' : str[x] = 'A'; break;
			case 'Ä' : str[x] = 'A'; break;
			case 'Ö' : str[x] = 'O'; break;
		}
	}
}

int fido_brev(char *tillpers,char *adr,struct Mote *motpek) {
  int length = 0, x = 0, editret, chrs, inputch, wantCopy;
	struct FidoDomain *fd;
	struct FidoText *komft,ft;
	struct MinNode *first, *last;
	char *foo,tmpfrom[100],fullpath[100],filnamn[20],subject[80],msgid[50];
	if(!(Servermem->inne[nodnr].grupper & Servermem->fidodata.mailgroups) &&
		Servermem->inne[nodnr].status < Servermem->fidodata.mailstatus) {
		puttekn("\n\n\rDu har ingen rätt att skicka FidoNet NetMail.\n\r",-1);
		return(0);
	}
	Servermem->action[nodnr] = SKRIVER;
	Servermem->varmote[nodnr] = -1;
	memset(&ft,0,sizeof(struct FidoText));
	if(!tillpers) { /* Det handlar om en kommentar */
		if(motpek) { /* Det är en personlig kommentar */
			strcpy(fullpath,motpek->dir);
			sprintf(filnamn,"%d.msg",senast_text_nr - motpek->renumber_offset);
			AddPart(fullpath,filnamn,99);
			komft = ReadFidoTextTags(fullpath,RFT_HeaderOnly,TRUE,TAG_DONE);
			if(!komft) {
                          LogEvent(SYSTEM_LOG, ERROR, "Can't read fido text %s.", fullpath);
                          DisplayInternalError();
                          return 0;
			}
			strcpy(ft.touser,komft->fromuser);
			ft.tozone = komft->fromzone;
			ft.tonet = komft->fromnet;
			ft.tonode = komft->fromnode;
			ft.topoint = komft->frompoint;
			strcpy(subject,komft->subject);
			strcpy(msgid,komft->msgid);
			FreeFidoText(komft);
		} else { /* Det är en kommentar av ett brev */
			strcpy(tmpfrom,brevread.from);
			foo=strchr(tmpfrom,'(');
			if(!foo) {
				puttekn("\n\n\rDen kommenterade texten saknar adress!\n\r",-1);
				return(0);
			}
			*(foo-1)='\0';
			foo++;
			strcpy(ft.touser,tmpfrom);
			ft.tozone=getzone(foo);
			ft.tonet=getnet(foo);
			ft.tonode=getnode(foo);
			ft.topoint=getpoint(foo);
			strcpy(subject,brevread.subject);
			strcpy(msgid,brevread.messageid);

		}
	} else { /* Det är ett helt nytt brev */
		strcpy(ft.touser,tillpers);
		sprattgok(ft.touser);
		ft.tozone=getzone(adr);
		ft.tonet=getnet(adr);
		ft.tonode=getnode(adr);
		ft.topoint=getpoint(adr);
	}
	fd = getfidodomain(0,ft.tozone);
	if(!fd) {
          SendString("\n\n\rDu kan inte skriva brev till zon %d.\n\r", ft.tozone);
          return 0;
	}
	if(!tillpers && !motpek) {
		foo = strchr(brevread.to,'(')+1;
		ft.fromzone = getzone(foo);
		ft.fromnet = getnet(foo);
		ft.fromnode = getnode(foo);
		ft.frompoint = getpoint(foo);
	} else {
		ft.fromzone = fd->zone;
		ft.fromnet = fd->net;
		ft.fromnode = fd->node;
		ft.frompoint = fd->point;
	}
	ft.attribut = FIDOT_PRIVATE | FIDOT_LOCAL;
	makefidousername(ft.fromuser,inloggad);
	makefidodate(ft.date);
	sprintf(outbuffer,"\r\n\nMöte: %s\r\n",Servermem->cfg.brevnamn);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Fido-nätbrev,  %s\n\r",ft.date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Avsändare: %s (%d:%d/%d.%d)\r\n",ft.fromuser,ft.fromzone,ft.fromnet,ft.fromnode,ft.frompoint);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Mottagare: %s (%d:%d/%d.%d)\n\r",ft.touser,ft.tozone,ft.tonet,ft.tonode,ft.topoint);
	puttekn(outbuffer,-1);
	puttekn("Ärende: ",-1);
	if(!tillpers) {
		if(!strncmp(subject,"Re:",3)) strcpy(ft.subject,subject);
		else sprintf(ft.subject,"Re: %s",subject);
		puttekn(ft.subject,-1);
	} else {
		if(getstring(EKO,40,NULL)) return(1);
		if(!inmat[0]) {
			eka('\n');
			return(0);
		}
		strcpy(ft.subject,inmat);
	}
	puttekn("\r\n",-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(ft.subject);
		for(x=0;x<length+8;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);
	crashmail = FALSE;
	editret = edittext(NULL);
	if(editret==1) return(1);
	if(editret==2) return(0);
	if(crashmail) ft.attribut |= FIDOT_CRASH;
	Servermem->inne[nodnr].skrivit++;
	Servermem->info.skrivna++;
	Statstr.write++;
	puttekn("\n\n\rTill vilken teckenuppsättning ska brevet konverteras?\r\n\n",-1);
	puttekn("1: ISO Latin 8-bitars tecken (Default)\n\r",-1);
	puttekn("2: IBM PC 8-bitars tecken\r\n",-1);
	puttekn("3: Macintosh 8-bitars tecken\r\n",-1);
	puttekn("4: Svenska 7-bitars tecken\r\n\n",-1);
	puttekn("Val: ",-1);
	for(;;) {
          inputch = GetChar();
          if(inputch == GETCHAR_LOGOUT) {
            return 1;
          }
          if(inputch == GETCHAR_RETURN) {
            inputch = '1';
          }
          if(inputch >= '1' && inputch <= '4') break;
	}
	sprintf(outbuffer,"%c\r\n\n",inputch);
	puttekn(outbuffer,-1);
	switch(inputch) {
		case '1' : chrs=CHRS_LATIN1; break;
		case '2' : chrs=CHRS_CP437; break;
		case '3' : chrs=CHRS_MAC; break;
		case '4' : chrs=CHRS_SIS7; break;
	}
	NewList((struct List *)&ft.text);
	first =  edit_list.mlh_Head;
	last = edit_list.mlh_TailPred;
	ft.text.mlh_Head = first;
	ft.text.mlh_TailPred = last;
	last->mln_Succ = (struct MinNode *)&ft.text.mlh_Tail;
	first->mln_Pred = (struct MinNode *)&ft.text;
	if(tillpers) x = WriteFidoTextTags(&ft,WFT_MailDir,Servermem->fidodata.matrixdir,
	                                              WFT_Domain,fd->domain,
	                                              WFT_CharSet,chrs,
	                                              TAG_DONE);
	else x = WriteFidoTextTags(&ft,WFT_MailDir,Servermem->fidodata.matrixdir,
	                                    WFT_Domain,fd->domain,
	                                    WFT_Reply,msgid,
	                                    WFT_CharSet,chrs,
	                                    TAG_DONE);
	sprintf(outbuffer,"Brevet fick nummer %d\r\n\n",x);
	puttekn(outbuffer,-1);
	if(Servermem->cfg.logmask & LOG_BREV) {
          LogEvent(USAGE_LOG, INFO, "%s skickar brev %d till %s (%d:%d/%d.%d)",
                   getusername(inloggad), x,
                   ft.touser, ft.tozone, ft.tonet, ft.tonode, ft.topoint);
	}

        if(GetYesOrNo(NULL, "Vill du ha en kopia av brevet i din egen brevlåda?",
                      NULL, NULL, "Ja", "Nej", "\r\n\n",
                      FALSE, &wantCopy)) {
          return 1;
        }
        if(wantCopy) {
          savefidocopy(&ft,inloggad);
        }

	while(first=(struct MinNode *)RemHead((struct List *)&ft.text)) FreeMem(first,sizeof(struct EditLine));
	NewList((struct List *)&edit_list);
	return(0);
}

int updatenextletter(int user) {
	BPTR fh;
	long nr;
	char nrstr[20],filnamn[50];
	sprintf(filnamn,"NiKom:Users/%d/%d/.nextletter",user/100,user);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\n\n\rKunde inte öppna .nextfile\n\r",-1);
		return(-1);
	}
	memset(nrstr,0,20);
	if(Read(fh,nrstr,19)==-1) {
		puttekn("\n\n\rKunde inte läsa .nextfile\n\r",-1);
		Close(fh);
		return(-1);
	}
	nr=atoi(nrstr);
	sprintf(nrstr,"%d",nr+1);
	if(Seek(fh,0,OFFSET_BEGINNING)==-1) {
		puttekn("\n\n\rKunde inte söka i .nextfile\n\r",-1);
		Close(fh);
		return(-1);
	}
	if(Write(fh,nrstr,strlen(nrstr))==-1) {
		puttekn("\n\n\rKunde inte skriva .nextfile\n\r",-1);
		Close(fh);
		return(-1);
	}
	Close(fh);
	return(nr);
}

void sparabrev(void) {
	BPTR fh,lock=NULL;
	struct EditLine *elpek;
	char bugbuf[100],orgfilename[50],*motstr;
	int till,nr,mot;
	Servermem->inne[nodnr].skrivit++;
	Servermem->info.skrivna++;
	Statstr.write++;
	till=atoi(brevspar.to);
	if((nr=updatenextletter(till))==-1) {
		freeeditlist();
		return;
	}
	sprintf(orgfilename,"NiKom:Users/%d/%d/%d.letter",till/100,till,nr);
	if(!(fh=Open(orgfilename,MODE_NEWFILE))) {
		puttekn("\n\n\rKunde inte öppna brevet\n\r",-1);
		freeeditlist();
		return;
	}
	strcpy(bugbuf,"System-ID: NiKom\n");
	FPuts(fh,bugbuf);
	sprintf(bugbuf,"From: %d\n",inloggad);
	FPuts(fh,bugbuf);
	sprintf(bugbuf,"To: %s\n",brevspar.to);
	FPuts(fh,bugbuf);
	if(brevspar.reply[0]) {
		sprintf(bugbuf,"Reply: %s\n",brevspar.reply);
		FPuts(fh,bugbuf);
	}
	sprintf(bugbuf,"Date: %s\n",brevspar.date);
	FPuts(fh,bugbuf);
	sprintf(bugbuf,"Subject: %s\n",brevspar.subject);
	FPuts(fh,bugbuf);
	for(elpek=(struct EditLine *)edit_list.mlh_Head;elpek->line_node.mln_Succ;elpek=(struct EditLine *)elpek->line_node.mln_Succ) {
		if(FPuts(fh,elpek->text)==-1) {
			puttekn("\n\n\rFel vid skrivandet av brevet\n\r",-1);
			break;
		}
		FPutC(fh,'\n');
	}
	Close(fh);
	freeeditlist();
	sprintf(outbuffer,"\r\nBrevet fick nummer %d hos %s\r\n",nr,getusername(till));
	puttekn(outbuffer,-1);
	if(Servermem->cfg.logmask & LOG_BREV) {
          strcpy(bugbuf,getusername(inloggad));
          LogEvent(USAGE_LOG, INFO, "%s skickar brev %d till %s",
                   bugbuf, nr, getusername(till));
	}
	motstr=hittaefter(brevspar.to);
	if(motstr[0]) {
		if(!(lock=Lock(orgfilename,ACCESS_READ))) {
			puttekn("\n\n\rKunde inte få ett lock för brevet\n\r",-1);
			return;
		}
	}
	while(motstr[0]) {
		mot=atoi(motstr);
		if((nr=updatenextletter(mot))==-1) {
			UnLock(lock);
			return;
		}
		sprintf(bugbuf,"NiKom:Users/%d/%d/%d.letter",mot/100,mot,nr);
		if(!MakeLink(bugbuf,lock,FALSE)) {
			puttekn("\n\n\rKunde inte skapa länk till brevet\n\r",-1);
			UnLock(lock);
			return;
		}
		sprintf(outbuffer,"\r\nBrevet fick nummer %d hos %s\r\n",nr,getusername(mot));
		puttekn(outbuffer,-1);
		if(Servermem->cfg.logmask & LOG_BREV) {
                  strcpy(bugbuf, getusername(inloggad));
                  LogEvent(USAGE_LOG, INFO, "%s skickar brev %d till %s",
                           bugbuf, nr, getusername(mot));
		}
		motstr=hittaefter(motstr);
	}
	if(lock) UnLock(lock);
}

void savefidocopy(struct FidoText *ft,int anv) {
	struct FidoLine *fl;
	BPTR fh;
	int nummer;
	char *foo,buffer[100];
	nummer=updatenextletter(anv);
	if(nummer==-1) {
		return;
	}
	sprintf(buffer,"NiKom:Users/%d/%d/%d.letter",anv/100,anv,nummer);
	if(!(fh=Open(buffer,MODE_NEWFILE))) {
		return;
	}
	FPuts(fh,"System-ID: Fido\n");
	sprintf(buffer,"From: %s (%d:%d/%d.%d)\n",ft->fromuser,ft->fromzone,ft->fromnet,ft->fromnode,ft->frompoint);
	FPuts(fh,buffer);
	sprintf(buffer,"To: %s (%d:%d/%d.%d)\n",ft->touser,ft->tozone,ft->tonet,ft->tonode,ft->topoint);
	FPuts(fh,buffer);
	sprintf(buffer,"Date: %s\n",ft->date);
	FPuts(fh,buffer);
	foo = hittaefter(ft->subject);
	if(ft->subject[0] == 0 || (ft->subject[0] == ' ' && foo[0] == 0)) strcpy(buffer,"Subject: -\n");
	else sprintf(buffer,"Subject: %s\n",ft->subject);
	FPuts(fh,buffer);
	for(fl=(struct FidoLine *)ft->text.mlh_Head;fl->line_node.mln_Succ;fl=(struct FidoLine *)fl->line_node.mln_Succ) {
		FPuts(fh,fl->text);
		FPutC(fh,'\n');
	}
	Close(fh);
	sprintf(outbuffer,"\n\n\rBrevet fick nummer %d i din brevlåda\r\n\n",nummer);
	puttekn(outbuffer,-1);
}

int skickabrev(void) {
	int pers,editret;
	char *adr;
	if(!(adr=strchr(argument,'@'))) {
		if((pers=parsenamn(argument))==-1) {
			puttekn("\r\n\nFinns ingen som heter så eller har det numret\r\n\n",-1);
			return(0);
		} else if(pers==-3) {
			puttekn("\r\n\nSkriv: Brev <användare>\r\n\n",-1);
			return(0);
		}
		nu_skrivs=BREV;
	} else {
		nu_skrivs=BREV_FIDO;
		*adr='\0';
		adr++;
		if(!getzone(adr) || !getnet(adr)) {
			puttekn("\n\n\rSkriv: Brev <användare>@<zon>:<nät>/<nod>[.<point>]\n\r",-1);
			return(0);
		}
	}
	if(nu_skrivs==BREV) editret=initbrevheader(pers);
	else if(nu_skrivs==BREV_FIDO) return(fido_brev(argument,adr,NULL));
	else return(0);
	if(editret==1) return(1);
	if(editret==2) return(0);
	if((editret=edittext(NULL))==1) return(1);
	if(!editret) sparabrev();
	return(0);
}

void initpersheader(void) {
	long tid,lappnr,length,x;
	struct tm *ts;
	struct User usr;
	char filnamn[40];
	Servermem->action[nodnr] = SKRIVER;
	Servermem->varmote[nodnr] = -1;
	memset(&brevspar,0,sizeof(struct ReadLetter));
	sprintf(brevspar.to,"%d",readhead.person);
	sprintf(brevspar.from,"%d",inloggad);
	readuser(readhead.person,&usr);
	if(usr.flaggor & LAPPBREV) {
		puttekn("\r\n\n",-1);
		lappnr=atoi(brevspar.to);
		sprintf(filnamn,"NiKom:Users/%d/%d/Lapp",lappnr/100,lappnr);
		if(!access(filnamn,0)) sendfile(filnamn);
		puttekn("\r\n",-1);
	}
	time(&tid);
	ts=localtime(&tid);
	sprintf(brevspar.date,"%02d%02d%02d %02d:%02d", ts->tm_year % 100,
                ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min);
	strcpy(brevspar.systemid,"NiKom");
	sprintf(outbuffer,"\r\n\nMöte: %s\r\n",Servermem->cfg.brevnamn);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Lokalt brev,  %s\n\r",brevspar.date);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Avsändare: %s\r\n",getusername(inloggad));
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Mottagare: %s\n\r",getusername(readhead.person));
	puttekn(outbuffer,-1);
	puttekn("Ärende: ",-1);
	strcpy(brevspar.subject,readhead.arende);
	puttekn(brevspar.subject,-1);
	puttekn("\n\r",-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(brevspar.subject);
		for(x=0;x<length+8;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else eka('\n');
}

void listabrev(void) {
	BPTR fh;
	int anv,x,first;
	char filnamn[50],namn[50];
	struct ReadLetter listhead;
	if(argument[0]) {
		anv=parsenamn(argument);
		if(anv==-1) {
			puttekn("\n\n\rFinns ingen sådan användare!\n\r",-1);
			return;
		}
	} else anv=inloggad;
	first=getfirstletter(anv);
	x=getnextletter(anv)-1;
	sprintf(outbuffer,"\n\n\rLägsta brev: %d  Högsta brev: %d\n\n\r",first,x);
	puttekn(outbuffer,-1);
	puttekn("Namn                              Text   Datum  Ärende\n\r",-1);
	puttekn("-------------------------------------------------------------------------\r\n",-1);
	for(;x>=first;x--) {
		sprintf(filnamn,"NiKom:Users/%d/%d/%d.letter",anv/100,anv,x);
		if(!(fh=Open(filnamn,MODE_OLDFILE))) {
			sprintf(outbuffer,"\n\rKunde inte öppna %s\n\r",filnamn);
			puttekn(outbuffer,-1);
			return;
		}
		if(readletterheader(fh,&listhead)) {
			sprintf(outbuffer,"\n\rKunde inte läsa %s\n\r",filnamn);
			puttekn(outbuffer,-1);
			Close(fh);
			return;
		}
		Close(fh);
		listhead.subject[26]=0;
		listhead.date[6]=0;
		if(!strcmp(listhead.systemid,"NiKom")) {
			if(anv!=inloggad && inloggad!=atoi(listhead.from)) continue;
			strcpy(namn,getusername(atoi(listhead.from)));
			sprintf(outbuffer,"%-34s%5d %s %s\r\n",namn,x,listhead.date,listhead.subject);
		} else if(!strcmp(listhead.systemid,"Fido")) {
			if(anv!=inloggad) continue;
			sprintf(outbuffer,"%-34s%5d %s %s\r\n",listhead.from,x,listhead.date,listhead.subject);
		} else sprintf(outbuffer,"%s%5d\r\n","<Okänd brevtyp>",x);
		if(puttekn(outbuffer,-1)) return;
	}
}

void rensabrev(void) {
	BPTR fh;
	int first,next,antal,x;
	char filnamn[50],nrbuf[20];
	first=getfirstletter(inloggad);
	next=getnextletter(inloggad);
	if(!argument[0]) {
		puttekn("\n\n\rSkriv: Rensa Brev <antal brev att radera>\n\r",-1);
		return;
	}
	antal=atoi(argument);
	if(antal <= 0) {
		puttekn("\n\n\rDu kan bara rensa ett positivt antal brev.\n\r",-1);
		return;
	}
	if(first+antal>next) {
		sprintf(outbuffer,"\n\n\rDu har bara %d brev!\n\r",next-first);
		puttekn(outbuffer,-1);
		return;
	}
	sprintf(outbuffer,"\n\n\rLägsta brev: %d  Högsta brev: %d  Antal att radera: %d\n\r",first,next-1,antal);
	puttekn(outbuffer,-1);
	puttekn("Rensar...\n\r",-1);
	for(x=first;x<first+antal;x++) {
		if(!(x%10)) {
			sprintf(outbuffer,"%d\r",x);
			puttekn(outbuffer,-1);
		}
		sprintf(filnamn,"NiKom:Users/%d/%d/%d.letter",inloggad/100,inloggad,x);
		if(!DeleteFile(filnamn)) {
			sprintf(outbuffer,"\n\rKunde inte radera %s\n\r",filnamn);
			puttekn(outbuffer,-1);
		}
	}
	sprintf(filnamn,"NiKom:Users/%d/%d/.firstletter",inloggad/100,inloggad);
	if(!(fh=Open(filnamn,MODE_NEWFILE))) {
		sprintf(outbuffer,"\n\rKunde inte öppna %s\n\r",filnamn);
		puttekn(outbuffer,-1);
		return;
	}
	sprintf(nrbuf,"%d",first+antal);
	if(Write(fh,nrbuf,strlen(nrbuf))==-1) {
		sprintf(outbuffer,"\n\rKunde inte skriva %s\n\r",filnamn);
		puttekn(outbuffer,-1);
	}
	Close(fh);
	sprintf(outbuffer,"\n\r%d brev raderade.\n\r",antal);
	puttekn(outbuffer,-1);
	if((first+antal) > Servermem->inne[nodnr].brevpek) Servermem->inne[nodnr].brevpek=first+antal;
}

int readletterheader(BPTR fh,struct ReadLetter *rl) {
	int len;
	char buffer[100];
	memset(rl,0,sizeof(struct ReadLetter));
	while(!rl->subject[0]) {
		if(!FGets(fh,buffer,99)) return(1);
		len=strlen(buffer);
		if(buffer[len-1]=='\n') buffer[len-1]=0;
		if(!strncmp(buffer,"System-ID:",10)) strcpy(rl->systemid,&buffer[11]);
		if(!strncmp(buffer,"From:",5)) strcpy(rl->from,&buffer[6]);
		if(!strncmp(buffer,"To:",3)) strcpy(rl->to,&buffer[4]);
		if(!strncmp(buffer,"Message-ID:",11)) strcpy(rl->messageid,&buffer[12]);
		if(!strncmp(buffer,"Reply:",6)) strcpy(rl->reply,&buffer[7]);
		if(!strncmp(buffer,"Date:",5)) strcpy(rl->date,&buffer[6]);
		if(!strncmp(buffer,"Subject:",8)) strcpy(rl->subject,&buffer[9]);
	}
	if(!rl->systemid[0]) return(1);
	return(0);
}
