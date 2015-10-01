#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <devices/timer.h>
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
extern int nodnr,inloggad,senast_text_typ,rad,mote2,buftkn,senast_brev_nr,senast_brev_anv;
extern char outbuffer[],inmat[],backspace[],*argument,vilkabuf[];
extern struct MsgPort *timerport,*conreadport,*serreadport;
extern struct Inloggning Statstr;
extern struct timerequest *timerreq;
extern char coninput;
extern struct MinList edit_list;

int nyanv(void) {
	BPTR lock,fh;
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	long tid;
	int going=TRUE,x=0;
	UBYTE tillftkn;
	struct ShortUser *allokpek;
	char dirnamn[100], filnamn[40], encryptedPwd[14];
	puttekn("\r\n\n",-1);
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
	Servermem->inne[nodnr].rader=Servermem->cfg.defaultrader;
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
	if(Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS) {
          CryptPassword(Servermem->inne[nodnr].losen, encryptedPwd);
          strcpy(Servermem->inne[nodnr].losen, encryptedPwd);
        }

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
          return 2;
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

char *hittaefter(strang)
char *strang;
{
	do
	{
		while(*(strang) != 0 && *(strang) != ' ')
			strang++;

	} while(*(strang) != 0 && *(++strang) == ' ');

	return strang;

/*	int test=TRUE;
	while(test) {
		test=FALSE;
		while(*(++strang)!=' ' && *strang!=0);
		if(*(strang+1)==' ') test=TRUE;
	}
	return(*strang==0 ? strang : ++strang); */
}

void status(void) {
	struct User readuserstr;
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	int nummer,nod,cnt=0,olasta=FALSE,tot=0;
	struct tm *ts;
	struct UserGroup *listpek;
	char filnamn[100];
        struct UnreadTexts unreadTextsBuf, *unreadTexts;

	if(argument[0]==0) {
		memcpy(&readuserstr,&Servermem->inne[nodnr],sizeof(struct User));
                unreadTexts = &Servermem->unreadTexts[nodnr];
		nummer=inloggad;
	} else {
		if((nummer=parsenamn(argument))==-1) {
			puttekn("\r\n\nFinns ingen som heter så eller har det numret\r\n\n",-1);
			return;
		}
		for(nod=0;nod<MAXNOD;nod++) if(nummer==Servermem->inloggad[nod]) break;
		if(nod<MAXNOD)
		{
			memcpy(&readuserstr,&Servermem->inne[nod],sizeof(struct User));
                        unreadTexts = &Servermem->unreadTexts[nod];
		} else {
			if(readuser(nummer,&readuserstr)) return;
                        if(!ReadUnreadTexts(&unreadTextsBuf, nummer)) {
                          return;
                        }
                        unreadTexts = &unreadTextsBuf;
		}
	}
	sprintf(outbuffer,"\r\n\nStatus för %s #%d\r\n\n",readuserstr.namn,nummer);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Status               : %d\r\n",readuserstr.status);
	if(puttekn(outbuffer,-1)) return;
	if(!((readuserstr.flaggor & SKYDDAD) && Servermem->inne[nodnr].status<Servermem->cfg.st.sestatus && inloggad!=nummer)) {
		sprintf(outbuffer,"Gatuadress           : %s\r\n",readuserstr.gata);
		if(puttekn(outbuffer,-1)) return;
		sprintf(outbuffer,"Postadress           : %s\r\n",readuserstr.postadress);
		if(puttekn(outbuffer,-1)) return;
		sprintf(outbuffer,"Land                 : %s\r\n",readuserstr.land);
		if(puttekn(outbuffer,-1)) return;
		sprintf(outbuffer,"Telefon              : %s\r\n",readuserstr.telefon);
		if(puttekn(outbuffer,-1)) return;
	}
	sprintf(outbuffer,"Annan info           : %s\r\n",readuserstr.annan_info);
	if(puttekn(outbuffer,-1)) return;
	if(!Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS && Servermem->inne[nodnr].status>=Servermem->cfg.st.psw && readuserstr.status<Servermem->inne[nodnr].status)
	{
		sprintf(outbuffer,"Lösenord             : %s\r\n",readuserstr.losen);
		if(puttekn(outbuffer,-1)) return;
	}
	sprintf(outbuffer,"Antal rader          : %d\r\n",readuserstr.rader);
	if(puttekn(outbuffer,-1)) return;
	ts=localtime(&readuserstr.forst_in);
	sprintf(outbuffer,"Först inloggad       : %4d%02d%02d  %02d:%02d\r\n",
                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
                ts->tm_min);
	if(puttekn(outbuffer,-1)) return;
	ts=localtime(&readuserstr.senast_in);
	sprintf(outbuffer,"Senast inloggad      : %4d%02d%02d  %02d:%02d\r\n",
                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
                ts->tm_min);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Total tid inloggad   : %d:%02d\r\n",readuserstr.tot_tid/3600,(readuserstr.tot_tid%3600)/60);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Antal inloggningar   : %d\r\n",readuserstr.loggin);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Antal lästa texter   : %d\r\n",readuserstr.read);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Antal skrivna texter : %d\r\n",readuserstr.skrivit);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Antal downloads      : %d\r\n",readuserstr.download);
	if(puttekn(outbuffer,-1)) return;
	sprintf(outbuffer,"Antal uploads        : %d\r\n",readuserstr.upload);
	if(puttekn(outbuffer,-1)) return;

	if(readuserstr.downloadbytes < 90000)
	{
		sprintf(outbuffer,"Antal downloaded B   : %d\r\n",readuserstr.downloadbytes);
		if(puttekn(outbuffer,-1)) return;
	}
	else
	{
		sprintf(outbuffer,"Antal downloaded KB  : %d\r\n",readuserstr.downloadbytes/1024);
		if(puttekn(outbuffer,-1)) return;
	}

	if(readuserstr.uploadbytes < 90000)
	{
		sprintf(outbuffer,"Antal uploaded B     : %d\r\n\n",readuserstr.uploadbytes);
		if(puttekn(outbuffer,-1)) return;
	}
	else
	{
		sprintf(outbuffer,"Antal uploaded KB    : %d\r\n\n",readuserstr.uploadbytes/1024);
		if(puttekn(outbuffer,-1)) return;
	}

	ShowProgramDatacfg(nummer);

	if(readuserstr.grupper) {
		puttekn("Grupper:\r\n",-1);
		for(listpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;listpek->grupp_node.mln_Succ;listpek=(struct UserGroup *)listpek->grupp_node.mln_Succ) {
			if(!BAMTEST((char *)&readuserstr.grupper,listpek->nummer)) continue;
			if((listpek->flaggor & HEMLIGT) && !BAMTEST((char *)&Servermem->inne[nodnr].grupper,listpek->nummer) && Servermem->inne[nodnr].status<Servermem->cfg.st.medmoten) continue;
			sprintf(outbuffer," %s\r\n",listpek->namn);
			if(puttekn(outbuffer,-1)) return;
		}
		eka('\n');
	}
	if(cnt=countmail(nummer,readuserstr.brevpek)) {
		sprintf(outbuffer,"%4d %s\r\n",cnt,Servermem->cfg.brevnamn);
		if(puttekn(outbuffer,-1)) return;
		olasta=TRUE;
		tot+=cnt;
	}
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ)
	{
		if(motpek->status & SUPERHEMLIGT) continue;
		if(IsMemberConf(motpek->nummer, nummer, &readuserstr)) {
			cnt=0;
			switch(motpek->type) {
                        case MOTE_ORGINAL :
                          cnt = CountUnreadTexts(motpek->nummer, unreadTexts);
                          break;
                        default :
                          break;
			}
			if(cnt && MaySeeConf(motpek->nummer, inloggad, &Servermem->inne[nodnr]))
			{
				sprintf(outbuffer,"%4d %s\r\n",cnt,motpek->namn);
				if(puttekn(outbuffer,-1)) return;
				olasta=TRUE;
				tot+=cnt;
			}
		}
	}
	if(!olasta) {
		if(nummer==inloggad) puttekn("Du har inga olästa texter.",-1);
		else {
			sprintf(outbuffer,"%s har inga olästa texter.",readuserstr.namn);
			puttekn(outbuffer,-1);
		}
	} else {
		sprintf(outbuffer,"\r\nSammanlagt %d olästa.",tot);
		puttekn(outbuffer,-1);
	}
	puttekn("\r\n\n",-1);
	sprintf(filnamn,"NiKom:Users/%d/%d/Lapp",nummer/100,nummer);
	if(!access(filnamn,0)) sendfile(filnamn);
}

