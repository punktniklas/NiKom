#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <fifoproto.h>
#include <fifo.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"

#define ERROR	10
#define OK		0
#define EKO		1
#define EJEKO	0
#define KOM		1
#define EJKOM	0

extern struct System *Servermem;
extern int nodnr,inloggad,mote2,senast_text_typ,radcnt;
extern char outbuffer[],inmat[],*argument,usernamebuf[],reggadnamn[];
extern struct Inloggning Statstr;
extern struct Header readhead;
extern struct MinList aliaslist, edit_list;

struct Library *FifoBase;
char gruppnamebuf[41];

int movetext(void) {
  int nummer, newConf, oldConf;
  struct Mote *motpek;
  struct Header movehead;
  if(!isdigit(argument[0])) {
    puttekn("\r\n\nSkriv: Flytta Text <textnummer> <möte>\r\n",-1);
    return(0);
  }
  if((nummer=atoi(argument))<Servermem->info.lowtext || nummer>Servermem->info.hightext) {
    puttekn("\r\n\nTexten finns inte!\r\n",-1);
    return(0);
  }
  argument=hittaefter(argument);
  if(!argument[0]) {
    puttekn("\r\n\nTill vilket möte: ",-1);
    if(getstring(EKO,45,NULL)) return(1);
    if(!inmat[0]) return(0);
    argument=inmat;
  }
  if((newConf = parsemot(argument))==-1) {
    puttekn("\r\n\nMötet finns inte!\r\n",-1);
    return(0);
  }
  oldConf = GetConferenceForText(nummer);
  if(newConf == oldConf) {
    puttekn("\r\n\nTexten är redan i det mötet!\r\n",-1);
    return(0);
  }
  if(readtexthead(nummer,&movehead)) return(0);
  motpek=getmotpek(oldConf);
  if(!motpek) {
    puttekn("\n\n\rTexten är inte i något möte\n\r",-1);
    return(0);
  }
  if(movehead.person != inloggad
     && !MayAdminConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
    puttekn("\r\n\nDu har ingen rätt att flytta den texten\r\n",-1);
    return(0);
  }
  if(!MayWriteConf(newConf, inloggad, &Servermem->inne[nodnr])
     || !MayReplyConf(newConf, inloggad, &Servermem->inne[nodnr])) {
    puttekn("\n\n\rDu har ingen rätt att flytta texten till det mötet.\n\r",-1);
    return(0);
  }
  motpek = getmotpek(newConf);
  sprintf(outbuffer,"\r\n\nFlyttar texten till %s.\r\n",motpek->namn);
  puttekn(outbuffer,-1);
  movehead.mote = newConf;
  if(writetexthead(nummer,&movehead)) return(0);
  SetConferenceForText(nummer, newConf, TRUE);
  return 0;
}

