#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Terminal.h"

#define ERROR	10
#define OK		0
#define EKO		1
#define EJEKO	0
#define KOM		1
#define EJKOM	0

extern struct System *Servermem;
extern char /* brevtxt[][81], */ outbuffer[],inmat[], crashmail;
extern int rad,nu_skrivs,nodnr,senast_text_typ,senast_text_nr,senast_text_mote, inloggad;
extern struct Header sparhead;
extern struct ReadLetter brevspar;
extern struct Library *FifoBase;

char yankbuffer[81],edxxcombuf[257];
int antkol=0,kolpos=0,edxxcommand=FALSE,edxxleft=0;

struct MinList edit_list;
struct EditLine *curline,*tmpline;

int edittext(char *filnamn) {
	int ret;
	freeeditlist();
	if(Servermem->inne[nodnr].flaggor & FULLSCREEN) ret=fulledit(filnamn);
	else ret=lineedit(filnamn);
	if(ret) freeeditlist();
	return(ret);
}

void quote(void) {
	struct Mote *motpek;
	if(senast_text_typ==TEXT || senast_text_typ==TEXT_FIDO) {
		if(!(motpek=getmotpek(senast_text_mote))) return;
		if(motpek->type==MOTE_FIDO) fidotextquote(motpek);
		else if(motpek->type == MOTE_ORGINAL) orgtextquote(motpek);
	} else if(senast_text_typ == BREV || senast_text_typ == BREV_FIDO) brevquote();
}

void fidotextquote(struct Mote *motpek) {
	struct FidoText *ft;
	struct Node *line;
	char filnamn[10],fullpath[100];
	sprintf(filnamn,"%d.msg",senast_text_nr - motpek->renumber_offset);
	strcpy(fullpath,motpek->dir);
	AddPart(fullpath,filnamn,99);
	ft = ReadFidoTextTags(fullpath,RFT_NoKludges,TRUE,
	                               RFT_NoSeenBy,TRUE,
	                               RFT_Quote,TRUE,
	                               RFT_LineLen,75,
	                               TAG_DONE);
	if(!ft) return;
	while(line=RemHead((struct List *)&ft->text)) AddTail((struct List *)&edit_list,line);
	FreeFidoText(ft);
}

void orgtextquote(struct Mote *motpek) { }

void brevquote(void) { }

int lineedit(char *filnamn) {
	int aktrad,getret;
	struct EditLine *el;
	char letmp[81],wrap[81];
	NewList((struct List *)&edit_list);
	if(!(Servermem->inne[nodnr].flaggor & INGENHELP)) {
		puttekn("Tryck Ctrl-Z på en tom rad för att spara texten.\n\r",-1);
		puttekn("Skriv !? på en tom rad för att få en hjälptext\n\n\r",-1);
	}
	if(filnamn) {
		lineloadfile(filnamn);
		linerenumber();
	}
	letmp[0]=0;
	for(;;) {
		wrap[0]=0;
		el=(struct EditLine *)edit_list.mlh_TailPred;
		if(el->line_node.mln_Pred) aktrad=el->number+1;
		else aktrad=1;
		getret=linegetline(letmp,wrap,aktrad);
		switch(getret) {
			case 0 :
				if(letmp[0]=='!') {
					letmp[0]=0;
					if(letmp[1]=='r' || letmp[1]=='R') linedelline(hittaefter(&letmp[1]));
					else if(letmp[1]=='i' || letmp[1]=='I') { if(lineinsert(atoi(hittaefter(&letmp[1])))) return(1); }
					else if(letmp[1]=='l' || letmp[1]=='L') lineread();
					else if(letmp[1]=='h' || letmp[1]=='H') lineread();
					else if(letmp[1]=='b' || letmp[1]=='B') return(2);
					else if(letmp[1]=='k' || letmp[1]=='K') return(2);
					else if(letmp[1]=='s' || letmp[1]=='S') return(0);
					else if(letmp[1]=='f' || letmp[1]=='F') lineflyttatext(hittaefter(&letmp[1]));
					else if(letmp[1]=='a' || letmp[1]=='A') lineaddera(hittaefter(&letmp[1]));
					else if(letmp[1]=='ä' || letmp[1]=='Ä') {
						if(letmp[2]=='n' || letmp[2]=='N') { if(linechange(atoi(hittaefter(&letmp[1])))) return(1); }
						else if(letmp[2]=='r' || letmp[2]=='R') linearende(hittaefter(letmp));
					}
					else if(letmp[1]=='d' || letmp[1]=='D') linedump();
					else if(letmp[1]=='c' || letmp[1]=='C') {
						if(letmp[2]=='i' || letmp[2]=='I') linequote();
						else if(letmp[2]=='r' || letmp[2]=='R') linecrash();
					}
					else if(letmp[1]=='?') sendfile("NiKom:Texter/EditorHelp.txt");
				} else {
					if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
						puttekn("\n\n\rKunde inte allokera en textrad\n\r",-1);
						return(0);
					}
					strcpy(el->text,letmp);
					el->number=aktrad++;
					AddTail((struct List *)&edit_list,(struct Node *)el);
					strcpy(letmp,wrap);
				}
				break;

			case 1 :
				return(1);
			case 2 :
				if(aktrad>1) {
					el=(struct EditLine *)RemTail((struct List *)&edit_list);
					strcpy(letmp,el->text);
					FreeMem(el,sizeof(struct EditLine));
					aktrad--;
				}
				break;
			case 3 :
				return(0);
		}
	}
}