int andraanv(void) {
	int nummer,going=TRUE,tillf,x;
	char kor;
	struct User readuserstr;
	struct ShortUser *letpek;
	if(argument[0]) {
		if(Servermem->inne[nodnr].status<Servermem->cfg.st.anv) {
			puttekn("\r\n\nDu kan inte ändra andra användare än dig själv!\r\n\n",-1);
			return(0);
		}
		if((nummer=parsenamn(argument))==-1) {
			puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n",-1);
			return(0);
		} else if(nummer==-3) {
			puttekn("\r\n\nÖhh... Nu returnerades ett värde som inte borde returneras!\r\n\n",-1);
			return(0);
		}
		for(x=0;x<MAXNOD;x++) if(nummer==Servermem->inloggad[x]) break;
		if(x<MAXNOD) memcpy(&readuserstr,&Servermem->inne[x],sizeof(struct User));
		else if(readuser(nummer,&readuserstr)) return(0);
	} else {
		memcpy(&readuserstr,&Servermem->inne[nodnr],sizeof(struct User));
		nummer=inloggad;
		puttekn("\r\n\nÄndrar egen status",-1);
	}
	while(going) {
		sprintf(outbuffer,"\r\nNamn : (%s) ",readuserstr.namn);
		puttekn(outbuffer,-1);
		if(getstring(EKO,40,NULL)) return(1);
		if(inmat[0]) {
			if((tillf=parsenamn(inmat))!=-1 && tillf!=nummer) puttekn("\r\n\nNamnet finns redan!\r\n",-1);
			else {
				strncpy(readuserstr.namn,inmat,40);
				going=FALSE;
			}
		} else going=FALSE;
	}
	if(Servermem->inne[nodnr].status>=Servermem->cfg.st.chgstatus) {
		sprintf(outbuffer,"\r\nStatus : (%d) ",readuserstr.status);
		puttekn(outbuffer,-1);
		if(getstring(EKO,3,NULL)) return(1);
		if(inmat[0]) {
			if((tillf=atoi(inmat))<0 || tillf>100) puttekn("\r\nFelaktig status, behåller den gamla!",-1);
			else readuserstr.status=tillf;
		}
	}
	sprintf(outbuffer,"\r\nGatuadress : (%s) ",readuserstr.gata);
	puttekn(outbuffer,-1);
	if(getstring(EKO,40,NULL)) return(1);
	if(inmat[0]) strncpy(readuserstr.gata,inmat,40);
	sprintf(outbuffer,"\r\nPostadress : (%s) ",readuserstr.postadress);
	puttekn(outbuffer,-1);
	if(getstring(EKO,40,NULL)) return(1);
	if(inmat[0]) strncpy(readuserstr.postadress,inmat,40);
	sprintf(outbuffer,"\r\nLand : (%s) ",readuserstr.land);
	puttekn(outbuffer,-1);
	if(getstring(EKO,40,NULL)) return(1);
	if(inmat[0]) strncpy(readuserstr.land,inmat,40);
	sprintf(outbuffer,"\r\nTelefon : (%s) ",readuserstr.telefon);
	puttekn(outbuffer,-1);
	if(getstring(EKO,20,NULL)) return(1);
	if(inmat[0]) strncpy(readuserstr.telefon,inmat,20);
	sprintf(outbuffer,"\r\nAnnan info : (%s) ",readuserstr.annan_info);
	puttekn(outbuffer,-1);
	if(getstring(EKO,59,NULL)) return(1);
	if(inmat[0]) strncpy(readuserstr.annan_info,inmat,60);
	sprintf(outbuffer,"\r\nLösenord : ");

	puttekn(outbuffer,-1);
	if(getstring(EKO,15,NULL)) return(1);
	if(inmat[0]) {
          if(Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS) {
            CryptPassword(inmat, readuserstr.losen);
          } else {
            strncpy(readuserstr.losen, inmat, 15);
          }
	}
	sprintf(outbuffer,"\r\nPrompt : (%s) ",readuserstr.prompt);
	puttekn(outbuffer,-1);
	if(getstring(EKO,5,NULL)) return(1);
	if(inmat[0]) strncpy(readuserstr.prompt,inmat,5);
	sprintf(outbuffer,"\r\nAntal rader : (%d) ",readuserstr.rader);
	puttekn(outbuffer,-1);
	if(getstring(EKO,5,NULL)) return(1);
	if(inmat[0]) readuserstr.rader=atoi(inmat);
	if(Servermem->inne[nodnr].status>=Servermem->cfg.st.anv) {
		sprintf(outbuffer,"\r\nAntal lästa : (%d) ",readuserstr.read);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.read=atoi(inmat);
		sprintf(outbuffer,"\r\nAntal skrivna : (%d) ",readuserstr.skrivit);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.skrivit=atoi(inmat);
		sprintf(outbuffer,"\r\nAntal downloads : (%d) ",readuserstr.download);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.download=atoi(inmat);
		sprintf(outbuffer,"\r\nAntal uploads : (%d) ",readuserstr.upload);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.upload=atoi(inmat);
		sprintf(outbuffer,"\r\nAntal dlbytes : (%d) ",readuserstr.downloadbytes);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.downloadbytes=atoi(inmat);

		sprintf(outbuffer,"\r\nAntal ulbytes : (%d) ",readuserstr.uploadbytes);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.uploadbytes=atoi(inmat);

		sprintf(outbuffer,"\r\nAntal inloggningar : (%d) ",readuserstr.loggin);
		puttekn(outbuffer,-1);
		if(getstring(EKO,8,NULL)) return(1);
		if(inmat[0]) readuserstr.loggin=atoi(inmat);
	}
	puttekn("\r\n\nÄr allt korrekt? (J/n) ",-1);
	while((kor=gettekn())!='j' && kor!='J' && kor!='n' && kor!='N' && kor!='\r');
	if(kor=='j' || kor=='J' || kor=='\r') puttekn("Ja\r\n\n",-1);
	else {
		puttekn("Nej\r\n\n",-1);
		return(0);
	}
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
		if(letpek->nummer==nummer) break;
	if(letpek->user_node.mln_Succ) {
		strncpy(letpek->namn,readuserstr.namn,40);
		letpek->status=readuserstr.status;
	} else {
		puttekn("\r\n\nHittade inte ShortUser-strukturen!\r\n",-1);
		return(0);
	}
	for(x=0;x<MAXNOD;x++) if(nummer==Servermem->inloggad[x]) break;
	if(x<MAXNOD) {
		strncpy(Servermem->inne[x].namn,readuserstr.namn,40);
		Servermem->inne[x].status=readuserstr.status;
		strncpy(Servermem->inne[x].gata,readuserstr.gata,40);
		strncpy(Servermem->inne[x].postadress,readuserstr.postadress,40);
		strncpy(Servermem->inne[x].land,readuserstr.land,40);
		strncpy(Servermem->inne[x].telefon,readuserstr.telefon,40);
		strncpy(Servermem->inne[x].annan_info,readuserstr.annan_info,60);
		strncpy(Servermem->inne[x].losen,readuserstr.losen,15);
		strncpy(Servermem->inne[x].prompt,readuserstr.prompt,5);
		Servermem->inne[x].rader=readuserstr.rader;
		Servermem->inne[x].read=readuserstr.read;
		Servermem->inne[x].skrivit=readuserstr.skrivit;
		Servermem->inne[x].download=readuserstr.download;
		Servermem->inne[x].upload=readuserstr.upload;
		Servermem->inne[x].uploadbytes=readuserstr.uploadbytes;
		Servermem->inne[x].downloadbytes=readuserstr.downloadbytes;
		Servermem->inne[x].loggin=readuserstr.loggin;
		return(0);
	}
	writeuser(nummer,&readuserstr);
	return(0);
}

