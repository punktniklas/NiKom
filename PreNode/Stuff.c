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
extern char outbuffer[],inmat[];

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