int linegetline(char *str,char *wrap,int linenr) {
	int kol=strlen(str),cnt,x;
	UBYTE tecken;
	sprintf(outbuffer,"\r%2d: %s",linenr,str);
	putstring(outbuffer,-1,0);
	while(kol<75) {
		tecken=gettekn();
		if((tecken>31 && tecken <128) || (tecken>159 && tecken<=255)) {
			str[kol++]=tecken;
			str[kol]=0;
			eka(tecken);
		} else if(tecken==13) {
			putstring("\n",-1,0);
			return(0);
		} else if(tecken==10) {
			if(carrierdropped()) return(1);
			putstring("\n",-1,0);
			return(0);
		} else if(tecken==8) {
			if(kol==0) return(2);
			else {
				str[--kol]=0;
				putstring("\b \b",-1,0);
			}
		} else if(tecken==9 && kol<66) {
			cnt=8-(kol%8);
			for(x=0;x<cnt;x++) {
				str[kol++]=' ';
				eka(' ');
			}
		} else if(tecken==26 && kol==0) {
			putstring("!Spara\r\n\n",-1,0);
			return(3);
		}
	}
	if(!wrap) return(0);
	cnt=kol+1;
	while(str[--cnt]!=32 && cnt>0);
	if(cnt>0) {
		str[cnt]=0;
		strcpy(wrap,&str[cnt+1]);
		for(x=cnt+1;x<75;x++) putstring("\b \b",-1,0);
	} else {
		wrap[0]=str[74];
		str[74]=0;
		wrap[1]=0;
		putstring("\b \b",-1,0);
	}
	putstring("\n\r",-1,0);
	return(0);
}

void linerenumber(void) {
	struct EditLine *el;
	int x=1;
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) el->number=x++;
}

void lineloadfile(char *filnamn) {
	FILE *fp;
	struct EditLine *el;
	char loadbuff[81];
	if(!(fp=fopen(filnamn,"r"))) return;
	while(fgets(loadbuff,77,fp)) {
		loadbuff[strlen(loadbuff)-1]=0;
		if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
			puttekn("\n\n\rKunde inte allokera en textrad\n\r",-1);
			fclose(fp);
			return;
		}
		strcpy(el->text,loadbuff);
		AddTail((struct List *)&edit_list,(struct Node *)el);
	}
	fclose(fp);
}

void linedelline(char *arg) {
	struct EditLine *el;
	int radnr=atoi(arg);
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) if(el->number==radnr) {
		Remove((struct Node *)el);
		FreeMem(el,sizeof(struct EditLine));
		linerenumber();
		break;
	}
}

int lineinsert(int before) {
	struct EditLine *el,*ela;
	int getret,highrad;
	char instmp[81]="";
	el=(struct EditLine *)edit_list.mlh_TailPred;
	if(el->line_node.mln_Pred) highrad=el->number+1;
	else {
		puttekn("\n\rFinns ingen rad att infoga framför.\n\r",-1);
		return(0);
	}
	if(before<1 || before>highrad) {
		puttekn("\n\rFinns ingen sådan rad!\n\r",-1);
		return(0);
	}
	getret=linegetline(instmp,NULL,before);
	if(getret==1) return(1);
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) if(el->number==before) break;
	el=(struct EditLine *)el->line_node.mln_Pred;
	if(!(ela=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
		puttekn("\n\n\rKunde inte allokera en textrad\n\r",-1);
		return(0);
	}
	strcpy(ela->text,instmp);
	Insert((struct List *)&edit_list,(struct Node *)ela,(struct Node *)el);
	linerenumber();
	return(0);
}

void lineread(void) {
	struct EditLine *el;
	int x=1;
	putstring("\n\n\r",-1,0);
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) {
		sprintf(outbuffer,"%2d: %s\n\r",x++,el->text);
		/* if(puttekn(outbuffer,-1)) break; */
		putstring(outbuffer,-1,0);
	}
}

void lineflyttatext(char *vart) {
	int motnr;
	if(nu_skrivs!=TEXT) {
		putstring("\r\nDu kan bara flytta texter, inte brev!\r\n",-1,0);
		return;
	}
	if((motnr=parsemot(vart))==-1) {
		putstring("\r\nFinns inget sådant möte!\r\n",-1,0);
		return;
	}
	if(motnr==-3) {
		putstring("\r\nSkriv : !flytta <mötesnamn>\r\n",-1,0);
		return;
	}
	if(!MayWriteConf(motnr, inloggad, &Servermem->inne[nodnr]) || !MayReplyConf(motnr, inloggad, &Servermem->inne[nodnr])) {
		putstring("\n\rDu har ingen rätt att flytta texten till det mötet.\n\r",-1,0);
		return;
	}
	sparhead.mote=motnr;
	sprintf(outbuffer,"\r\nFlyttar texten till mötet %s.\r\n",getmotnamn(motnr));
	putstring(outbuffer,-1,0);
}

void linearende(char *nytt) {
	if(nu_skrivs!=TEXT && nu_skrivs!=BREV) {
		putstring("\n\rKan inte ändra ärendet!\n\r",-1,0);
		return;
	}
	if(!nytt[0]) {
		putstring("\r\nSkriv: !ärende <nytt ärende>\r\n",-1,0);
		return;
	}
	if(nu_skrivs==TEXT) strncpy(sparhead.arende,nytt,40);
	else strncpy(brevspar.subject,nytt,40);
}