void raderaanv(void) {
	struct ShortUser *letpek;
	char kor;
	int nummer;
	if(!argument[0]) {
		puttekn("\r\n\nSkriv: Radera Användare <användare>\r\n\n",-1);
		return;
	}
	if((nummer=parsenamn(argument))==-1) {
		puttekn("\r\n\nFinns ingen som heter så eller har det numret\r\n\n",-1);
		return;
	}
	sprintf(outbuffer,"\r\n\nÄr du säker på att du vill radera %s? (j/N) ",getusername(nummer));
	puttekn(outbuffer,-1);
	while((kor=gettekn())!='j' && kor!='J' && kor!='n' && kor!='N' && kor!='\r');
	if(kor=='n' || kor=='N' || kor=='\r') {
		puttekn("Nej\r\n\n",-1);
		return;
	}
	puttekn("Ja\r\n\n",-1);
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
		if(letpek->nummer==nummer) break;
	if(letpek->user_node.mln_Succ) Remove((struct Node *)letpek);
	else {
		puttekn("\r\n\nKunde inte hitta ShortUser-strukturen!\r\n",-1);
		return;
	}
	sprintf(inmat,"%d",nummer);
	argument=inmat;
	sendrexx(15);
}

int listaanv(void) {
	struct ShortUser *listpek;
	int backwards=FALSE,status=-1,going=TRUE,more=0,less=100,cnt=2;
	char pattern[40]="",*argpek=argument,*tpek=NULL;
	puttekn("\r\n\n",-1);
	while(argpek[0] && going) {
		if(argpek[0]=='-') {
			if(argpek[1]=='b' || argpek[1]=='B') backwards=TRUE;
			else if(argpek[1]='s' || argpek[1]=='S') {
				while(argpek[cnt] && argpek[cnt]!=' ') {
					if(argpek[cnt]=='-') { tpek=&argpek[cnt+1]; break; }
					cnt++;
				}
				if(tpek) {
					if(argpek[2]!='-') more=atoi(&argpek[2]);
					if(tpek[0] && tpek[0]!=' ') less=atoi(tpek);
				} else status=atoi(&argpek[2]);
			}
		} else {
			strncpy(pattern,argpek,39);
			going=FALSE;
		}
		argpek=hittaefter(argpek);
	}
	if(backwards) {
		for(listpek=(struct ShortUser *)Servermem->user_list.mlh_TailPred;listpek->user_node.mln_Pred;listpek=(struct ShortUser *)listpek->user_node.mln_Pred) {
			if(status!=-1 && status!=listpek->status) continue;
			if(listpek->status>less || listpek->status<more) continue;
			if(!namematch(pattern,listpek->namn)) continue;
			sprintf(outbuffer,"%s #%d\r\n",listpek->namn,listpek->nummer);
			if(puttekn(outbuffer,-1)) break;
		}
		return(0);
	}
	for(listpek=(struct ShortUser *)Servermem->user_list.mlh_Head;listpek->user_node.mln_Succ;listpek=(struct ShortUser *)listpek->user_node.mln_Succ) {
		if(status!=-1 && status!=listpek->status) continue;
		if(listpek->status>less || listpek->status<more) continue;
		if(!namematch(pattern,listpek->namn)) continue;
		sprintf(outbuffer,"%s #%d\r\n",listpek->namn,listpek->nummer);
		if(puttekn(outbuffer,-1)) break;
	}
	return(0);
}