char *getusername(int nummer) {
	struct ShortUser *letpek;
	int found=FALSE;
/*	if(!reggadnamn[0] && nummer>=5) return((char *)0xbadbad42); */
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
	{
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

int namematch(char *pat,char *fac) {
	char *pat2,*fac2;
	if(!matchar(pat,fac)) return(FALSE);
	pat2=hittaefter(pat);
	fac2=hittaefter(fac);
	if(!pat2[0]) return(TRUE);
	if(!matchar(pat2,fac2)) {
		fac2=hittaefter(fac2);
		if(!matchar(pat2,fac2)) return(FALSE);
	}
	return(TRUE);
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

int skapagrupp(void) {
	struct UserGroup *allok,*letpek;
	int x=0, groupadmin, going;
	char buff[9];

	if(Servermem->inne[nodnr].status < Servermem->cfg.st.grupper)
	{
		puttekn("\r\n\nDu har inte rättigheter att skapa grupper!\r\n",-1);
		return(0);
	}

		for(letpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;letpek->grupp_node.mln_Succ;letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ) {
		if(letpek->nummer>x) {
			break;
		}
		x++;
	}
	if(x>=32) {
		puttekn("\r\n\nSorry, det finns redan maximalt med användargrupper.\r\n",-1);
		return(0);
	}
	if(!(allok=AllocMem(sizeof(struct UserGroup),MEMF_CLEAR | MEMF_PUBLIC))) {
		puttekn("\r\n\nKunde inte allokera en UserGroup-struktur.\r\n",-1);
		return(0);
	}
	allok->nummer = x;
	if(!argument[0]) {
		puttekn("\r\n\nNamn: ",-1);
		if(getstring(EKO,40,NULL)) {
			FreeMem(allok,sizeof(struct UserGroup));
			return(1);
		}
		if(!inmat[0]) {
			FreeMem(allok,sizeof(struct UserGroup));
			return(0);
		}
		strcpy(allok->namn,inmat);
	} else strcpy(allok->namn,argument);
	if(parsegrupp(allok->namn) != -1) {
		puttekn("\n\n\rFinns redan en grupp med det namnet.\n\r",-1);
		FreeMem(allok,sizeof(struct UserGroup));
		return(0);
	}
	puttekn("\r\n\nVid vilken statusnivå ska man bli automatiskt medlem i gruppen?\r\n",-1);
	puttekn("0  = alla blir automatiskt medlemmar.\r\n",-1);
	puttekn("-1 = ingen blir automatiskt medlem.\r\n",-1);
	if(getstring(EKO,3,NULL)) {
		FreeMem(allok,sizeof(struct UserGroup));
		return(1);
	}
	allok->autostatus=atoi(inmat);
	puttekn("\r\nSka gruppen vara hemlig? ",-1);
	if(jaellernej('j','n',2)) {
		allok->flaggor |= HEMLIGT;
		puttekn("Ja, hemligt\r\n",-1);
	} else puttekn("Nej, inte hemligt\r\n",-1);

	going = TRUE;
	while(going)
	{
		puttekn("\r\nVilken användare ska vara administratör för denna grupp: ",-1);
		sprintf(buff,"%d",inloggad);
		if(getstring(EKO,40,buff)) return(1);
		if(inmat[0])
		{
			if((groupadmin=parsenamn(inmat))==-1) puttekn("\r\nFinns ingen sådan användare!",-1);
			else
			{
				allok->groupadmin = groupadmin;
				going = FALSE;
			}
		}
	}

	if(writegrupp(x,allok)) {
		FreeMem(allok,sizeof(struct UserGroup));
		return(0);
	}

	Insert((struct List *)&Servermem->grupp_list,(struct Node *)allok,(struct Node *)letpek->grupp_node.mln_Pred);
	BAMSET((char *)&Servermem->inne[nodnr].grupper,x);
	return(0);
}

int writegrupp(int nummer,struct UserGroup *pek) {
	BPTR fh;
	if(!(fh=Open("NiKom:DatoCfg/Grupper.dat",MODE_OLDFILE))) {
		puttekn("\r\nKunde inte öppna Grupper.dat!\r\n",-1);
		return(1);
	}
	if(Seek(fh,nummer*sizeof(struct UserGroup),OFFSET_BEGINNING)==-1) {
		puttekn("\r\nKunde inte söka i Grupper.dat!\r\n",-1);
		Close(fh);
		return(1);
	}
	if(Write(fh,(void *)pek,sizeof(struct UserGroup))==-1) {
		puttekn("\r\nKunde inte skriva i Grupper.dat!\r\n",-1);
		Close(fh);
		return(1);
	}
	Close(fh);
	return(0);
}

void listagrupper(void) {
	struct UserGroup *listpek;
	puttekn("\r\n\n",-1);
	for(listpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;listpek->grupp_node.mln_Succ;listpek=(struct UserGroup *)listpek->grupp_node.mln_Succ) {
		if((listpek->flaggor & HEMLIGT) && !gruppmed(listpek,Servermem->inne[nodnr].status,Servermem->inne[nodnr].grupper && Servermem->inloggad[nodnr] != listpek->groupadmin)) continue;
		if(gruppmed(listpek,Servermem->inne[nodnr].status,Servermem->inne[nodnr].grupper))
			sprintf(outbuffer,"  %s\r\n",listpek->namn);
		else sprintf(outbuffer,"* %s\r\n",listpek->namn);
		if(puttekn(outbuffer,-1)) break;
	}
}

struct UserGroup *getgrouppek(int nummer)
{
	int going=TRUE;
	struct UserGroup *letpek;

	letpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;
	while(letpek->grupp_node.mln_Succ && going)
	{
		if(letpek->nummer == nummer)
			going = FALSE;
		else
			letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ;
	}

	return(letpek);
}

int adderagruppmedlem(void) {
	int x,vem,grupp;
	struct User anv;
	struct ShortUser *letpek;
	struct UserGroup *grouppek;

	if(!argument[0]) {
		puttekn("\r\n\nVem ska adderas? ",-1);
		if(getstring(EKO,40,NULL)) return(1);
		if(!inmat[0]) return(0);
		argument=inmat;
	}
	if((vem=parsenamn(argument))==-1) {
		puttekn("\r\n\nFinns ingen sådan användare!\r\n",-1);
		return(0);
	}
	puttekn("\r\n\nTill vilken grupp? ",-1);
	if(getstring(EKO,40,NULL)) return(1);
	if(!inmat[0]) return(0);
	if((grupp=parsegrupp(inmat))==-1) {
		puttekn("\r\n\nFinns ingen sådan grupp!\r\n",-1);
		return(0);
	}
	grouppek = getgrouppek(grupp);
	if(inloggad != grouppek->groupadmin && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper)
	{
		puttekn("\r\n\nDu har inte rättigheter att addera gruppmedlemmar till denna grupp!\r\n",-1);
		return(0);
	}
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==vem) break;
	if(x==MAXNOD) {
		if(readuser(vem,&anv)) return(0);
		BAMSET((char *)&anv.grupper,grupp);
		if(writeuser(vem,&anv)) return(0);
	} else BAMSET((char *)&Servermem->inne[x].grupper,grupp);
	sprintf(outbuffer,"\r\n\n%s adderad till %s.\r\n",getusername(vem),getgruppname(grupp));
	puttekn(outbuffer,-1);
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
		if(letpek->nummer==vem) break;
	if(letpek->user_node.mln_Succ) BAMSET((char *)&letpek->grupper,grupp);
	else {
		puttekn("\r\n\nHittade inte ShortUser-strukturen!\r\n",-1);
		return(0);
	}
	return(0);
}

int subtraheragruppmedlem(void) {
	int x,vem,grupp;
	struct User anv;
	struct ShortUser *letpek;
	struct UserGroup *grouppek;

	if(!argument[0]) {
		puttekn("\r\n\nVem ska subtraheras? ",-1);
		if(getstring(EKO,40,NULL)) return(1);
		if(!inmat[0]) return(0);
		argument=inmat;
	}
	if((vem=parsenamn(argument))==-1) {
		puttekn("\r\n\nFinns ingen sådan användare!\r\n",-1);
		return(0);
	}
	puttekn("\r\n\nFrån vilken grupp? ",-1);
	if(getstring(EKO,40,NULL)) return(1);
	if(!inmat[0]) return(0);
	if((grupp=parsegrupp(inmat))==-1) {
		puttekn("\r\n\nFinns ingen sådan grupp!\r\n",-1);
		return(0);
	}
	grouppek = getgrouppek(grupp);
	if(inloggad != grouppek->groupadmin && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper)
	{
		puttekn("\r\n\nDu har inte rättigheter att subtrahera gruppmedlemmar från denna grupp!\r\n",-1);
		return(0);
	}
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==vem) break;
	if(x==MAXNOD) {
		if(readuser(vem,&anv)) return(0);
		BAMCLEAR((char *)&anv.grupper,grupp);
		if(writeuser(vem,&anv)) return(0);
	} else BAMCLEAR((char *)&Servermem->inne[x].grupper,grupp);
	sprintf(outbuffer,"\r\n\n%s subtraherad från %s.\r\n",getusername(vem),getgruppname(grupp));
	puttekn(outbuffer,-1);
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
		if(letpek->nummer==vem) break;
	if(letpek->user_node.mln_Succ) BAMCLEAR((char *)&letpek->grupper,grupp);
	else {
		puttekn("\r\n\nHittade inte ShortUser-strukturen!\r\n",-1);
		return(0);
	}
	return(0);
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
		letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ;
	}
	return(found);
}