int ismotthere(int mot,char *motstr) {
	while(motstr[0]) {
		if(atoi(motstr)==mot) return(1);
		motstr=hittaefter(motstr);
	}
	return(0);
}


void lineaddera(char *vem) {
	int anv;
	char nrbuf[10];
	if(nu_skrivs!=BREV) {
		putstring("\n\rDu kan bara addera personer till brev!\n\r",-1,0);
		return;
	}
	if((anv=parsenamn(vem))==-1) {
		putstring("\n\rFinns ingen sådan användare!\n\r",-1,0);
		return;
	}
	if(anv==-3) {
		putstring("\n\rSkriv: !Addera <användare>\n\r",-1,0);
		return;
	}
	if(ismotthere(anv,brevspar.to)) {
		putstring("\n\rAnvändaren är redan mottagare av brevet!\n\r",-1,0);
		return;
	}
	sprintf(nrbuf," %d",anv);
	if(strlen(nrbuf)+strlen(brevspar.to)>98) {
		putstring("\n\rKan inte addera flera användare!\n\r",-1,0);
		return;
	}
	strcat(brevspar.to,nrbuf);
	sprintf(outbuffer,"\n\rAdderar %s\n\r",getusername(anv));
	putstring(outbuffer,-1,0);
}

int linechange(int foorad) {
	struct EditLine *el,*chel=NULL;
	int buffercnt=0,kol=0;
	char andrabuffer[80];
	UBYTE tecken;
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) if(el->number==foorad) chel=el;
	if(!chel) {
		putstring("\n\rRaden finns inte!\n\r",-1,0);
		return(0);
	}
	strcpy(andrabuffer,chel->text);
	sprintf(outbuffer,"\r\n%2d: ",foorad);
	putstring(outbuffer,-1,0);
	for(;;) {
		tecken=gettekn();
		if((tecken>31 && tecken <128) || (tecken>159 && tecken<=255)) {
			if(kol<75) {
				chel->text[kol++]=tecken;
				chel->text[kol]=0;
				eka(tecken);
			} else eka('\a');
		} else if(tecken==13) break;
		else if(tecken==10) {
			if (carrierdropped()) return(1);
		} else if(tecken==8) {
			if(kol==0) eka('\a');
			else {
				chel->text[--kol]=0;
				putstring("\b \b",-1,0);
			}
		} else if(tecken==6) {
			if(kol<75 && andrabuffer[buffercnt]) {
				eka(andrabuffer[buffercnt]);
				chel->text[kol++]=andrabuffer[buffercnt++];
				chel->text[kol]=0;
			} else eka('\a');
		} else if(tecken==18) {
			while(kol<75 && andrabuffer[buffercnt]) {
				eka(andrabuffer[buffercnt]);
				chel->text[kol++]=andrabuffer[buffercnt++];
			}
			chel->text[kol]=0;
		}
	}
	putstring("\n",-1,0);
	return(0);
}

int linedump(void) {
	UBYTE tecken,col=0;
	struct EditLine *el;
	if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
		puttekn("\n\n\rKunde inte allokera en textrad\n\r",-1);
		return(0);
	}
	AddTail((struct List *)&edit_list,(struct Node *)el);
	while((tecken=gettekn())!=3) {
		if((tecken>31 && tecken<127) || (tecken>159 && tecken<=255)) {
			el->text[col++]=tecken;
			if(col>75) {
				col=0;
				if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
					puttekn("\n\n\rKunde inte allokera en textrad\n\r",-1);
					return(0);
				}
				AddTail((struct List *)&edit_list,(struct Node *)el);
			}
		} else if(tecken==13) {
			col=0;
			if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
				puttekn("\n\n\rKunde inte allokera en textrad\n\r",-1);
				return(0);
			}
			AddTail((struct List *)&edit_list,(struct Node *)el);
		}
		else if(tecken==10) {
			if(carrierdropped()) return(1);
		}
	}
	linerenumber();
	return(0);
}

void linequote(void) {
	quote();
	linerenumber();
}

void linecrash(void) {
	if(nu_skrivs != BREV_FIDO) {
		puttekn("\n\rEndast Fido-brev kan skickas som crashmail.\n\r",-1);
		return;
	}
	if(Servermem->inne[nodnr].status < Servermem->fidodata.crashstatus) {
		puttekn("\n\rDu har inte rätt att skicka crashmail.\n\r",-1);
		return;
	}
	crashmail = TRUE;
	puttekn("\n\rBrevet skickas som crashmail.\n\r",-1);
}

/*********** Fullskärmseditorn **************/

