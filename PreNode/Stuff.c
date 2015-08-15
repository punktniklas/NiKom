#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include "NiKomstr.h"
#include "NiKomLib.h"
#include "PreNodeFuncs.h"

#define ERROR	10

#define OK		0
#define EKO		1
#define EJEKO	0
#define KOM		1
#define EJKOM	0

extern struct System *Servermem;
extern int radcnt,nodnr,inloggad;
extern char outbuffer[],inmat[],reggadnamn[];

char usernamebuf[50];

char *hittaefter(strang)
char *strang;
{
	int test=TRUE;
	while(test) {
		test=FALSE;
		while(*(++strang)!=' ' && *strang!=0);
		if(*(strang+1)==' ') test=TRUE;
	}
	return(*strang==0 ? strang : ++strang);
}

void logevent(char *str) {
	BPTR fh;
	long tid;
	struct tm *ts;
	char logbuf[150];
	time(&tid);
	ts=localtime(&tid);
	sprintf(logbuf,"%4d%02d%02d %02d:%02d - %s\n", ts->tm_year + 1900,
                ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min,str);
	if(!(fh=Open(Servermem->cfg.logfile,MODE_OLDFILE))) {
		if(!(fh=Open(Servermem->cfg.logfile,MODE_NEWFILE))) {
			printf("Kunde inte öppna logfilen!\n");
			return;
		}
	}
	if(Seek(fh,0,OFFSET_END)==-1) {
		printf("Fel vid sökandet i logfilen!\n");
		Close(fh);
		return;
	}
	if(Write(fh,logbuf,strlen(logbuf))==-1) printf("Kunde inte skriva till logfilen!\n");
	Close(fh);
}

int jaellernej(char val1,char val2,int defulle) {
	UBYTE tk;
	radcnt=0;
	if(defulle==1) {
		if((val1>='a' && val1<='z') || val1=='å' || val1=='ä' || val1=='ö') val1-=32;
		if((val2>='A' && val2<='Z') || val2=='Å' || val2=='Ä' || val2=='Ö') val2+=32;
	} else {
		if((val1>='A' && val1<='Z') || val1=='Å' || val1=='Ä' || val1=='Ö') val1+=32;
		if((val2>='a' && val2<='z') || val2=='å' || val2=='ä' || val2=='ö') val2-=32;
	}
	sprintf(outbuffer,"(%c/%c) ",val1,val2);
	puttekn(outbuffer,-1);
	for(;;) {
		tk=gettekn();
		if(tk!=val1 && tk!=val1-32 && tk!=val1+32 && tk!=val2 && tk!=val2-32 && tk!=val2+32 && tk!=13 && tk!=10) continue;
		if(tk==val1 || tk==val1-32 || tk==val1+32 || ((tk==13 || tk==10) && defulle==1)) return(TRUE);
		if(tk==val2 || tk==val2-32 || tk==val2+32 || ((tk==13 || tk==10) && defulle==2)) return(FALSE);
	}
}

int speciallogin(char bokstav) {
	struct SpecialLogin *pek;
	for(pek=(struct SpecialLogin *)Servermem->special_login.mlh_Head;pek->login_node.mln_Succ;pek=(struct SpecialLogin *)pek->login_node.mln_Succ)
		if(pek->bokstav==bokstav) return(pek->rexxprg);
	return(0);
}