void listmed(void) {
	char kor;
	int med;
	struct ShortUser *letpek;
	struct Mote *motpek;
	struct User listuser;
	if(argument[0]=='-' && (argument[1]=='g' || argument[1]=='G')) {
		argument=hittaefter(argument);
		listgruppmed();
		return;
	}
	if(mote2==-1) {
		sprintf(outbuffer,"\r\n\nAlla är medlemmar i %s\r\n\n",Servermem->cfg.brevnamn);
		puttekn(outbuffer,-1);
		return;
	}
	puttekn("\r\n\nLista medlemmar eller icke medlemmar? (M/i) ",-1);
	while((kor=gettekn())!='m' && kor!='M' && kor!='i' && kor!='I' && kor!='\r');
	if(kor=='m' || kor=='M' || kor=='\r') {
		puttekn("Medlemmar\r\n\n",-1);
		med=TRUE;
	} else {
		puttekn("Icke medlemmar\r\n\n",-1);
		med=FALSE;
	}
	motpek=getmotpek(mote2);
	sprintf(outbuffer,"%sedlemmar i mötet %s\n\n\r",med ? "M" : "Icke m",motpek->namn);
	puttekn(outbuffer,-1);
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(readuser(letpek->nummer,&listuser)) return;
		if((med && IsMemberConf(mote2, letpek->nummer, &listuser)) || (!med && !IsMemberConf(mote2, letpek->nummer, &listuser))) {
			sprintf(outbuffer,"%s #%d\r\n",listuser.namn,letpek->nummer);
			if(puttekn(outbuffer,-1)) return;
		}
	}
}