int fulledit(char *filnamn) {
	UBYTE tecken;
	NewList((struct List *)&edit_list);
	yankbuffer[0]=0;
	if(!(Servermem->inne[nodnr].flaggor & INGENHELP)) {
		puttekn("\r\nTryck Ctrl-Z för att spara texten, Ctrl-C för att kasta bort den.\n\r",-1);
		puttekn("Ge kommandot 'Info Full' ute i KOM-systemet för en hjälptext\n\n\r",-1);
	}
	if(!(curline=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
		puttekn("\r\n\nKunde inte allokera en rad!\r\n",-1);
		return(2);
	}
	AddTail((struct List *)&edit_list,(struct Node *)curline);
	antkol=kolpos=0;
	if(filnamn) {
		fullloadtext(filnamn);
		fulldisplaytext();
	}
	for(;;)
	{
		tecken=gettekn();
		if((tecken>31 && tecken<127) || (tecken>159 && tecken<=255))
			fullvanlgtkn(tecken);
		else if(tecken==8) fullbackspace();
		else if(tecken==127) fulldelete();
		else if(tecken==9 && antkol<66) fulltab();
		else if(tecken==10) { if(carrierdropped()) return(1); else fullreturn(); }
		else if(tecken==17) { if(doedkmd()) return(1); }
		else if(tecken==1) fullctrla();
		else if(tecken==5) fullctrle();
		else if(tecken==11) fullctrlk();
		else if(tecken==12) fullctrll();
		else if(tecken==18) fullquote();
		else if(tecken==24) fullctrlx();
		else if(tecken==25) fullctrly();
		else if(tecken==26)
		{
			putstring("\r\n\nSparar!\r\n",-1,0);
			return(0);
		} else if(tecken==3) {
			putstring("\r\n\nKastar bort texten\r\n",-1,0);
			return(2);
		} else if(tecken=='\x9b' || (tecken=='\x1b' && ((tecken=gettekn())=='[' || tecken=='Ä')))
			fullansisekv();
		else if(tecken==13) fullreturn();
	}
}

void fullvanlgtkn(char tecken) {
	int cnt;
	if(antkol<MAXFULLEDITTKN-1) {
		if(kolpos!=antkol) putstring("\x1b\x5b\x31\x40",-1,0);
		eka(tecken);
		if(kolpos!=antkol) movmem(&curline->text[kolpos],&curline->text[kolpos+1],antkol-kolpos);
		curline->text[kolpos++]=tecken;
		antkol++;
	} else  {
		if(antkol!=kolpos) return;
		if(!(tmpline=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR)))
		{
			puttekn("\r\nKunde inte allokera en rad, avslutar editorn!\r\n",-1);
			return;
		}
		movmem(&curline->text[kolpos],&curline->text[kolpos+1],antkol-kolpos);
		curline->text[kolpos++]=tecken;
		antkol++;
		curline->text[kolpos]=0;
		cnt=antkol+1;
		while(curline->text[--cnt]!=32 && cnt>0);
		if(cnt>0) {
			sprintf(outbuffer,"\r\x1b\x5b%d\x43\x1b\x5b\x4b\n",cnt);
			putstring(outbuffer,-1,0);
			strcpy(tmpline->text,&curline->text[cnt+1]);
			memset(&curline->text[cnt],0,80-cnt);
			Insert((struct List *)&edit_list,(struct Node *)tmpline,(struct Node *)curline);
		} else {
			/* putstring("\b \b",-1,0); */
			/* eka('\n'); */
			putstring("\n",-1,0);
			Insert((struct List *)&edit_list,(struct Node *)tmpline,(struct Node *)curline);
			tmpline->text[0]=curline->text[MAXFULLEDITTKN-1];
			curline->text[MAXFULLEDITTKN-1]=0;
		}
		curline=tmpline;
		sprintf(outbuffer,"\x1b\x5b\x4c\r%s",curline->text);
		putstring(outbuffer,-1,0);
		antkol=kolpos=strlen(curline->text);
	}
}

void fullbackspace(void) {
	struct EditLine *prevline=(struct EditLine *)curline->line_node.mln_Pred;
	if(kolpos)
	{
		putstring("\x1b\x5b\x44\x1b\x5b\x50",-1,0);
		movmem(&curline->text[kolpos],&curline->text[kolpos-1],antkol-kolpos+1);
		kolpos--;
		antkol--;
	} else if(prevline->line_node.mln_Pred!=NULL) {
		if(strlen(curline->text)+strlen(prevline->text)<MAXFULLEDITTKN) {
			kolpos=strlen(prevline->text);
			strcat(prevline->text,curline->text);
			putstring("\x1b\x5b\x4d",-1,0);
			Remove((struct Node *)curline);
			FreeMem(curline,sizeof(struct EditLine));
			sprintf(outbuffer,"\x1b\x5b\x41\r%s",prevline->text);
			putstring(outbuffer,-1,0);
			curline=prevline;
			antkol=strlen(curline->text);
			if(kolpos) {
				sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
				putstring(outbuffer,-1,0);
			} else eka('\r');
		}
	}
}

void fulldelete(void) {
	struct EditLine *succline=(struct EditLine *)curline->line_node.mln_Succ;
	if(kolpos!=antkol) {
		putstring("\x1b\x5b\x50",-1,0);
		movmem(&curline->text[kolpos+1],&curline->text[kolpos],antkol-kolpos);
		antkol--;
	} else if(succline->line_node.mln_Succ!=NULL) {
		if(strlen(curline->text)+strlen(succline->text)<MAXFULLEDITTKN-1) {
			strcat(curline->text,succline->text);
			putstring("\n",-1,0);
			Remove((struct Node *)succline);
			FreeMem(succline,sizeof(struct EditLine));
			putstring("\x1b\x5b\x4d",-1,0);
			sprintf(outbuffer,"\x1b\x5b\x41\r%s",curline->text);
			putstring(outbuffer,-1,0);
			antkol=strlen(curline->text);
			if(kolpos) {
				sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
				putstring(outbuffer,-1,0);
			} else eka('\r');
		}
	}
}