int andragrupp(void) {
	struct UserGroup *pek,tmp;
	int nummer, heml, going, groupadmin;
	char buff[9];

	if(!argument[0]) {
		puttekn("\r\n\nSkriv: Ändra Grupp <gruppnamn>\r\n",-1);
		return(0);
	}
	if((nummer=parsegrupp(argument))==-1) {
		puttekn("\r\n\nFinns ingen sådan grupp!\r\n",-1);
		return(0);
	}
	for(pek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;pek->grupp_node.mln_Succ;pek=(struct UserGroup *)pek->grupp_node.mln_Succ)
		if(pek->nummer==nummer) break;
	if(!pek->grupp_node.mln_Succ) {
		puttekn("\r\n\nHmm, det var skumt det här..Rapportera tack.\r\n",-1);
		return(0);
	}
	if(inloggad != pek->groupadmin && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper)
	{
		puttekn("\r\n\nDu har inte rättigheter att ändra denna grupp!\r\n",-1);
		return(0);
	}
	memcpy(&tmp,pek,sizeof(struct UserGroup));
	sprintf(outbuffer,"\r\n\nNamn (%s) : ",tmp.namn);
	puttekn(outbuffer,-1);
	if(getstring(EKO,40,NULL)) return(1);
	if(inmat[0]) strcpy(tmp.namn,inmat);
	puttekn("\r\n\nVid vilken statusnivå ska man bli automatiskt medlem i gruppen?\r\n",-1);
	puttekn("0  = alla blir automatiskt medlemmar.\r\n",-1);
	puttekn("-1 = ingen blir automatiskt medlem.\r\n",-1);
	sprintf(outbuffer,"(%d) : ",tmp.autostatus);
	puttekn(outbuffer,-1);
	if(getstring(EKO,3,NULL)) return(1);
	if(inmat[0]) tmp.autostatus=atoi(inmat);
	puttekn("\r\nSka gruppen vara hemlig? ",-1);
	if(tmp.flaggor & HEMLIGT) heml=jaellernej('j','n',1);
	else heml=jaellernej('j','n',2);
	if(heml) {
		tmp.flaggor |= HEMLIGT;
		puttekn("Ja, hemligt\r\n",-1);
	} else {
		tmp.flaggor &= ~HEMLIGT;
		puttekn("Nej, inte hemligt\r\n",-1);
	}
	going = TRUE;
	while(going)
	{
		puttekn("\r\nVilken användare ska vara administratör för denna grupp: ",-1);
		sprintf(buff,"%d",pek->groupadmin);
		if(getstring(EKO,40,buff)) return(1);
		if(inmat[0])
		{
			if((groupadmin=parsenamn(inmat))==-1) puttekn("\r\nFinns ingen sådan användare!",-1);
			else
			{
				tmp.groupadmin = groupadmin;
				going = FALSE;
			}
		}
	}
	puttekn("\r\nÄr allt korrekt? ",-1);
	if(!jaellernej('j','n',1)) {
		puttekn("Nej\r\n",-1);
		return(0);
	}
	puttekn("Ja\r\n",-1);
	memcpy(pek,&tmp,sizeof(struct UserGroup));
	writegrupp(nummer,pek);
	return(0);
}