void listratt(void) {
	char kor;
	int ratt;
	struct ShortUser *letpek;
	struct Mote *motpek;
	struct User listuser;
	if(mote2==-1) {
		sprintf(outbuffer,"\r\n\nAlla har fullständiga rättigheter i %s\r\n\n",Servermem->cfg.brevnamn);
		puttekn(outbuffer,-1);
		return;
	}
	motpek=getmotpek(mote2);
	if(motpek->status & AUTOMEDLEM) {
		puttekn("\n\n\rAlla har rättigheter i auto-möten.\n\r",-1);
		return;
	}
	if(motpek->status & SUPERHEMLIGT) {
		puttekn("\n\n\rIngen, men samtidigt alla, har rättigheter i ARexx-styrda möten.\n\r",-1);
		return;
	}
	puttekn("\r\n\nLista rättigheter eller icke rättigheter? (R/i) ",-1);
	while((kor=gettekn())!='r' && kor!='R' && kor!='i' && kor!='I' && kor!='\r');
	if(kor=='r' || kor=='R' || kor=='\r') {
		puttekn("Rättigheter\r\n\n",-1);
		ratt=TRUE;
	} else {
		puttekn("Icke rättigheter\r\n\n",-1);
		ratt=FALSE;
	}
	sprintf(outbuffer,"%sättigheter i mötet %s\n\n\r",ratt ? "R" : "Icke r",motpek->namn);
	puttekn(outbuffer,-1);
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(readuser(letpek->nummer,&listuser)) return;
		if((ratt && BAMTEST(listuser.motratt,mote2)) || (!ratt && !BAMTEST(listuser.motratt,mote2))) {
			sprintf(outbuffer,"%s #%d\r\n",listuser.namn,letpek->nummer);
			if(puttekn(outbuffer,-1)) return;
		}
	}
}