void fulltab(void) {
	int cnt=8-(kolpos%8),x;
	sprintf(outbuffer,"\x1b\x5b%d\x40",cnt);
	putstring(outbuffer,-1,0);
	movmem(&curline->text[kolpos],&curline->text[kolpos+cnt],antkol-kolpos);
	for(x=0;x<cnt;x++) {
		curline->text[kolpos+x]=' ';
		eka(' ');
	}
	kolpos+=cnt;
	antkol+=cnt;
}

void fullctrla(void) {
	kolpos=0;
	eka('\r');
}

void fullctrle(void) {
	int oldpos=kolpos,move;
	kolpos=strlen(curline->text);
	move=kolpos-oldpos;
	if(move) {
		sprintf(outbuffer,"\x1b\x5b%d\x43",move);
		putstring(outbuffer,-1,0);
	}
}

void fullctrlk(void) {
	struct EditLine *succline=(struct EditLine *)curline->line_node.mln_Succ;
	if(antkol!=kolpos) {
		putstring("\x1b\x5b\x4b",-1,0);
		strcpy(yankbuffer,&curline->text[kolpos]);
		memset(&curline->text[kolpos],0,80-kolpos);
		antkol=kolpos;
	} else if(succline->line_node.mln_Succ!=NULL) {
		if(strlen(curline->text)+strlen(succline->text)<MAXFULLEDITTKN-1) {
			strcat(curline->text,succline->text);
			putstring("\n",-1,0);
			Remove((struct Node *)succline);
			FreeMem(succline,sizeof(struct EditLine));
			putstring("\x1b\x5b\x4d",-1,0);
			sprintf(outbuffer,"\x1b\x5b\x41\r%s",curline->text);
			putstring(outbuffer,-1,0);
			antkol=strlen(curline->text);
			if(kolpos) {
				sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
				putstring(outbuffer,-1,0);
			} else eka('\r');
		}
	}
}

void fullctrll(void) {
	struct EditLine *prevline=(struct EditLine *)curline->line_node.mln_Pred,
		*succline=(struct EditLine *)curline->line_node.mln_Succ;
	eka('\f');
	sprintf(outbuffer,"%s\n\r",prevline->text);
	putstring(outbuffer,-1,0);
	sprintf(outbuffer,"%s\n\r",curline->text);
	putstring(outbuffer,-1,0);
	sprintf(outbuffer,"%s\x1b\x5b\x41\r\x1b\x5b%d\x43",succline->text,kolpos);
	putstring(outbuffer,-1,0);
}

void fullctrlx(void) {
	struct EditLine *succline=(struct EditLine *)curline->line_node.mln_Succ;
	if(succline->line_node.mln_Succ!=NULL) {
		Remove((struct Node *)curline);
		strcpy(yankbuffer,curline->text);
		FreeMem(curline,sizeof(struct EditLine));
		putstring("\x1b\x5b\x4d",-1,0);
		curline=succline;
		antkol=strlen(curline->text);
		kolpos=0;
		eka('\r');
	} else {
		strcpy(yankbuffer,curline->text);
		memset(curline->text,0,80);
		antkol=kolpos=0;
		puttekn("\r\x1b\x5b\x4b",-1);
	}
}

void fullctrly(void) {
	if(strlen(curline->text)+strlen(yankbuffer)<MAXFULLEDITTKN-1) {
		movmem(&curline->text[kolpos],&curline->text[kolpos+strlen(yankbuffer)],antkol-kolpos);
		movmem(yankbuffer,&curline->text[kolpos],strlen(yankbuffer));
		sprintf(outbuffer,"\r%s",curline->text);
		putstring(outbuffer,-1,0);
		antkol=strlen(curline->text);
		if(kolpos) {
			sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
			putstring(outbuffer,-1,0);
		} else eka('\r');
	}
}

void fullansisekv(void) {
	struct EditLine *prevline=(struct EditLine *)curline->line_node.mln_Pred,
		*succline=(struct EditLine *)curline->line_node.mln_Succ;
	UBYTE tecken;
	tecken=gettekn();
	if(tecken=='\x44') {
		if(kolpos) {
			putstring("\x1b\x5b\x44",-1,0);
			kolpos--;
		} else if(prevline->line_node.mln_Pred) {
			putstring("\x1b\x5b\x41",-1,0);
			curline=prevline;
			sprintf(outbuffer,"\r%s",curline->text);
			putstring(outbuffer,-1,0);
			kolpos=strlen(curline->text);
			if(kolpos) {
				sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
				putstring(outbuffer,-1,0);
			} else eka('\r');
			antkol=strlen(curline->text);
		}
	} else if(tecken=='\x43') {
		if(kolpos<antkol) {
			putstring("\x1b\x5b\x43",-1,0);
			kolpos++;
		} else if(succline->line_node.mln_Succ) {
			curline=succline;
			sprintf(outbuffer,"\n\r%s",curline->text);
			putstring(outbuffer,-1,0);
			kolpos=0;
			eka('\r');
			antkol=strlen(curline->text);
		}
	} else if(tecken=='\x41' && prevline->line_node.mln_Pred) {
		putstring("\x1b\x5b\x41",-1,0);
		curline=prevline;
		sprintf(outbuffer,"\r%s",curline->text);
		putstring(outbuffer,-1,0);
		if(kolpos>strlen(curline->text)) kolpos=strlen(curline->text);
		antkol=strlen(curline->text);
		if(kolpos) {
			sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
			putstring(outbuffer,-1,0);
		} else eka('\r');
	} else if(tecken=='\x42' && succline->line_node.mln_Succ) {
		curline=succline;
		sprintf(outbuffer,"\n\r%s",curline->text);
		putstring(outbuffer,-1,0);
		if(kolpos>strlen(curline->text)) kolpos=strlen(curline->text);
		antkol=strlen(curline->text);
		if(kolpos) {
			sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
			putstring(outbuffer,-1,0);
		} else eka('\r');
	} else if(tecken=='\x20') fullshiftedcursor();
}