void raderagrupp(void) {
	struct UserGroup *pek;
	int nummer;
	if(!argument[0]) {
		puttekn("\r\n\nSkriv: Radera Grupp <grupp>\r\n",-1);
		return;
	}
	if((nummer=parsegrupp(argument))==-1) {
		puttekn("\r\n\nFinns ingen sådan grupp!\r\n",-1);
		return;
	}
	for(pek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;pek->grupp_node.mln_Succ;pek=(struct UserGroup *)pek->grupp_node.mln_Succ)
		if(pek->nummer==nummer) break;
	if(!pek->grupp_node.mln_Succ) {
		puttekn("\r\n\nHmm, det var skumt det här..Rapportera tack.\r\n",-1);
		return;
	}
	if(inloggad != pek->groupadmin && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper)
	{
		puttekn("\r\n\nDu har inte rättigheter att radera denna grupp!\r\n",-1);
		return;
	}
	sprintf(outbuffer,"\r\n\nRadera gruppen %s? ",pek->namn);
	puttekn(outbuffer,-1);
	if(!jaellernej('j','n',2)) {
		puttekn("Nej\r\n",-1);
		return;
	}
	puttekn("Ja\r\n",-1);
	Remove((struct Node *)pek);
	pek->namn[0]=0;
	writegrupp(pek->nummer,pek);
	FreeMem(pek,sizeof(struct UserGroup));
	return;
}

void initgrupp(void) {
	struct UserGroup *pek;
	for(pek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;pek->grupp_node.mln_Succ;pek=(struct UserGroup *)pek->grupp_node.mln_Succ) {
		if(pek->autostatus!=-1) if(Servermem->inne[nodnr].status>=pek->autostatus) BAMSET((char *)&Servermem->inne[nodnr].grupper,pek->nummer);
	}
}

char *getgruppname(int nummer) {
	struct UserGroup *letpek;
	int found=FALSE;
	for(letpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;letpek->grupp_node.mln_Succ;letpek=(struct UserGroup *)letpek->grupp_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			if(letpek->namn[0]) strcpy(gruppnamebuf,letpek->namn);
			else strcpy(gruppnamebuf,"<Raderad Grupp>");
			found=TRUE;
			break;
		}
	}
	if(!found) strcpy(gruppnamebuf,"<Felaktigt gruppnummer>");
	return(gruppnamebuf);
}