void listnyheter(void) {
	int x,cnt=0,olasta=FALSE,tot=0;
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	struct Fil *sokpek;
	puttekn("\r\n\n",-1);
	if(cnt=countmail(inloggad,Servermem->inne[nodnr].brevpek)) {
		sprintf(outbuffer,"%4d %s\r\n",cnt,Servermem->cfg.brevnamn);
		puttekn(outbuffer,-1);
		olasta=TRUE;
		tot+=cnt;
	}
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
		if(motpek->status & SUPERHEMLIGT) continue;
		if(IsMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr]))
		{
			cnt=countunread(motpek->nummer);
			if(cnt) {
				sprintf(outbuffer,"%4d %s\r\n",cnt,motpek->namn);
				if(puttekn(outbuffer,-1)) return;
				olasta=TRUE;
				tot+=cnt;
			}
		}
	}
	if(!olasta) puttekn("Du har inga olästa texter.",-1);
	else {
		sprintf(outbuffer,"\r\nSammanlagt %d olästa texter.",tot);
		puttekn(outbuffer,-1);
	}
	puttekn("\r\n\n",-1);
	if(Servermem->inne[nodnr].flaggor & FILLISTA) nyafiler();
	else {
		for(x=0;x<Servermem->info.areor;x++) {
			if(!arearatt(x, inloggad, &Servermem->inne[nodnr])) continue;
			olasta=0;
			for(sokpek=(struct Fil *)Servermem->areor[x].ar_list.mlh_TailPred;sokpek->f_node.mln_Pred;sokpek=(struct Fil *)sokpek->f_node.mln_Pred) {
				if((sokpek->flaggor & FILE_NOTVALID) && Servermem->inne[nodnr].status < Servermem->cfg.st.filer && sokpek->uppladdare != inloggad) continue;
				if(sokpek->validtime < Servermem->inne[nodnr].senast_in) continue;
				else olasta++;
			}
			if(olasta) {
				sprintf(outbuffer,"%2d %s\r\n",olasta,Servermem->areor[x].namn);
				if(puttekn(outbuffer,-1)) return;
			}
		}
	}
}

void listflagg(void) {
	int x;
	puttekn("\r\n\n",-1);
	for(x=31;x>(31-ANTFLAGG);x--) {
		if(BAMTEST((char *)&Servermem->inne[nodnr].flaggor,x)) puttekn("<På>  ",-1);
		else puttekn("<Av>  ",-1);
		sprintf(outbuffer,"%s\r\n",Servermem->flaggnamn[31-x]);
		puttekn(outbuffer,-1);
	}
}

int hoppaover(int rot,int ack) {
	static struct Header hoppahead;
	int x=0;
	long kom_i[MAXKOM];
	if(readtexthead(rot,&hoppahead)) return(ack);
	memcpy(kom_i,hoppahead.kom_i,MAXKOM*sizeof(long));
        ChangeUnreadTextStatus(rot, 0, &Servermem->unreadTexts[nodnr]);
	ack++;
	while(kom_i[x]!=-1 && x<MAXKOM) {
		ack=hoppaover(kom_i[x],ack);
		x++;
	}
	return(ack);
}