void fullshiftedcursor(void) {
	int cnt;
	UBYTE tecken=gettekn();
	if(tecken=='\x41') {
		if(!kolpos) return;
		for(cnt=kolpos;cnt;cnt--)
			if(curline->text[cnt]!=' ' && curline->text[cnt-1]==' ' && cnt!=kolpos) break;
		sprintf(outbuffer,"\x1b\x5b%d\x44",kolpos-cnt);
		putstring(outbuffer,-1,0);
		kolpos=cnt;
	} else if(tecken=='\x40') {
		if(kolpos==antkol) return;
		for(cnt=kolpos;cnt<antkol;cnt++)
			if(cnt && curline->text[cnt]!=' ' && curline->text[cnt-1]==' ' && cnt!=kolpos) break;
		sprintf(outbuffer,"\x1b\x5b%d\x43",cnt-kolpos);
		putstring(outbuffer,-1,0);
		kolpos=cnt;
	}
}

void fullreturn(void)
{
	if(!(tmpline=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR)))
	{
		puttekn("\r\nKunde inte allokera en rad. Avsluta editorn!\r\n",-1);
		return;
	}
	if(kolpos==antkol)
	{
		putstring("\n",-1,0);
		Insert((struct List *)&edit_list,(struct Node *)tmpline,(struct Node *)curline);
		curline=tmpline;
		putstring("\x1b\x5b\x4c\r",-1,0);
		antkol=kolpos=0;
	}
	else
	{
		putstring("\x1b\x5b\x4b\n",-1,0);
		Insert((struct List *)&edit_list,(struct Node *)tmpline,(struct Node *)curline);
		strcpy(tmpline->text,&curline->text[kolpos]);
		memset(&curline->text[kolpos],0,80-kolpos);
		kolpos=0;
		curline=tmpline;
		antkol=strlen(curline->text);
		sprintf(outbuffer,"\x1b\x5b\x4c\r%s\r",curline->text);
		putstring(outbuffer,-1,0);
	}
}

int doedkmd(void) {
	putstring("\r\x1b\x5b\x4b\x1b\x5b\x31\x6d Kommando: ",-1,0);
	if(getstring(EKO,45,NULL)) return(1);
	if(inmat[0]=='f' || inmat[0]=='F') fullflyttatext(hittaefter(inmat));
	else if(inmat[0]=='a' || inmat[0]=='A') fulladdera(hittaefter(inmat));
	else if(inmat[0]=='d' || inmat[0]=='D') { if(fulldumpa()) return(1); }
	else if(inmat[0]=='ä' || inmat[0]=='Ä') fullarende(hittaefter(inmat));
	else if(inmat[0]=='c' || inmat[0]=='C') fullcrash();
	else if(inmat[0]=='?' || inmat[0]=='h' || inmat[0]=='H') {
		putstring("\rCtrl-Z=Spara, Ctrl-C=Avbryt, 'Info Full' för mer hjälp. <RETURN>",-1,0);
		gettekn();
	} else {
		putstring("\rFelaktigt kommando! <RETURN>",-1,0);
		gettekn();
	}
	putstring("\r\x1b\x5b\x4b\x1b\x5b\x30\x6d",-1,0);
	sprintf(outbuffer,"\r%s",curline->text);
	putstring(outbuffer,-1,0);
	if(kolpos) {
		sprintf(outbuffer,"\r\x1b\x5b%d\x43",kolpos);
		putstring(outbuffer,-1,0);
	} else eka('\r');
	return(0);
}