int editgrupp(char *bitmap) {
	int grupp;
	struct UserGroup *pek;
	do {
		if(getstring(EKO,40,NULL)) return(1);
		if(inmat[0]=='?') {
			listagrupper();
			puttekn("'!' för att se vilka grupper du har satt.\r\n",-1);
		} else if(inmat[0]=='!') {
			puttekn("\r\nFöljande grupper är satta:\r\n",-1);
			for(pek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;pek->grupp_node.mln_Succ;pek=(struct UserGroup *)pek->grupp_node.mln_Succ)
				if(BAMTEST(bitmap,pek->nummer)) {
				sprintf(outbuffer,"%s\r\n",pek->namn);
				puttekn(outbuffer,-1);
			}
		} else if(inmat[0]!=0) {
			if((grupp=parsegrupp(inmat))==-1) puttekn("\r\nFinns ingen sådan grupp\r\n",-1);
			else if(grupp>=0) {
				sprintf(outbuffer,"\r%s\r\n",getgruppname(grupp));
				puttekn(outbuffer,-1);
				if(BAMTEST(bitmap,grupp)) BAMCLEAR(bitmap,grupp);
				else BAMSET(bitmap,grupp);
			}
		}
	} while(inmat[0]!=0);
	return(0);
}

void motesstatus(void) {
	int motnr;
	struct Mote *motpek;
	struct tm *ts;
	struct UserGroup *pek;
	char filnamn[50];
	if(!argument[0]) {
		if(mote2!=-1) motnr=mote2;
		else {
			puttekn("\r\n\nSkriv: Mötesstatus <möte>\r\n",-1);
			return;
		}
	} else {
		motnr=parsemot(argument);
		if(motnr==-1) {
			puttekn("\r\n\nFinns inget sådant möte!\r\n",-1);
			return;
		}
	}
	motpek=getmotpek(motnr);
	sprintf(outbuffer,"\r\n\nNamn:            %s",motpek->namn);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"\r\nSkapat av:       %s",getusername(motpek->skapat_av));
	puttekn(outbuffer,-1);
	ts=localtime(&motpek->skapat_tid);
	sprintf(outbuffer,"\r\nSkapat tid:      %4d%02d%02d  %02d:%02d",
                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
                ts->tm_min);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"\r\nMötesnummer:     %d",motpek->nummer);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"\r\nSorteringsvärde: %d",motpek->sortpri);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"\r\nMAD:             %s",getusername(motpek->mad));
	puttekn(outbuffer,-1);
	puttekn("\r\nMötestyp:        ",-1);
	if(motpek->type == MOTE_ORGINAL) puttekn("lokalt möte.",-1);
	else if(motpek->type == MOTE_FIDO)
	{
	          puttekn("FidoNet möte.",-1);
		sprintf(outbuffer,"\r\nTagnamn:         %s",motpek->tagnamn);
		puttekn(outbuffer,-1);
		sprintf(outbuffer,"\r\nBibliotek:       %s",motpek->dir);
		puttekn(outbuffer,-1);
		sprintf(outbuffer,"\r\nOrigin:          %s\r\n",motpek->origin);
		puttekn(outbuffer,-1);
	}
	else
		puttekn("\r\n", -1);

/*
struct Mote {
   struct MinNode mot_node;
   long skapat_tid,grupper,skapat_av,mad,sortpri,lowtext,domain,texter, renumber_offset,
  	   reserv1, reserv2, reserv3, reserv4, reserv5;
   short gren,status,charset,nummer;
   char type,namn[41],origin[70],tagnamn[50],dir[80];
};
*/
	if(motpek->status & KOMSKYDD) puttekn("Kommentarsskyddat  ",-1);
	if(motpek->status & SKRIVSKYDD) puttekn("Skrivskyddat  ",-1);
	if(motpek->status & SLUTET) puttekn("Slutet  ",-1);
	if(motpek->status & HEMLIGT) puttekn("Hemligt  ",-1);
	if(motpek->status & AUTOMEDLEM) puttekn("Auto ",-1);
	if(motpek->status & SKRIVSTYRT) puttekn("Skrivstyrt  ",-1);
	if(motpek->status & SUPERHEMLIGT) puttekn("ARexx-åtkomst  ",-1);
	puttekn("\r\n\n",-1);
	if((motpek->status & (SLUTET | SKRIVSTYRT)) && motpek->grupper) {
		puttekn("Följande grupper har tillgång till mötet:\r\n",-1);
		for(pek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;pek->grupp_node.mln_Succ;pek=(struct UserGroup *)pek->grupp_node.mln_Succ) {
			if(!BAMTEST((char *)&motpek->grupper,pek->nummer)) continue;
			if((pek->flaggor & HEMLIGT) && !gruppmed(pek,Servermem->inne[nodnr].status,Servermem->inne[nodnr].grupper)) continue;
			sprintf(outbuffer,"%s\r\n",pek->namn);
			puttekn(outbuffer,-1);
		}
	}
	sprintf(filnamn,"NiKom:Lappar/%d.motlapp",motnr);
	if(!access(filnamn,0)) sendfile(filnamn);
}