int parseflagga(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==ANTFLAGG) going=FALSE;
		faci=Servermem->flaggnamn[count];
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=hittaefter(skri2);
				faci=hittaefter(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=hittaefter(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

void slaav(void) {
	int flagga;
	if((flagga=parseflagga(argument))==-1) {
		puttekn("\r\n\nFinns ingen flagga som heter så!\r\n\n",-1);
		return;
	} else if(flagga==-3) {
		puttekn("\r\n\nSkriv: Slå Av <flaggnamn>\r\n\n",-1);
		return;
	}
	sprintf(outbuffer,"\r\n\nSlår av flaggan %s\r\n",Servermem->flaggnamn[flagga]);
	puttekn(outbuffer,-1);
	if(BAMTEST((char *)&Servermem->inne[nodnr].flaggor,31-flagga)) puttekn("Den var påslagen.\r\n\n",-1);
	else puttekn("Den var redan avslagen.\r\n\n",-1);
	BAMCLEAR((char *)&Servermem->inne[nodnr].flaggor,31-flagga);
}

void slapa(void) {
	int flagga;
	if((flagga=parseflagga(argument))==-1) {
		puttekn("\r\n\nFinns ingen flagga som heter så!\r\n\n",-1);
		return;
	} else if(flagga==-3) {
		puttekn("\r\n\nSkriv: Slå På <flaggnamn>\r\n\n",-1);
		return;
	}
	sprintf(outbuffer,"\r\n\nSlår på flaggan %s\r\n",Servermem->flaggnamn[flagga]);
	puttekn(outbuffer,-1);
	if(!BAMTEST((char *)&Servermem->inne[nodnr].flaggor,31-flagga)) puttekn("Den var avslagen.\r\n\n",-1);
	else puttekn("Den var redan påslagen.\r\n\n",-1);
	BAMSET((char *)&Servermem->inne[nodnr].flaggor,31-flagga);
}

int ropa(void) {
	long signals, timesig=1L <<timerport->mp_SigBit, consig=1L<<conreadport->mp_SigBit;
	int going=TRUE,x,flagga=FALSE;
	UBYTE tecken;
	puttekn("\r\n\n",-1);
	for(x=0;x<10;x++) {
		sprintf(outbuffer,"\rSYSOP!! %s vill dig något!! (%d)",Servermem->inne[nodnr].namn,x);
		putstring(outbuffer,-1,0);
		DisplayBeep(NULL);
		timerreq->tr_node.io_Command=TR_ADDREQUEST;
		timerreq->tr_node.io_Message.mn_ReplyPort=timerport;
		timerreq->tr_time.tv_secs=1;
		timerreq->tr_time.tv_micro=0;
		SendIO((struct IORequest *)timerreq);
		signals=Wait(timesig | consig);
		if(signals & timesig) {
			WaitIO((struct IORequest *)timerreq);
		}
		if(signals & consig) {
			congettkn();
			if(!CheckIO((struct IORequest *)timerreq)) {
				AbortIO((struct IORequest *)timerreq);
				WaitIO((struct IORequest *)timerreq);
			}
			break;
		}
	}
	if(x==10) {
		puttekn("\r\n\nTyvärr, sysop verkar inte vara tillgänglig för tillfället\r\n\n",-1);
		return(0);
	}
	puttekn("\r\nSysop här! (Tryck Ctrl-Z för att avsluta samtalet.)\r\n",-1);
	strcpy(vilkabuf,"pratar med sysop");
	Servermem->vilkastr[nodnr]=vilkabuf;
	Servermem->action[nodnr]=GORNGTANNAT;
	while(going) {
		tecken=gettekn();
		if((tecken>31 && tecken <127) || (tecken>159 && tecken<=255)) {
			putstring("\x1b\x5b\x31\x40",-1,0);
			eka(tecken);
		} else if(tecken==13) {
			eka((char)13);
			eka((char)10);
		} else if(tecken==10) {
			if(carrierdropped()) return(1);
			else {
				eka((char)10);
				eka((char)13);
			}
		} else if(tecken==8) {
			putstring("\x1b\x5b\x44\x1b\x5b\x50",-1,0);
		} else if(tecken==127) {
			putstring("\x1b\x5b\x50",-1,0);
		} else if(tecken==26) {
			going=FALSE;
		} else if(tecken==3) {
			going=FALSE;
			flagga=TRUE;
		} else if(tecken==7) eka(7);
		else if(tecken=='\x9b' || (tecken=='\x1b' && gettekn()=='[')) {
			if((tecken=gettekn())=='\x44') putstring("\x1b\x5b\x44",-1,0);
			else if(tecken=='\x43') putstring("\x1b\x5b\x43",-1,0);
		}
	}
	if(flagga) {
		puttekn("\r\nTar bort filtret.\r\n",-1);
		going=TRUE;
		while(going) {
			if((tecken=gettekn())==10) {
				if(carrierdropped()) return(1);
				else {
					eka((char)10);
				}
			} else if(tecken==26) going=FALSE;
			else eka(tecken);
		}
	}
	return(0);
}

void writemeet(struct Mote *motpek) {
	BPTR fh;
	NiKForbid();
	if(!(fh=Open("NiKom:DatoCfg/Möten.dat",MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Möten.dat\r\n",-1);
		NiKPermit();
		return;
	}
	if(Seek(fh,motpek->nummer*sizeof(struct Mote),OFFSET_BEGINNING)==-1) {
		puttekn("\r\n\nKunde inte söka i Moten.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return;
	}
	if(Write(fh,(void *)motpek,sizeof(struct Mote))==-1) {
		puttekn("\r\n\nFel vid skrivandet av Möten.dat\r\n",-1);
	}
	Close(fh);
	NiKPermit();
}

void addratt(void) {
	int anv,x;
	struct Mote *motpek;
	struct User adduser;
	if(mote2==-1) {
		puttekn("\r\n\nDu kan inte addera rättigheter i brevlådan!\r\n",-1);
		return;
	}
	motpek=getmotpek(mote2);
	if(!MayAdminConf(mote2, inloggad, &Servermem->inne[nodnr])) {
		puttekn("\r\n\nDu har inte rätt att addera rättigheter i det här mötet!\r\n\n",-1);
		return;
	}
	if((anv=parsenamn(argument))==-1) {
		puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n",-1);
		return;
	} else if(anv==-3) {
		puttekn("\r\n\nSkriv: Addera rättigheter <användare>\r\n(Se till så att du befinner dig i rätt möte)\r\n\n",-1);
		return;
	}
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==anv) break;
	if(x<MAXNOD) {
		BAMSET(Servermem->inne[x].motratt,motpek->nummer);
		sprintf(outbuffer,"\n\n\rRättigheter i %s adderade för %s\n\r",motpek->namn,getusername(anv));
	} else {
		if(readuser(anv,&adduser)) return;
		BAMSET(adduser.motratt,motpek->nummer);
		if(writeuser(anv,&adduser)) return;
		sprintf(outbuffer,"\n\n\rRättigheter i %s adderade för %s #%d\n\r",motpek->namn,adduser.namn,anv);
	}
	puttekn(outbuffer,-1);
}

void subratt(void) {
	int anv,x;
	struct Mote *motpek;
	struct User subuser;
	if(mote2==-1) {
		puttekn("\r\n\nDu kan inte subtrahera rättigheter i brevlådan!\r\n",-1);
		return;
	}
	motpek=getmotpek(mote2);
	if(!MayAdminConf(mote2, inloggad, &Servermem->inne[nodnr])) {
		puttekn("\r\n\nDu har inte rätt att subtrahera rättigheter i det här mötet!\r\n\n",-1);
		return;
	}
	if((anv=parsenamn(argument))==-1) {
		puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n",-1);
		return;
	} else if(anv==-3) {
		puttekn("\r\n\nSkriv: Subtrahera rättigheter <användare>\r\n(Se till så att du befinner dig i rätt möte)\r\n\n",-1);
		return;
	}
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==anv) break;
	if(x<MAXNOD) {
		BAMCLEAR(Servermem->inne[x].motratt,motpek->nummer);
		BAMCLEAR(Servermem->inne[x].motmed,motpek->nummer);
		sprintf(outbuffer,"\n\n\rRättigheter i %s subtraherade för %s\n\r",motpek->namn,getusername(anv));
	} else {
		if(readuser(anv,&subuser)) return;
		BAMCLEAR(subuser.motratt,motpek->nummer);
		BAMCLEAR(subuser.motmed,motpek->nummer);
		if(writeuser(anv,&subuser)) return;
		sprintf(outbuffer,"\n\n\rRättigheter i %s subtraherade för %s #%d\n\r",motpek->namn,subuser.namn,anv);
	}
	puttekn(outbuffer,-1);
}