int nyanv(void) {
	BPTR lock,fh;
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	long tid;
	int going=TRUE,x=0;
	struct ShortUser *allokpek;
	char dirnamn[100],filnamn[40];
	UBYTE tillftkn;
	if(bytteckenset()) return(0);
	puttekn("\r\n\n",-1);
	Servermem->inne[nodnr].rader=Servermem->cfg.defaultrader;
	sendfile("NiKom:Texter/NyAnv.txt");
	while(going) {
		do {
			puttekn("\r\n\nNamn: ",-1);
			if(getstring(EKO,40,NULL)) return(0);
		} while(!inmat[0]);
		if(parsenamn(inmat)!=-1) puttekn("\r\n\nNamnet finns redan!\r\n\n",-1);
		else going=FALSE;
	}
	strncpy(Servermem->inne[nodnr].namn,inmat,40);
	puttekn("\r\nGatuadress: ",-1);
	if(getstring(EKO,40,NULL)) return(0);
	strncpy(Servermem->inne[nodnr].gata,inmat,40);
	puttekn("\r\nPostadress: ",-1);
	if(getstring(EKO,40,NULL)) return(0);
	strncpy(Servermem->inne[nodnr].postadress,inmat,40);
	puttekn("\r\nLand :-): ",-1);
	if(getstring(EKO,40,NULL)) return(0);
	strncpy(Servermem->inne[nodnr].land,inmat,40);
	puttekn("\r\nTelefon: ",-1);
	if(getstring(EKO,20,NULL)) return(0);
	strncpy(Servermem->inne[nodnr].telefon,inmat,20);
	puttekn("\r\nAnnan info: ",-1);
	if(getstring(EKO,59,NULL)) return(0);
	strncpy(Servermem->inne[nodnr].annan_info,inmat,60);
	do {
		puttekn("\r\nLösenord: ",-1);
		if(getstring(EKO,15,NULL)) return(0);
	} while(!inmat[0]);
		strncpy(Servermem->inne[nodnr].losen,inmat,15);
	puttekn("\r\nÖnskad prompt (-->) : ",-1);
	if(getstring(EKO,5,NULL)) return(0);
	if(inmat[0]) strncpy(Servermem->inne[nodnr].prompt,inmat,5);
	else strcpy(Servermem->inne[nodnr].prompt,"-->");
	Servermem->inne[nodnr].tot_tid=0L;
	time(&tid);
	Servermem->inne[nodnr].forst_in=tid;
	Servermem->inne[nodnr].senast_in=0L;
	Servermem->inne[nodnr].read=0L;
	Servermem->inne[nodnr].skrivit=0L;
	Servermem->inne[nodnr].flaggor=Servermem->cfg.defaultflags;
	Servermem->inne[nodnr].upload=0;
	Servermem->inne[nodnr].download=0;
	Servermem->inne[nodnr].loggin=0;
	Servermem->inne[nodnr].grupper=0L;
	Servermem->inne[nodnr].defarea=0L;
	Servermem->inne[nodnr].shell = 0;
	Servermem->inne[nodnr].status=Servermem->cfg.defaultstatus;
	Servermem->inne[nodnr].protokoll=Servermem->cfg.defaultprotokoll;
	Servermem->inne[nodnr].brevpek=0;
	memset((void *)Servermem->inne[nodnr].motmed,0,MAXMOTE/8);
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
		if(motpek->status & (SKRIVSTYRT | SLUTET)) BAMCLEAR(Servermem->inne[nodnr].motratt,motpek->nummer);
		else BAMSET(Servermem->inne[nodnr].motratt,motpek->nummer);
	}
        InitUnreadTexts(&Servermem->unreadTexts[nodnr]);
	sprintf(outbuffer,"\r\n\nNamn :        %s\r\n",Servermem->inne[nodnr].namn);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Gatuadress :  %s\r\n",Servermem->inne[nodnr].gata);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Postadress :  %s\r\n",Servermem->inne[nodnr].postadress);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Land :        %s\r\n",Servermem->inne[nodnr].land);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Telefon :     %s\r\n",Servermem->inne[nodnr].telefon);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Annan info :  %s\r\n",Servermem->inne[nodnr].annan_info);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Lösenord :    %s\r\n",Servermem->inne[nodnr].losen);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Prompt :      %s\r\n",Servermem->inne[nodnr].prompt);
	puttekn(outbuffer,-1);
	puttekn("\r\nStämmer detta? (J/n) ",-1);
	while((tillftkn=gettekn())=='n' || tillftkn=='N') {
		puttekn("Nej",-1);
		going=TRUE;
		while(going) {
			sprintf(outbuffer,"\r\nNamn : (%s) ",Servermem->inne[nodnr].namn);
			puttekn(outbuffer,-1);
			if(getstring(EKO,40,NULL)) return(0);
			if(inmat[0]) {
				if(parsenamn(inmat)!=-1) puttekn("\r\n\nNamnet finns redan!\r\n",-1);
				else {
					going=FALSE;
					strncpy(Servermem->inne[nodnr].namn,inmat,40);
				}
			} else going=FALSE;
		}
		sprintf(outbuffer,"\r\nGatuadress : (%s) ",Servermem->inne[nodnr].gata);
		puttekn(outbuffer,-1);
		if(getstring(EKO,40,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].gata,inmat,40);
		sprintf(outbuffer,"\r\nPostadress : (%s) ",Servermem->inne[nodnr].postadress);
		puttekn(outbuffer,-1);
		if(getstring(EKO,40,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].postadress,inmat,40);
		sprintf(outbuffer,"\r\nLand : (%s) ",Servermem->inne[nodnr].land);
		puttekn(outbuffer,-1);
		if(getstring(EKO,40,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].land,inmat,40);
		sprintf(outbuffer,"\r\nTelefon : (%s) ",Servermem->inne[nodnr].telefon);
		puttekn(outbuffer,-1);
		if(getstring(EKO,20,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].telefon,inmat,20);
		sprintf(outbuffer,"\r\nAnnan info : (%s) ",Servermem->inne[nodnr].annan_info);
		puttekn(outbuffer,-1);
		if(getstring(EKO,59,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].annan_info,inmat,60);
		sprintf(outbuffer,"\r\nLösenord : (%s) ",Servermem->inne[nodnr].losen);
		puttekn(outbuffer,-1);
		if(getstring(EKO,15,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].losen,inmat,15);
		sprintf(outbuffer,"\r\nPrompt : (%s) ",Servermem->inne[nodnr].prompt);
		puttekn(outbuffer,-1);
		if(getstring(EKO,5,NULL)) return(0);
		if(inmat[0]) strncpy(Servermem->inne[nodnr].prompt,inmat,5);
		puttekn("\r\n\nStämmer allt nu? (J/n) ",-1);
	}
	if(Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS)
		strcpy(Servermem->inne[nodnr].losen, CryptPassword(Servermem->inne[nodnr].losen));

	puttekn("Ja",-1);
	x=((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer+1;
	if(!(allokpek=(struct ShortUser *)AllocMem(sizeof(struct ShortUser),MEMF_CLEAR | MEMF_PUBLIC))) {
		puttekn("\r\n\nKunde inte allokera en ShortUser-struktur\r\n",-1);
		return(2);
	}
	strcpy(allokpek->namn,Servermem->inne[nodnr].namn);
	allokpek->nummer=x;
	allokpek->status=Servermem->inne[nodnr].status;
	AddTail((struct List *)&Servermem->user_list,(struct Node *)allokpek);
	sprintf(dirnamn,"NiKom:Users/%d",x/100);
	if(!(lock=Lock(dirnamn,ACCESS_READ)))
		if(!(lock=CreateDir(dirnamn))) {
			printf("Kunde inte skapa %s\n",dirnamn);
			return(2);
		}
	UnLock(lock);
	sprintf(dirnamn,"NiKom:Users/%d/%d",x/100,x);
	if(!(lock=Lock(dirnamn,ACCESS_READ)))
		if(!(lock=CreateDir(dirnamn))) {
			printf("Kunde inte skapa %s\n",dirnamn);
			return(2);
		}
	UnLock(lock);
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",x/100,x);
	if(!(fh=Open(filnamn,MODE_NEWFILE))) {
		puttekn("\r\n\nKunde inte öppna Users.dat!\r\n\n",-1);
		return(2);
	}
	if(Write(fh,(void *)&Servermem->inne[nodnr],sizeof(struct User))==-1) {
		puttekn("\r\n\nKunde inte skriva Users.dat!\r\n\n",-1);
		Close(fh);
		return(2);
	}
	Close(fh);

        if(!WriteUnreadTexts(&Servermem->unreadTexts[nodnr], x)) {
          puttekn("\r\n\nKunde inte skriva data om olästa texter!\r\n\n",-1);
          return(2);
        }

	sprintf(filnamn,"Nikom:Users/%d/%d/.firstletter",x/100,x);
	if(!(fh=Open(filnamn,MODE_NEWFILE))) {
		printf("Kunde inte öppna %s\n",filnamn);
		return(2);
	}
	Write(fh,"0",1);
	Close(fh);
	sprintf(filnamn,"Nikom:Users/%d/%d/.nextletter",x/100,x);
	if(!(fh=Open(filnamn,MODE_NEWFILE))) {
		printf("Kunde inte öppna %s\n",filnamn);
		return(2);
	}
	Write(fh,"0",1);
	Close(fh);
	inloggad=x;
	sprintf(outbuffer,"\r\n\nDu får användarnumret %d\r\n",inloggad);
	puttekn(outbuffer,-1);
	if(Servermem->cfg.ar.nyanv) sendrexx(Servermem->cfg.ar.nyanv);
	return(1);
}

int parsenamn(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,found=-1,nummer;
	char *faci,*skri2;
	struct ShortUser *letpek;
	if(skri[0]==0 || skri[0]==' ') return(-3);
	if(isdigit(skri[0]) || (skri[0]=='#' && isdigit(skri[1]))) {
		if(skri[0]=='#') skri++;
		nummer=atoi(skri);
		faci=getusername(nummer);
		if(!strcmp(faci,"<Raderad Användare>") || !strcmp(faci,"<Felaktigt användarnummer>")) return(-1);
		else return(nummer);
	}
	if(matchar(skri,"sysop")) return(0);
	letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;
	while(letpek->user_node.mln_Succ && going) {
		faci=letpek->namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=hittaefter(skri2);
				faci=hittaefter(faci);
				if(skri2[0]==0) {
					found=letpek->nummer;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=hittaefter(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		letpek=(struct ShortUser *)letpek->user_node.mln_Succ;
	}
	return(found);
}

int matchar(skrivet,facit)
char *skrivet,*facit;
{
	int mat=TRUE,count=0;
	char tmpskrivet,tmpfacit;
	if(facit!=NULL) {
		while(mat && skrivet[count]!=' ' && skrivet[count]!=0) {
			if(skrivet[count]=='*') { count++; continue; }
			tmpskrivet=ToUpper(skrivet[count]);
			tmpfacit=ToUpper(facit[count]);
			if(tmpskrivet!=tmpfacit) mat=FALSE;
			count++;
		}
	}
	return(mat);
}

int readuser(int nummer,struct User *user) {
	BPTR fh;
	char filnamn[40];
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",nummer/100,nummer);
	NiKForbid();
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Users.dat!\r\n\n",-1);
		NiKPermit();
		return(1);
	}
	if(Read(fh,(void *)user,sizeof(struct User))==-1) {
		puttekn("\r\n\nKunde inte läsa Users.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return(1);
	}
	Close(fh);
	NiKPermit();
	return(0);
}

char *getusername(int nummer) {
	struct ShortUser *letpek;
	int found=FALSE;
/*	if(!reggadnamn[0] && nummer>=5) return((char *)0xbadbad42); */
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			if(letpek->namn[0]) sprintf(usernamebuf,"%s #%d",letpek->namn,nummer);
			else strcpy(usernamebuf,"<Raderad Användare>");
			found=TRUE;
			break;
		}
	}
	if(!found) strcpy(usernamebuf,"<Felaktigt användarnummer>");
	return(usernamebuf);
}

int bytteckenset(void) {
	char tkn;
	int chrsetbup,going=TRUE;
	chrsetbup = Servermem->inne[nodnr].chrset;
	if(Servermem->nodtyp[nodnr] == NODCON) puttekn("\n\n\r*** OBS! Du kör på en CON-nod, alla teckenset kommer se likadana ut!! ***",-1);
	puttekn("\n\n\rDessa teckenset finns. Ta det alternativ vars svenska tecken ser bra ut.\n\r",-1);
	puttekn("* markerar nuvarande val.\n\n\r",-1);
	puttekn("  Nr Namn                              Exempel\n\r",-1);
	puttekn("----------------------------------------------\n\r",-1);
	Servermem->inne[nodnr].chrset = CHRS_LATIN1;
	sprintf(outbuffer,"%c 1: ISO 8859-1 (ISO Latin 1)          åäö ÅÄÖ\n\r",chrsetbup == CHRS_LATIN1 ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = CHRS_CP437;
	sprintf(outbuffer,"%c 2: IBM CodePage (PC8)                åäö ÅÄÖ\n\r",chrsetbup == CHRS_CP437 ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = CHRS_MAC;
	sprintf(outbuffer,"%c 3: Macintosh                         åäö ÅÄÖ\n\r",chrsetbup == CHRS_MAC ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = CHRS_SIS7;
	sprintf(outbuffer,"%c 4: SIS-7 (SF7, måsvingar)            åäö ÅÄÖ\n\r",chrsetbup == CHRS_SIS7 ? '*' : ' ');
	puttekn(outbuffer,-1);
	Servermem->inne[nodnr].chrset = chrsetbup;
	puttekn("\n\rOm du bara ser ett alternativ ovan beror det av att ditt terminalprogram\n\r",-1);
	puttekn("strippar den 8:e biten i inkommande tecken. Om exemplet ser bra ut, ta\n\r",-1);
	puttekn("SIS-7 (dvs, tryck 4).\n\r",-1);
	puttekn("\n\rVal: ",-1);
	while(going) {
		tkn = gettekn();
		if(carrierdropped()) return(1);
		going = FALSE;
		switch(tkn) {
			case '1' :
				Servermem->inne[nodnr].chrset = CHRS_LATIN1;
				puttekn("ISO 8859-1\n\r",-1);
				break;
			case '2' :
				Servermem->inne[nodnr].chrset = CHRS_CP437;
				puttekn("IBM CodePage\n\r",-1);
				break;
			case '3' :
				Servermem->inne[nodnr].chrset = CHRS_MAC;
				puttekn("Macintosh\n\r",-1);
				break;
			case '4' :
				Servermem->inne[nodnr].chrset = CHRS_SIS7;
				puttekn("SIS-7\n\r",-1);
				break;
			default :
				going = TRUE;
		}
	}
	return(0);
}