void hoppaarende(void) {
	struct Header hoppahead;
	int hoppade=0, nextUnread;
	if(!argument[0]) {
		if(senast_text_typ!=TEXT) {
			puttekn("\r\n\nSkriv: Hoppa Ärende <början av ärendet>\r\n",-1);
			return;
		} else argument=readhead.arende;
	}
	if(mote2==-1) {
		sprintf(outbuffer,"\r\n\nDu kan inte utföra Hoppa Ärende i %s!\r\n",Servermem->cfg.brevnamn);
		puttekn(outbuffer,-1);
		return;
	}
	if(strlen(argument)>40) {
		puttekn("\r\n\nEtt ärende kan inte vara så långt...\r\n",-1);
		return;
	}
        nextUnread = -1;
        while((nextUnread = FindNextUnreadText(nextUnread + 1, mote2,
            &Servermem->unreadTexts[nodnr])) != -1) {
          if(readtexthead(nextUnread, &hoppahead)) {
            return;
          }
          if(!strncmp(hoppahead.arende,argument,strlen(argument))) {
            ChangeUnreadTextStatus(nextUnread, 0, &Servermem->unreadTexts[nodnr]);
            hoppade++;
          }
	}
	if(!hoppade) puttekn("\r\n\nInga texter överhoppade.\r\n",-1);
	else if(hoppade==1) puttekn("\r\n\n1 text överhoppad.\r\n",-1);
	else {
		sprintf(outbuffer,"\r\n\n%d texter överhoppade.\r\n",hoppade);
		puttekn(outbuffer,-1);
	}
}

int arearatt(int area, int usrnr, struct User *usr)
{
	int x=0;
	if(usr->status>=Servermem->cfg.st.bytarea) return(1);
	if(Servermem->areor[area].mote==-1 || MayBeMemberConf(Servermem->areor[area].mote, usrnr, usr)) x++;
	if(!Servermem->areor[area].grupper || (Servermem->areor[area].grupper & usr->grupper)) x++;
	return(x==2 ? 1 : 0);
}

int gruppmed(struct UserGroup *grupp,char status,long grupper) {
	if(status>=Servermem->cfg.st.medmoten) return(1);
	if(status >= grupp->autostatus) return(1);
	if(BAMTEST((char *)&grupper,grupp->nummer)) return(1);
	return(0);
}

void listgruppmed(void) {
	struct ShortUser *listpek;
	int grupp;
	if(!argument[0]) {
		puttekn("\r\n\nSkriv: Lista Medlemmar -g <grupp>\r\n",-1);
		return;
	}
	if((grupp=parsegrupp(argument))==-1) {
		puttekn("\r\n\nFinns ingen sådan grupp!\r\n",-1);
		return;
	}
	puttekn("\r\n\n",-1);
	for(listpek=(struct ShortUser *)Servermem->user_list.mlh_Head;listpek->user_node.mln_Succ;listpek=(struct ShortUser *)listpek->user_node.mln_Succ) {
		if(BAMTEST((char *)&listpek->grupper,grupp)) {
			sprintf(outbuffer,"%s #%d\r\n",listpek->namn,listpek->nummer);
			if(puttekn(outbuffer,-1)) return;
		}
	}
}

struct Alias *parsealias(char *skri) {
	struct Alias *pek;
	int x;
	for(pek=(struct Alias *)aliaslist.mlh_Head;pek->alias_node.mln_Succ;pek=(struct Alias *)pek->alias_node.mln_Succ)
	{
		for(x=0;pek->namn[x]!=' ' && pek->namn[x]!=0;x++);
		if(!strnicmp(skri,pek->namn,x) && (skri[x]==' ' || skri[x]==0)) return(pek);
/*		if(!strnicmp(skri,pek->namn,strlen(pek->namn))) return(pek); */
	}
	return(NULL);
}