void fulladdera(char *vem) {
	int anv;
	char nrbuf[10];
	if(nu_skrivs!=BREV) {
		putstring("\rDu kan bara addera personer till brev! <RETURN>",-1,0);
		gettekn();
		return;
	}
	if((anv=parsenamn(vem))==-1) {
		putstring("\rFinns ingen sådan användare! <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(anv==-3) {
		putstring("\rSkriv: Addera <användare> <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(ismotthere(anv,brevspar.to)) {
		putstring("\rAnvändaren är redan mottagare av brevet! <RETURN>",-1,0);
		gettekn();
		return;
	}
	sprintf(nrbuf," %d",anv);
	if(strlen(nrbuf)+strlen(brevspar.to)>98) {
		putstring("\rKan inte addera flera användare! <RETURN>",-1,0);
		gettekn();
		return;
	}
	strcat(brevspar.to,nrbuf);
	sprintf(outbuffer,"\rAdderar %s <RETURN>",getusername(anv));
	putstring(outbuffer,-1,0);
	gettekn();
}

void fullflyttatext(char *vart) {
	int motnr;
	if(nu_skrivs!=TEXT) {
		putstring("\rDu kan bara flytta texter i möten! <RETURN>",-1,0);
		gettekn();
		return;
	}
	if((motnr=parsemot(vart))==-1) {
		putstring("\rFinns inget sådant möte! <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(motnr==-3) {
		putstring("\rSkriv : flytta <mötesnamn> <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(!MayWriteConf(motnr, inloggad, &Servermem->inne[nodnr]) || !MayReplyConf(motnr, inloggad, &Servermem->inne[nodnr])) {
		putstring("\rDu har ingen rätt att flytta texten till det mötet. <RETURN>",-1,0);
		return;
	}
	sparhead.mote=motnr;
	sprintf(outbuffer,"\rFlyttar texten till mötet %s. <RETURN>",getmotnamn(motnr));
	putstring(outbuffer,-1,0);
	gettekn();
}

void fullarende(char *nytt) {
	if(nu_skrivs!=TEXT && nu_skrivs!=BREV) {
		putstring("\rKan inte ändra ärende! <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(!nytt[0]) {
		putstring("\rSkriv: Ärende <nytt ärende>  <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(nu_skrivs==TEXT) strncpy(sparhead.arende,nytt,40);
	else strncpy(brevspar.subject,nytt,40);
}

int fulldumpa(void) {
	UBYTE tecken,col=0;
	if(antkol && fullnewline()) return(0);
	putstring("\r\n\x1b\x5b\x4b\x1b\x5b\x31\x6d Ok, tar emot text utan eko. Tryck Ctrl-C för att återgå.\r\n",-1,0);
	while((tecken=gettekn())!=3) {
		if((tecken>31 && tecken<127) || (tecken>159 && tecken<=255)) {
			curline->text[col++]=tecken;
			if(col>MAXFULLEDITTKN-1) { col=0; if(fullnewline()) return(0); }
		} else if(tecken==13) { col=0; if(fullnewline()) return(0); }
		else if(tecken==10) {
			if(carrierdropped()) return(1);
		}
	}
	putstring("\r\x1b\x5b\x4b\x1b\x5b\x30\x6d",-1,0);
	if(fullnewline()) return(0);
	antkol=kolpos=0;
	return(0);
}

int fullnewline(void) {
	if(!(tmpline=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR))) {
		puttekn("\r\nKunde inte allokera en rad. Avsluta editorn\r\n",-1);
		return(1);
	}
	Insert((struct List *)&edit_list,(struct Node *)tmpline,(struct Node *)curline);
	curline=tmpline;
	return(0);
}

int fullloadtext(char *filnamn) {
	FILE *fp;
	if(!(fp=fopen(filnamn,"r"))) return(0);
	while(fgets(curline->text,MAXFULLEDITTKN+1,fp)) {
		if(strlen(curline->text) > MAXFULLEDITTKN)
		{
			curline->text[strlen(curline->text)-1]=0;
		}
		else
		{
			if(curline->text[strlen(curline->text)-1] == '\n')
				curline->text[strlen(curline->text)-1]=0;
		}
		if(fullnewline()) {
			fclose(fp);
			return(0);
		}
	}
	fclose(fp);
}

void fulldisplaytext(void) {
	struct EditLine *pek;

	for(pek=(struct EditLine *)edit_list.mlh_Head;pek->line_node.mln_Succ;pek=(struct EditLine *)pek->line_node.mln_Succ) {
		/* puttekn("\r\n",-1);
		puttekn(pek->text,-1); */
		putstring("\r\n",-1,0);
		putstring(pek->text,-1,0);
	}
	eka('\r');
	curline=(struct EditLine *)edit_list.mlh_TailPred;
	antkol=strlen(curline->text);
	kolpos=0;
}

void fullquote(void) {
	putstring("\r\x1b\x5b\x4a",-1,0);
	quote();
	fulldisplaytext();
}

void fullcrash(void) {
	if(nu_skrivs != BREV_FIDO) {
		putstring("\rEndast Fido-brev kan skickas som crashmail. <RETURN>",-1,0);
		gettekn();
		return;
	}
	if(Servermem->inne[nodnr].status < Servermem->fidodata.crashstatus) {
		putstring("\rDu har inte rätt att skicka crashmail. <RETURN>",-1,0);
		gettekn();
		return;
	}
	crashmail = TRUE;
	putstring("\rBrevet skickas som crashmail. <RETURN>",-1,0);
	gettekn();
}


/************* Externa editorer ******************/
/*

#define FIFOEVENT_FROMUSER 1
#define FIFOEVENT_FROMFIFO 2
#define FIFOEVENT_CLOSEW   4

int doedxxcommand(void) {
	char svar[256];
	switch(edxxcommand) {
		case 1 :


int handleedxxinput(unsigned char *buffer,int cnt) {
	int x,comlen,ret=FALSE;
	if(edxxcommand) {
		if(cnt>=edxxleft) {
			strncat(edxxcombuf,buffer,left);
			buffer+=left;
			cnt-=left;
			left=0;
		} else {
			strncat(edxxcombuf,buffer,cnt);
			buffer+=cnt;
			left-=cnt;
			cnt=0;
		}
		if(!left) {
			ret=doedxxcommand();
			edxxcommand=0;
		}
	} else {
		for(x=0;x<cnt && buffer[x]!=0xff;x++);
		if(x==cnt) {
			putstring(buffer,cnt,0);
		} else {
			edxxcommand=buffer[x+1];
			comlen=buffer[x+2];
			x+=3;
			if(comlen<=cnt-x) {
				memcpy(edxxcombuf,&buffer[x],comlen);
				edxxcombuf[comlen]=0;
				ret=doedxxcommand();
				edxxcommand=0;
				if(cnt-comlen-x) putstring(&buffer[x+comlen],cnt-comlen-x,0);
			} else {
				memcpy(eddxxcombuf,&buffer[x],cnt-x);
				edxxcombuf[cnt-x]=0;
			}
		}
	}
}

int edxx(char *filnamn) {
	struct MsgPort *fifoport;
	void *fiforead, *fifowrite;
	BPTR fh;
	int avail=0,ended=FALSE,event;
	char fifonamn[40],*buffer,userchar;
	struct Message fiforeadmess,fifowritemess,*backmess;
	fiforeadmess.mn_Node.ln_Type=NT_MESSAGE;
	fiforeadmess.mn_Length=sizeof(struct Message);
	fifowritemess.mn_Node.ln_Type=NT_MESSAGE;
	fifowritemess.mn_Length=sizeof(struct Message);
	if(!(FifoBase=OpenLibrary("fifo.library",0L))) {
		puttekn("\n\n\rKunde inte öppna fifo.library\n\r",-1);
		return(0);
	}
	if(!(fifoport=CreateMsgPort())) {
		puttekn("\n\n\rKunde inte skapa en MsgPort\n\r",-1);
		CloseLibrary(FifoBase);
		return(0);
	}
	fiforeadmess.mn_ReplyPort=fifoport;
	fifowritemess.mn_ReplyPort=fifoport;
	sprintf(fifonamn,"NiKomFifo%d_s",nodnr);
	if(!(fiforead=OpenFifo(fifonamn,2048,FIFOF_READ|FIFOF_NORMAL|FIFOF_NBIO))) {
		puttekn("\n\n\rKunde inte öppna fiforead\n\r",-1);
		DeleteMsgPort(fifoport);
		CloseLibrary(FifoBase);
		return(0);
	}
	sprintf(fifonamn,"NiKomFifo%d_m",nodnr);
	if(!(fifowrite=OpenFifo(fifonamn,2048,FIFOF_WRITE|FIFOF_NORMAL))) {
		puttekn("\n\n\rKunde inte öppna fiforead\n\r",-1);
		CloseFifo(fiforead,FIFOF_EOF);
		DeleteMsgPort(fifoport);
		CloseLibrary(FifoBase);
		return(0);
	}
	sprintf(fifonamn,"NiKom:Edxx NiKomFifo%d %s",nodnr,filename);
	if(SystemTags(fifonamn,SYS_Asynch,TRUE,
	                      SYS_UserShell,TRUE)==-1) {
		puttekn("\n\n\rKunde inte utföra kommandot\n\r",-1);
		CloseFifo(fifowrite,FIFOF_EOF);
		CloseFifo(fiforead,FIFOF_EOF);
		DeleteMsgPort(fifoport);
		CloseLibrary(FifoBase);
		return(0);
	}
	abortinactive();
	RequestFifo(fiforead,&fiforeadmess,FREQ_RPEND);
	while(!ended) {
		event=getfifoevent(fifoport,&userchar);
		if(event & FIFOEVENT_FROMFIFO) {
			while(avail=ReadFifo(fiforead,&buffer,avail)) {
				if(avail==-1) {
					ended=2;
					break;
				} else ended=handleedxxinput(buffer,avail);
			}
			RequestFifo(fiforead,&fiforeadmess,FREQ_RPEND);
		}
		if(event & FIFOEVENT_FROMUSER) {
			while(WriteFifo(fifowrite,&userchar,1)==-2) {
				RequestFifo(fiforead,&fifowritemess,FREQ_WAVAIL);
				WaitPort(fifoport);
				while((backmess=GetMsg(fifoport))!=&fifowritemess) if(backmess==&fiforeadmess) {
					while(avail=ReadFifo(fiforead,&buffer,avail)) {
						if(avail==-1) {
							ended=2;
							break;
						} else ended=handleedxxinput(buffer,avail);
					}
					RequestFifo(fiforead,&fiforeadmess,FREQ_RPEND);
					WaitPort(fifoport);
				}
			}
		}
		if(event & FIFOEVENT_CLOSEW) {
			RequestFifo(fiforead,&fiforeadmess,FREQ_ABORT);
			WaitPort(fifoport);
			GetMsg(fifoport);
			CloseFifo(fifowrite,FIFOF_EOF);
			CloseFifo(fiforead,FIFOF_EOF);
			DeleteMsgPort(fifoport);
			CloseLibrary(FifoBase);
			cleanup(OK,"");
		}
		if(carrierdropped()) ended=2;
	}
	RequestFifo(fiforead,&fiforeadmess,FREQ_ABORT);
	WaitPort(fifoport);
	GetMsg(fifoport);
	CloseFifo(fifowrite,FIFOF_EOF);
	CloseFifo(fiforead,FIFOF_EOF);
	DeleteMsgPort(fifoport);
	CloseLibrary(FifoBase);
	if(carrierdropped()) return(1);
	updateinactive();
	return(0);
}
*/