void listalias(void) {
	struct Alias *pek;
	puttekn("\r\n\n",-1);
	for(pek=(struct Alias *)aliaslist.mlh_Head;pek->alias_node.mln_Succ;pek=(struct Alias *)pek->alias_node.mln_Succ) {
		sprintf(outbuffer,"%-21s = %s\r\n",pek->namn,pek->blirtill);
		puttekn(outbuffer,-1);
	}
}

void remalias(void) {
	struct Alias *rempek;
	if(!(rempek=parsealias(argument))) {
		puttekn("\r\n\nFinns inget sådant alias definierat!\r\n",-1);
		return;
	}
	Remove((struct Node *)rempek);
	FreeMem(rempek,sizeof(struct Alias));
}

void defalias(void) {
	struct Alias *defpek;
	char *andra, *pek;
	andra=hittaefter(argument);
	pek = &argument[0];
	while(pek[0] != ' ' && pek[0]) pek++;
	pek[0] = NULL;
	if(defpek=parsealias(argument)) {
		strncpy(defpek->blirtill,andra,40);
	} else {
		if(!(defpek=AllocMem(sizeof(struct Alias),MEMF_CLEAR))) {
			puttekn("\r\n\nKunde inte allokera minne till en alias-struktur!\r\n",-1);
			return;
		}
		strncpy(defpek->namn,argument,20);
		strncpy(defpek->blirtill,andra,40);
		AddTail((struct List *)&aliaslist,(struct Node *)defpek);
	}
}

void alias(void) {
	char *pekare=hittaefter(argument);
	if(!argument[0]) listalias();
	else if(!pekare[0]) remalias();
	else defalias();
}

int readtextlines(char typ, long pos, int rader, int nummer) {
	FILE *fp;
	struct EditLine *el;
	int x,fil;
	char rlbuf[100],filnamn[40];
	fil=nummer/512;
	NewList((struct List *)&edit_list);
	NiKForbid();
	if(typ==TEXT) sprintf(filnamn,"NiKom:Moten/Text%d.dat",fil);
	else puttekn("\n\n\rAj! Buggelibugg! Försöker läsa BrevText.dat\n\r",-1);
	if(!(fp=fopen(filnamn,"r"))) {
		NiKPermit();
		return(1);
	}
	if(fseek(fp,pos,0)) {
		fclose(fp);
		NiKPermit();
		return(1);
	}
	for(x=0;x<rader;x++) {
		if(!fgets(rlbuf,MAXFULLEDITTKN+1,fp))
		{
			fclose(fp);
			NiKPermit();
			return(1);
		}
		if(!(el=(struct EditLine *)AllocMem(sizeof(struct EditLine),MEMF_CLEAR|MEMF_PUBLIC))) {
			fclose(fp);
			NiKPermit();
			return(1);
		}
		strcpy(el->text,rlbuf);
		AddTail((struct List *)&edit_list,(struct Node *)el);
	}
	fclose(fp);
	NiKPermit();
	return(0);
}

void freeeditlist(void) {
	struct EditLine *el;
	while(el=(struct EditLine *)RemHead((struct List *)&edit_list))
		FreeMem(el,sizeof(struct EditLine));
	NewList((struct List *)&edit_list);
}

int rek_flyttagren(int rot,int ack,int tomeet) {
	static struct Header flyttahead;
	int x=0;
	long kom_i[MAXKOM];
	if(readtexthead(rot,&flyttahead)) return(ack);
	memcpy(kom_i,flyttahead.kom_i,MAXKOM*sizeof(long));
	flyttahead.mote=tomeet;
	if(writetexthead(rot,&flyttahead)) return(ack);
        SetConferenceForText(rot, tomeet, TRUE);
	ack++;
	while(kom_i[x]!=-1 && x<MAXKOM) {
		ack=rek_flyttagren(kom_i[x],ack,tomeet);
		x++;
	}
	return(ack);
}

void flyttagren(void) {
	char *meetnamn=hittaefter(argument);
	int rot,motnr;
	if(!argument[0] || !meetnamn[0]) {
		puttekn("\n\r\rSkriv: Flytta Gren <rot-textnr> <möte>\n\r",-1);
		return;
	}
	rot=atoi(argument);
	if(rot<Servermem->info.lowtext || rot>Servermem->info.hightext) {
		puttekn("\n\n\rTexten finns inte!\n\r",-1);
		return;
	}
	motnr=parsemot(meetnamn);
	if(motnr==-1) {
		puttekn("\n\n\rFinns inget sådant möte\n\r",-1);
		return;
	}
	sprintf(outbuffer,"\n\n\rFlyttade %d texter\n\r",rek_flyttagren(rot,0,motnr));
	puttekn(outbuffer,-1);
}

#define FIFOEVENT_FROMUSER  1
#define FIFOEVENT_FROMFIFO  2
#define FIFOEVENT_CLOSEW    4
#define FIFOEVENT_NOCARRIER 8

int execfifo(char *command,int cooked) {
	struct MsgPort *fifoport;
	void *fiforead, *fifowrite;
	BPTR fh;
	int avail=0,going=TRUE,event;
	char fifonamn[40],*buffer=NULL,userchar,topsbuf[200];
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
	if(cooked) sprintf(fifonamn,"FIFO:NiKomFifo%d/rwkecs",nodnr);
	else sprintf(fifonamn,"FIFO:NiKomFifo%d/rwkes",nodnr);
	if(!(fh=Open(fifonamn,MODE_OLDFILE))) {
		puttekn("\n\n\rKunde inte öppna fifo-filen\n\r",-1);
		CloseFifo(fifowrite,FIFOF_EOF);
		CloseFifo(fiforead,FIFOF_EOF);
		DeleteMsgPort(fifoport);
		CloseLibrary(FifoBase);
		return(0);
	}
	if(SystemTags(command,SYS_Asynch,TRUE,
	                      SYS_Input,fh,
	                      SYS_Output,NULL,
	                      SYS_UserShell,TRUE)==-1) {
		puttekn("\n\n\rKunde inte utföra kommandot\n\r",-1);
		Close(fh);
		CloseFifo(fifowrite,FIFOF_EOF);
		CloseFifo(fiforead,FIFOF_EOF);
		DeleteMsgPort(fifoport);
		CloseLibrary(FifoBase);
		return(0);
	}
	puttekn("\n\n\r",-1);
	abortinactive();
	RequestFifo(fiforead,&fiforeadmess,FREQ_RPEND);
	while(going) {
		event=getfifoevent(fifoport,&userchar);
		if(event & FIFOEVENT_FROMFIFO) {
			while(avail=ReadFifo(fiforead,&buffer,avail)) {
				if(avail==-1) {
					going=FALSE;
					break;
				} else {
					if(avail>199) avail=199;
					strncpy(topsbuf,buffer,avail);
					topsbuf[avail] = 0;
					putstring(topsbuf,-1,0);
				}
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
							going=FALSE;
							break;
						} else {
							if(avail>199) avail=199;
							strncpy(topsbuf,buffer,avail);
							topsbuf[avail] = 0;
							putstring(topsbuf,-1,0);
						}
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
			sprintf(fifonamn,"FIFO:NiKomFifo%d/C",nodnr);
			if(fh=Open(fifonamn,MODE_OLDFILE)) Close(fh);
else printf("Ajajajaj!\n");
			CloseFifo(fifowrite,FIFOF_EOF);
			CloseFifo(fiforead,FIFOF_EOF);
			DeleteMsgPort(fifoport);
			CloseLibrary(FifoBase);
			cleanup(OK,"");
		}
		if(event & FIFOEVENT_NOCARRIER) going = FALSE;
		if(carrierdropped()) going=FALSE;
	}
	RequestFifo(fiforead,&fiforeadmess,FREQ_ABORT);
	WaitPort(fifoport);
	GetMsg(fifoport);

/* Att skicka ett Ctrl-C funkade inge vidare. Det hamnade hos NiKom själv
*  istället för hos dörren.
*
*	sprintf(fifonamn,"FIFO:NiKomFifo%d/C",nodnr);
*	if(fh=Open(fifonamn,MODE_OLDFILE)) Close(fh); */

	if(carrierdropped())
	{
		WriteFifo(fifowrite, " ", 1);
		sprintf(fifonamn,"FIFO:NiKomFifo%d/C",nodnr);
		if(fh=Open(fifonamn,MODE_OLDFILE)) Close(fh);
	}
	CloseFifo(fifowrite,FIFOF_EOF);
	/* Changed FIFOF_EOF --> FALSE */
	CloseFifo(fiforead,FALSE);
	DeleteMsgPort(fifoport);
	CloseLibrary(FifoBase);
	if(carrierdropped()) return(1);
	updateinactive();
	return(0);
}
