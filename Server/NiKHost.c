#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NiKomStr.h"
#include "ServerFuncs.h"
#include "NiKomLib.h"
#include "Config.h"
#include "RexxUtils.h"
#include "StringUtils.h"

extern struct System *Servermem;

char usernamebuf[50];

void handlerexx(struct RexxMsg *mess) {
	if(!strcmp(mess->rm_Args[0],"USERINFO")) userinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"MEETINFO")) motesinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGMEET")) chgmote(mess);
	else if(!stricmp(mess->rm_Args[0],"SYSINFO")) sysinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"NIKPARSE")) nikparse(mess);
	else if(!stricmp(mess->rm_Args[0],"LASTLOGIN")) senaste(mess);
	else if(!stricmp(mess->rm_Args[0],"COMMANDINFO")) kominfo(mess);
	else if(!stricmp(mess->rm_Args[0],"FILEINFO")) filinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"AREAINFO")) areainfo(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGUSER")) chguser(mess);
	else if(!stricmp(mess->rm_Args[0],"CREATEFILE")) skapafil(mess);
	else if(!stricmp(mess->rm_Args[0],"READCONFIG")) rexxreadcfg(mess);
	else if(!stricmp(mess->rm_Args[0],"NODEINFO")) rexxnodeinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"NEXTFILE")) rexxnextfile(mess);
	else if(!stricmp(mess->rm_Args[0],"DELETEFILE")) rexxraderafil(mess);
	else if(!stricmp(mess->rm_Args[0],"DELNIKFILE")) rexxraderafil(mess);
	else if(!stricmp(mess->rm_Args[0],"CREATETEXT")) createtext(mess);
	else if(!stricmp(mess->rm_Args[0],"TEXTINFO")) textinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"NEXTUNREAD")) nextunread(mess);
	else if(!stricmp(mess->rm_Args[0],"CREATELETTER")) createletter(mess);
	else if(!stricmp(mess->rm_Args[0],"MEETRIGHT")) meetright(mess);
	else if(!stricmp(mess->rm_Args[0],"MEETMEMBER")) meetmember(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGMEETRIGHT")) chgmeetright(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGFILE")) chgfile(mess);
	else if(!stricmp(mess->rm_Args[0],"KEYINFO")) keyinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"GETDIR")) getdir(mess);
	else if(!stricmp(mess->rm_Args[0],"DELOLDTEXTS")) rexxPurgeOldTexts(mess);
	else if(!stricmp(mess->rm_Args[0],"SENDNODEMESS")) rxsendnodemess(mess);
	else if(!stricmp(mess->rm_Args[0],"STATUSINFO")) rexxstatusinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"AREARIGHT")) rexxarearight(mess);
	else if(!stricmp(mess->rm_Args[0],"SYSSETTINGS")) rexxsysteminfo(mess);
	else if(!stricmp(mess->rm_Args[0],"MOVEFILE")) movefile(mess);
	else if(!stricmp(mess->rm_Args[0],"MARKTEXTREAD")) rexxmarktextread(mess);
	else if(!stricmp(mess->rm_Args[0],"MARKTEXTUNREAD")) rexxmarktextunread(mess);
	else if(!stricmp(mess->rm_Args[0],"NEXTPATTERNFILE")) rexxnextpatternfile(mess);
	else if(!stricmp(mess->rm_Args[0],"CONSOLETEXT")) rexxconsoletext(mess);
	else if(!stricmp(mess->rm_Args[0],"CHECKUSERPASSWORD")) rexxcheckuserpassword(mess);
	else {
		mess->rm_Result1=10;
		mess->rm_Result2=1L;
	}
}

int writefiles(int area)
{
	struct Node *nod;
	BPTR fh;
	char datafil[110];
	int index;

	ObtainSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

	sprintf(datafil,"nikom:datocfg/areor/%d.dat",area);

	if(!(fh=Open(datafil,MODE_NEWFILE)))
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		return 1;
	}

	index=0;
	nod = ((struct List *)&Servermem->areor[area].ar_list)->lh_Head;
	while(nod->ln_Succ)
	{
		if(Write(fh,nod,sizeof(struct DiskFil)) != sizeof(struct DiskFil)) {
			Close(fh);
			ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
			return 3;
		}
		((struct Fil *)nod)->index = index++;
		nod = nod->ln_Succ;
	}
	Close(fh);

	ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

	return 0;
}

int updatefile(int area,struct Fil *fil)
{
	BPTR fh;
	char datafil[110];

	ObtainSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

	sprintf(datafil,"nikom:datocfg/areor/%d.dat",area);

	if(!(fh=Open(datafil,MODE_OLDFILE)))
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		return 1;
	}

	if(Seek(fh,fil->index * sizeof(struct DiskFil),OFFSET_BEGINNING)==-1)
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		Close(fh);
		return 2;
	}

	if(Write(fh,fil,sizeof(struct DiskFil)) != sizeof(struct DiskFil))
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		Close(fh);
		return 3;
	}

	Seek(fh,0,OFFSET_END);
	Close(fh);
	ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
	return 0;
}

int readuser(int nummer, struct User *user) {
	BPTR fh;
	int x;
	char filnamn[40];
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==nummer) break;
	if(x!=MAXNOD) {
		memcpy(user,&Servermem->inne[x],sizeof(struct User));
		return(0);
	}
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",nummer/100,nummer);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		printf("Kunde inte öppna Users.dat!\n");
		return(1);
	}
	if(!Read(fh,(void *)user,sizeof(struct User))) {
		printf("Kunde inte läsa Users.dat!\n");
		Close(fh);
		return(1);
	}
	Close(fh);
	return(0);
}

int writeuser(int nummer, struct User *user) {
	BPTR fh;
	char filnamn[40];
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",nummer/100,nummer);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		printf("Kunde inte öppna Users.dat!\n");
		return(1);
	}
	if(!Write(fh,(void *)user,sizeof(struct User))) {
		printf("Kunde inte skriva Users.dat!\n");
		Close(fh);
		return(1);
	}
	Close(fh);
	return(0);
}

void userinfo(struct RexxMsg *mess) {
	int nummer, nodnr, usrloggedin = 1;
	char str[100];
	struct tm *ts;
	struct User userinfouser;

	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	if(!userexists(nummer=atoi(mess->rm_Args[1]))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	if(mess->rm_Args[2][0]!='n' && mess->rm_Args[2][0]!='N' && mess->rm_Args[2][0]!='r'  && mess->rm_Args[2][0]!='R') {
		for(nodnr=0;nodnr<MAXNOD;nodnr++) if(Servermem->inloggad[nodnr]==nummer) break;

		if(nodnr>=MAXNOD)
		{
			if(readuser(nummer,&userinfouser))
			{
				mess->rm_Result1=3;
				mess->rm_Result2=NULL;
				return;
			}
			usrloggedin = 0;
		}
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].annan_info);
			else
				strcpy(str,userinfouser.annan_info);
			break;
		case 'b' : case 'B' :
			if(usrloggedin)
				ts = localtime(&Servermem->inne[nodnr].forst_in);
			else
				ts=localtime(&userinfouser.forst_in);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'c' : case 'C' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].land);
			else
				strcpy(str,userinfouser.land);
			break;
		case 'd' : case 'D' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].download);
			else
				sprintf(str,"%d",userinfouser.download);
			break;
		case 'e' : case 'E' :
			if(usrloggedin)
				ts=localtime(&Servermem->inne[nodnr].senast_in);
			else
				ts=localtime(&userinfouser.senast_in);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'f' : case 'F' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].telefon);
			else
				strcpy(str,userinfouser.telefon);
			break;
		case 'g' : case 'G' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].gata);
			else
				strcpy(str,userinfouser.gata);
			break;
		case 'h' : case 'H' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].downloadbytes);
			else
				sprintf(str,"%d",userinfouser.downloadbytes);
			break;
		case 'j' : case 'J' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].uploadbytes);
			else
				sprintf(str,"%d",userinfouser.uploadbytes);
			break;
		case 'i' : case 'I' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].loggin);
			else
				sprintf(str,"%d",userinfouser.loggin);
			break;
		case 'l' : case 'L' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].read);
			else
				sprintf(str,"%d",userinfouser.read);
			break;
		case 'm' : case 'M' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].prompt);
			else
				strcpy(str,userinfouser.prompt);
			break;
		case 'n' : case 'N' :
			strcpy(str,getusername(nummer));
			break;
		case 'o' : case 'O' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].flaggor);
			else
				sprintf(str,"%d",userinfouser.flaggor);
			break;
		case 'p' : case 'P' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].postadress);
			else
				strcpy(str,userinfouser.postadress);
			break;
		case 'q' : case 'Q' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].rader);
			else
				sprintf(str,"%d",userinfouser.rader);
			break;
		case 'r' : case 'R' :
			sprintf(str,"%d",getuserstatus(nummer));
			break;
		case 's' : case 'S' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].skrivit);
			else
				sprintf(str,"%d",userinfouser.skrivit);
			break;
		case 't' : case 'T' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].tot_tid);
			else
				sprintf(str,"%d",userinfouser.tot_tid);
			break;
		case 'u' : case 'U' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].upload);
			else
				sprintf(str,"%d",userinfouser.upload);
			break;
		case 'x' : case 'X' :
			if(usrloggedin)
				strcpy(str,Servermem->inne[nodnr].losen);
			else
				strcpy(str,userinfouser.losen);
			break;
		case 'y' : case 'Y' :
			if(usrloggedin)
				sprintf(str,"%d,%d",(unsigned int)Servermem->inne[nodnr].grupper/65536,(unsigned int)Servermem->inne[nodnr].grupper%65536);
			else
				sprintf(str,"%d,%d",(unsigned int)userinfouser.grupper/65536,(unsigned int)userinfouser.grupper%65536);
			break;
		case 'z' : case 'Z' :
			if(usrloggedin)
				sprintf(str,"%d",Servermem->inne[nodnr].brevpek);
			else
				sprintf(str,"%d",userinfouser.brevpek);
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}


void motesinfo(struct RexxMsg *mess) {
	int nummer;
	struct Mote *motpek;
	char str[100];
	struct tm *ts;
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	motpek=getmotpek(nummer=atoi(mess->rm_Args[1]));
	if(!motpek && nummer != -1) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%d",motpek->skapat_av);
			break;
		case 'c' : case 'C' :
			sprintf(str,"%d",motpek->charset);
			break;
		case 'd' : case 'D' :
			sprintf(str,motpek->dir);
			break;
		case 'f' : case 'F' :
			strcpy(str,motpek->tagnamn);
			break;
		case 'g' : case 'G' :
			sprintf(str,"%d,%d",(unsigned int)motpek->grupper/65536,(unsigned int)motpek->grupper%65536);
			break;
		case 'h' : case 'H' :
			sprintf(str,"%d",motpek->texter);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%d",motpek->lowtext);
			break;
		case 'm' : case 'M' :
			sprintf(str,"%d",motpek->mad);
			break;
		case 'n' : case 'N' :
			if(nummer != -1)
				strcpy(str,motpek->namn);
			else
				strcpy(str,Servermem->cfg.brevnamn);
			break;
		case 'o' : case 'O' :
			strcpy(str,motpek->origin);
			break;
		case 'p' : case 'P' :
			sprintf(str,"%d",motpek->sortpri);
			break;
		case 'r' : case 'R' :
			sprintf(str,"%d",motpek->renumber_offset);
			break;
		case 's' : case 'S' :
			sprintf(str,"%d",motpek->status);
			break;
		case 't' : case 'T' :
			ts=localtime(&motpek->skapat_tid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour,ts->tm_min);
			break;
		case 'y' : case 'Y' :
			sprintf(str,"%d",motpek->type);
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void chgmote(struct RexxMsg *mess)
{
	int nummer, ok = 0;
	struct Mote *motpek;

	if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3]))
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	motpek=getmotpek(nummer=atoi(mess->rm_Args[1]));
	if(!motpek) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			ok = 1;
			motpek->skapat_av = atoi(mess->rm_Args[3]);
			break;
		case 'c' : case 'C' :
			ok = 1;
			motpek->charset = atoi(mess->rm_Args[3]);
			break;
		case 'd' : case 'D' :
			ok = 1;
			strncpy(motpek->dir, mess->rm_Args[3], 79);
			break;
		case 'f' : case 'F' :
			ok = 1;
			strncpy(motpek->tagnamn, mess->rm_Args[3], 49);
			break;
		case 'h' : case 'H' :
			ok = 1;
			motpek->texter = atoi(mess->rm_Args[3]);
			break;
		case 'l' : case 'L' :
			ok = 1;
			motpek->lowtext = atoi(mess->rm_Args[3]);
			break;
		case 'm' : case 'M' :
			ok = 1;
			motpek->mad = atoi(mess->rm_Args[3]);
			break;
		case 'n' : case 'N' :
			ok = 1;
			strncpy(motpek->namn, mess->rm_Args[3], 40);
			break;
		case 'o' : case 'O' :
			ok = 1;
			strncpy(motpek->origin, mess->rm_Args[3], 69);
			break;
		case 'p' : case 'P' :
			motpek->sortpri = atoi(mess->rm_Args[3]);
			ok = 1;
			break;
		case 'r' : case 'R' :
			ok = 1;
			motpek->renumber_offset = atoi(mess->rm_Args[3]);
			break;
		case 's' : case 'S' :
			ok = 1;
			motpek->status = atoi(mess->rm_Args[3]);
			break;
		case 'y' : case 'Y' :
			ok = 1;
			motpek->type = atoi(mess->rm_Args[3]);
			break;
		default :
			if(!(mess->rm_Result2=(long)CreateArgstring("-2",2)))
				printf("Kunde inte allokera en ArgString\n");
			mess->rm_Result1=0;
			return;
			break;
	}

	if(ok)
		writemeet(motpek);

	if(!(mess->rm_Result2=(long)CreateArgstring("1",strlen("1"))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void writemeet(struct Mote *motpek)
{
	BPTR fh;
	if(!(fh=Open("NiKom:DatoCfg/Möten.dat",MODE_OLDFILE))) {
		printf("\r\n\nKunde inte öppna Möten.dat\r\n");
		return;
	}
	if(Seek(fh,motpek->nummer*sizeof(struct Mote),OFFSET_BEGINNING)==-1) {
		printf("\r\n\nKunde inte söka i Moten.dat!\r\n\n");
		Close(fh);
		return;
	}
	if(Write(fh,(void *)motpek,sizeof(struct Mote))==-1) {
		printf("\r\n\nFel vid skrivandet av Möten.dat\r\n");
	}
	Close(fh);
}

void nikparse(struct RexxMsg *mess) {
	char str[10];
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'k' : case 'K' :
			sprintf(str,"%d",parse(mess->rm_Args[1]));
			break;
		case 'm' : case 'M' :
			sprintf(str,"%d",parsemot(mess->rm_Args[1]));
			break;
		case 'n' : case 'N' :
			sprintf(str,"%d",parsenamn(mess->rm_Args[1]));
			break;
		case 'a' : case 'A' :
			sprintf(str,"%d",parsearea(mess->rm_Args[1]));
			break;
		case 'y' : case 'Y' :
			sprintf(str,"%d",parsenyckel(mess->rm_Args[1]));
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void senaste(struct RexxMsg *mess) {
	int nummer;
	struct tm *ts;
	char str[100];
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	if((nummer=atoi(mess->rm_Args[1]))<0 || nummer>=MAXSENASTE) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!Servermem->senaste[nummer].utloggtid) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!userexists(Servermem->senaste[nummer].anv)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%d",Servermem->senaste[nummer].anv);
			break;
		case 'd' : case 'D' :
			sprintf(str,"%d",Servermem->senaste[nummer].dl);
			break;
		case 'g' : case 'G' :
			ts=localtime(&Servermem->senaste[nummer].utloggtid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%d",Servermem->senaste[nummer].read);
			break;
		case 's' : case 'S' :
			sprintf(str,"%d",Servermem->senaste[nummer].write);
			break;
		case 't' : case 'T' :
			sprintf(str,"%d",Servermem->senaste[nummer].tid_inloggad);
			break;
		case 'u' : case 'U' :
			sprintf(str,"%d",Servermem->senaste[nummer].ul);
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void sysinfo(struct RexxMsg *mess) {
	int nummer=0, i, j, tmp = -1;
	long tmp1;
	struct Mote *letpek;
	char str[100], *args1;
	if(!mess->rm_Args[1]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	switch(mess->rm_Args[1][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%d",((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer);
			break;
		case 'h' : case 'H' :
			sprintf(str,"%d",Servermem->info.hightext);
			break;
		case 'k' : case 'K' :
			sprintf(str,"%d",Servermem->info.kommandon);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%d",Servermem->info.lowtext);
			break;
		case 'm' : case 'M' :
			letpek=(struct Mote *)Servermem->mot_list.mlh_Head;
			for(;letpek->mot_node.mln_Succ;letpek=(struct Mote *)letpek->mot_node.mln_Succ)
				if(letpek->nummer > nummer) nummer=letpek->nummer;
			sprintf(str,"%d",nummer);
			break;
		case 'n' : case 'N' :
			sprintf(str,"%d",Servermem->info.nycklar);
			break;
		case 'o' : case 'O' :
			sprintf(str,"%d",Servermem->info.areor);
			break;
		case 'T' : case 't' :
			while(Servermem->info.bps[++tmp] > 0);

			sprintf(str,"%d",tmp);
			break;

		case 'b' : case 'B' :
			args1 = &mess->rm_Args[1][0];
			args1++;
			if(args1 == NULL)
			{
				strcpy(str,"-2");
				return;
			}

			while(Servermem->info.bps[++tmp] > 0 && tmp < 50);

			for(i=0;i<tmp;i++)
			{
				for(j=0;j<i;j++)
				{
					if(i != j)
					{
						if(Servermem->info.bps[i] < Servermem->info.bps[j])
						{
							swapbps(&Servermem->info.bps[i], &Servermem->info.bps[j]);
							swapbps(&Servermem->info.antbps[i], &Servermem->info.antbps[j]);
						}
					}
				}
			}

			tmp1 = atoi((char *)args1);
			if(tmp1 < 1 || tmp1 > tmp)
			{
				strcpy(str,"-1");
				break;
			}

			sprintf(str,"%d %d",Servermem->info.bps[tmp1-1], Servermem->info.antbps[tmp1-1]);
			break;
		case 'd' : case 'D' :
			sprintf(str, "%d", (int) MAXNOD);
			break;

		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void swapbps(long *bps, long *bps2)
{
	long tmp;

	tmp = *bps;
	*bps = *bps2;
	*bps2 = tmp;
}

void rexxreadcfg(struct RexxMsg *mess) {
  ReadSystemConfig();
  ReadCommandConfig();
  ReadFileKeyConfig();
  ReadStatusConfig();
  ReadNodeTypesConfig();
  ReadFidoConfig();
  SetRexxResultString(mess, "");
}

int parsenamn(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,found=-1,nummer;
	char *faci,*skri2;
	struct ShortUser *letpek;
	if(skri[0]==0 || skri[0]==' ') return(-3);
	if(IzDigit(skri[0])) {
		nummer=atoi(skri);
		if(userexists(nummer)) return(nummer);
		else return(-1);
	}
	if(matchar(skri,"Sysop")) return(0);
	letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;
	while(letpek->user_node.mln_Succ && going)
	{
		faci=letpek->namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci))
		{
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0)
				{
					found=letpek->nummer;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci))
				{
					faci=FindNextWord(faci);
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

int parse(char *skri) {
	int nummer=0,argtyp;
	char *arg2,*ord2;
	struct Kommando *kompek,*forst=NULL;
	if(skri[0]==0) return(-3);
	if(IzDigit(skri[0])) {
		/* argument=skri; */
		return(212);
	}
	arg2=FindNextWord(skri);
	if(IzDigit(arg2[0])) argtyp=KOMARGNUM;
	else if(!arg2[0]) argtyp=KOMARGINGET;
	else argtyp=KOMARGCHAR;
	for(kompek=(struct Kommando *)Servermem->kom_list.mlh_Head;kompek->kom_node.mln_Succ;kompek=(struct Kommando *)kompek->kom_node.mln_Succ) {
		if(matchar(skri,kompek->namn)) {
			ord2=FindNextWord(kompek->namn);
			if((kompek->antal_ord==2 && matchar(arg2,ord2) && arg2[0]) || kompek->antal_ord==1) {
				if(kompek->antal_ord==1) {
					if(kompek->argument==KOMARGNUM && argtyp==KOMARGCHAR) continue;
					if(kompek->argument==KOMARGINGET && argtyp!=KOMARGINGET) continue;
				}
				if(forst==NULL) {
					forst=kompek;
					nummer=kompek->nummer;
				}
				else if(forst==(struct Kommando *)1L) {
				} else {
					forst=(struct Kommando *)1L;
				}
			}
		}
	}
	if(forst==NULL) return(-1);
	else if(forst==(struct Kommando *)1L) return(-2);
	return(nummer);
}

int parsemot(char *skri) {
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	int going2=TRUE,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') return(-3);
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
		faci=motpek->namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0) return((int)motpek->nummer);
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
	}
	return(found);
}

int parsearea(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==Servermem->info.areor) going=FALSE;
		faci=Servermem->areor[count].namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

int parsenyckel(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==Servermem->info.nycklar) going=FALSE;
		faci=Servermem->Nyckelnamn[count];
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

struct Fil *parsefil(skri,area)
char *skri;
int area;
{
	int quoted = FALSE;
	char tmpstr[42], *pt;
	struct Fil *letpek;
	if(skri[0]==0 || skri[0]==' ') return(NULL);
	if(skri[0]=='"') {
		quoted = TRUE;
		strcpy(tmpstr,&skri[1]);
		if(pt = strchr(tmpstr,'"')) *pt = 0;
	}
	else strcpy(tmpstr,skri);
	letpek=(struct Fil *)Servermem->areor[area].ar_list.mlh_TailPred;
	while(letpek->f_node.mln_Pred) {
		if(quoted) {
			if(!stricmp(tmpstr,letpek->namn)) return(letpek);
		} else {
			if(matchar(tmpstr,letpek->namn)) return(letpek);
		}
		letpek=(struct Fil *)letpek->f_node.mln_Pred;
	}
	return(NULL);
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

int userexists(int nummer) {
	struct ShortUser *letpek;
	int found=FALSE;
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			found=TRUE;
			break;
		}
	}
	return(found);
}

int getuserstatus(int nummer) {
	struct ShortUser *letpek;
	int status=-1;
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			status=letpek->status;
			break;
		}
	}
	return(status);
}

void kominfo(struct RexxMsg *mess) {
	int nummer,found=FALSE;
	char str[100];
	struct Kommando *kompek;
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	nummer=atoi(mess->rm_Args[1]);
	for(kompek=(struct Kommando *)Servermem->kom_list.mlh_Head;kompek->kom_node.mln_Succ;kompek=(struct Kommando *)kompek->kom_node.mln_Succ)
		if(kompek->nummer==nummer) {
			found=TRUE;
			break;
		}
	if(!found) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%d",kompek->argument);
			break;
		case 'd' : case 'D' :
			sprintf(str,"%d",kompek->mindays);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%d",kompek->minlogg);
			break;
		case 'n' : case 'N' :
			strcpy(str,kompek->namn);
			if(str[strlen(str)-1]=='\n') str[strlen(str)-1]=0;
			break;
		case 'o' : case 'O' :
			sprintf(str,"%d",kompek->antal_ord);
			break;
		case 's' : case 'S' :
			sprintf(str,"%d",kompek->status);
			break;
		case 'x' : case 'X' :
			strcpy(str,kompek->losen);
			break;
		defualt :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void filinfo(struct RexxMsg *mess) {
	int area;
	struct Fil *filpek;
	struct tm *ts;
	char str[256];
	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	area=atoi(mess->rm_Args[3]);
	if(area<0 || area>=Servermem->info.areor || !Servermem->areor[area].namn) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!(filpek=parsefil(mess->rm_Args[1],area))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'b' : case 'B' :
			strcpy(str,filpek->beskr);
			break;
		case 'd' : case 'D' :
			sprintf(str,"%d",filpek->downloads);
			break;
		case 'e' : case 'E' :
			ts=localtime(&filpek->senast_dl);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'f' : case 'F' :
			sprintf(str,"%d",filpek->flaggor);
			break;
		case 'i' : case 'I' :
			sprintf(str,"%d",filpek->dir);
			break;
		case 'n' : case 'N' :
			strcpy(str,filpek->namn);
			break;
		case 'r' : case 'R' :
			sprintf(str,"%d",filpek->status);
			break;
		case 's' : case 'S' :
			sprintf(str,"%d",filpek->size);
			break;
		case 't' : case 'T' :
			ts=localtime(&filpek->tid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'u' : case 'U' :
			sprintf(str,"%d",filpek->uppladdare);
			break;
		case 'l' : case 'L' :
			if(filpek->flaggor & FILE_LONGDESC)
				sprintf(str,"%slongdesc/%s.long",Servermem->areor[area].dir[filpek->dir],filpek->namn);
			else
			{
				if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
					printf("Kunde inte allokera en ArgString\n");
				mess->rm_Result1=0;
				return;
			}
			break;
		default:
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void areainfo(struct RexxMsg *mess) {
	int nummer,dir;
	char str[100];
	struct tm *ts;
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	if((nummer=atoi(mess->rm_Args[1]))<0 || nummer>=Servermem->info.areor) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!Servermem->areor[nummer].namn[0]) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%d",Servermem->areor[nummer].skapad_av);
			break;
		case 'd' : case  'D' :
			dir=atoi(&mess->rm_Args[2][1]);
			if(dir<0 || dir>15) {
				str[0]=0;
				break;
			}
			strcpy(str,Servermem->areor[nummer].dir[dir]);
			break;
		case 'f' : case 'F' :
			sprintf(str,"%d",Servermem->areor[nummer].flaggor);
			break;
		case 'g' : case 'G' :
			sprintf(str,"%d,%d",(unsigned int)Servermem->areor[nummer].grupper/65536,(unsigned int)Servermem->areor[nummer].grupper%65536);
			break;
		case 'm' : case 'M' :
			sprintf(str,"%d",Servermem->areor[nummer].mote);
			break;
		case 'n' : case 'N' :
			strcpy(str,Servermem->areor[nummer].namn);
			break;
		case 't' : case 'T' :
			ts=localtime(&Servermem->areor[nummer].skapad_tid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void chguser(struct RexxMsg *mess) {
	int user,x,foo;
	struct ShortUser *letpek;
	struct User chguseruser;
	char temp[20];

	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	if(!userexists(user=atoi(mess->rm_Args[1]))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==user) break;

	if(x==MAXNOD)
	{
		if(readuser(user,&chguseruser))
		{
			mess->rm_Result1=3;
			mess->rm_Result2=NULL;
			return;
		}
	}

	switch(mess->rm_Args[3][0]) {
		case 'a' : case 'A' :
			if(x==MAXNOD) strncpy(chguseruser.annan_info,mess->rm_Args[2],60);
			else strncpy(Servermem->inne[x].annan_info,mess->rm_Args[2],60);
			break;
		case 'c' : case 'C' :
			if(x==MAXNOD) strncpy(chguseruser.land,mess->rm_Args[2],40);
			else strncpy(Servermem->inne[x].land,mess->rm_Args[2],40);
			break;
		case 'd' : case 'D' :
			if(x==MAXNOD) chguseruser.download=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].download=atoi(mess->rm_Args[2]);
			break;
		case 'f' : case 'F' :
			if(x==MAXNOD) strncpy(chguseruser.telefon,mess->rm_Args[2],20);
			else strncpy(Servermem->inne[x].telefon,mess->rm_Args[2],20);
			break;
		case 'g' : case 'G' :
			if(x==MAXNOD) strncpy(chguseruser.gata,mess->rm_Args[2],40);
			else strncpy(Servermem->inne[x].gata,mess->rm_Args[2],40);
			break;
		case 'h' : case 'H' :
			if(x==MAXNOD) chguseruser.downloadbytes = atoi(mess->rm_Args[2]);
			else Servermem->inne[x].downloadbytes = atoi(mess->rm_Args[2]);
			break;
		case 'j' : case 'J' :
			if(x==MAXNOD) chguseruser.uploadbytes = atoi(mess->rm_Args[2]);
			else Servermem->inne[x].uploadbytes = atoi(mess->rm_Args[2]);
			break;
		case 'i' : case 'I' :
			if(x==MAXNOD) chguseruser.loggin=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].loggin=atoi(mess->rm_Args[2]);
			break;
		case 'l' : case 'L' :
			if(x==MAXNOD) chguseruser.read=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].read=atoi(mess->rm_Args[2]);
			break;
		case 'm' : case 'M' :
			if(x==MAXNOD) strncpy(chguseruser.prompt,mess->rm_Args[2],5);
			else strncpy(Servermem->inne[x].prompt,mess->rm_Args[2],5);
			break;
		case 'n' : case 'N' :
			if((foo=parsenamn(mess->rm_Args[2]))==-1 || foo==user) {
				if(x==MAXNOD) strncpy(chguseruser.namn,mess->rm_Args[2],40);
				else strncpy(Servermem->inne[x].namn,mess->rm_Args[2],40);
				for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
					if(letpek->nummer==user) break;
				if(letpek->user_node.mln_Succ) strncpy(letpek->namn,mess->rm_Args[2],40);
			}
			break;
		case 'o' : case 'O' :
			if(x==MAXNOD) chguseruser.flaggor=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].flaggor=atoi(mess->rm_Args[2]);
			break;
		case 'p' : case 'P' :
			if(x==MAXNOD) strncpy(chguseruser.postadress,mess->rm_Args[2],40);
			else strncpy(Servermem->inne[x].postadress,mess->rm_Args[2],40);
			break;
		case 'q' : case 'Q' :
			if(x==MAXNOD) chguseruser.rader=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].rader=atoi(mess->rm_Args[2]);
			break;
		case 'r' : case 'R' :
			if(x==MAXNOD) chguseruser.status=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].status=atoi(mess->rm_Args[2]);
			for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ)
				if(letpek->nummer==user) break;
			if(letpek->user_node.mln_Succ) letpek->status=atoi(mess->rm_Args[2]);
			break;
		case 's' : case 'S' :
			if(x==MAXNOD) chguseruser.skrivit=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].skrivit=atoi(mess->rm_Args[2]);
			break;
		case 't' : case 'T' :
			if(x==MAXNOD) chguseruser.tot_tid=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].tot_tid=atoi(mess->rm_Args[2]);
			break;
		case 'u' : case 'U' :
			if(x==MAXNOD) chguseruser.upload=atoi(mess->rm_Args[2]);
			else Servermem->inne[x].upload=atoi(mess->rm_Args[2]);
			break;
		case 'x' : case 'X' :
                  if(Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS) {
                    CryptPassword(mess->rm_Args[2], temp);
                  } else {
                    strncpy(temp, mess->rm_Args[2], 15);
                  }
                  if(x==MAXNOD) strncpy(chguseruser.losen,temp,15);
                  else strncpy(Servermem->inne[x].losen,temp,15);
                  break;
		default : break;
	}
	if(x==MAXNOD && writeuser(user,&chguseruser)) mess->rm_Result1=3;
	else mess->rm_Result1=0;
	if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
		printf("Kunde inte allokera en ArgString\n");
}

void skapafil(struct RexxMsg *mess) {
	struct Fil *fil;
	char filnamn[100];
	long tid;
	struct FileInfoBlock *fib;
	BPTR lock;
	int cnt=6,nyckel,area,x,valres;

	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3] || !mess->rm_Args[4] || !mess->rm_Args[5]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	if((area=atoi(mess->rm_Args[2]))<0 || area>=Servermem->info.areor || !Servermem->areor[area].namn[0]) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	valres = valnamn(mess->rm_Args[1],area);
	if(valres) {
		sprintf(filnamn,"%d",valres);
		if(!(mess->rm_Result2=(long)CreateArgstring(filnamn,strlen(filnamn))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	for(x=0;x<16;x++) {
		sprintf(filnamn,"%s%s",Servermem->areor[area].dir[x],mess->rm_Args[1]);
		if(!access(filnamn,0)) break;
	}
	if(x==16) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	sprintf(filnamn,"%s%s",Servermem->areor[area].dir[x],mess->rm_Args[1]);

	if(!userexists(atoi(mess->rm_Args[3]))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-4"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!(fil=(struct Fil *)AllocMem(sizeof(struct Fil),MEMF_CLEAR | MEMF_PUBLIC))) {
		printf("Kunde inte allokera minne för filen!\n");
		mess->rm_Result1=2;
		mess->rm_Result2=NULL;
		return;
	}
	fil->tid=fil->validtime=time(&tid);
	if(!(lock=Lock(filnamn,ACCESS_READ))) {
		printf("Kunde inte få ett lock för filen!\n",-1);
		mess->rm_Result1=3;
		mess->rm_Result2=NULL;
		FreeMem(fil,sizeof(struct Fil));
		return;
	}
	if(!(fib=(struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock),MEMF_CLEAR))) {
		printf("Kunde inte allokera minne för en FileInfoBlock-struktur!\n");
		mess->rm_Result1=4;
		mess->rm_Result2=NULL;
		FreeMem(fil,sizeof(struct Fil));
		UnLock(lock);
		return;
	}
	if(!Examine(lock,fib)) {
		printf("Kunde inte göra Examine() på filen!\n");
		mess->rm_Result1=5;
		mess->rm_Result2=NULL;
		FreeMem(fil,sizeof(struct Fil));
		UnLock(lock);
		FreeMem(fib,sizeof(struct FileInfoBlock));
		return;
	}
	fil->size=fib->fib_Size;
	UnLock(lock);
	FreeMem(fib,sizeof(struct FileInfoBlock));
	fil->uppladdare=atoi(mess->rm_Args[3]);
	strncpy(fil->namn,mess->rm_Args[1],40);
	fil->status=atoi(mess->rm_Args[5]);
	strncpy(fil->beskr,mess->rm_Args[4],70);
	fil->dir=x;
	if(Servermem->cfg.cfgflags & NICFG_VALIDATEFILES) fil->flaggor = FILE_NOTVALID;
	for(cnt=6;cnt<16 && mess->rm_Args[cnt];cnt++)
		if((nyckel=parsenyckel(mess->rm_Args[cnt]))!=-1) BAMSET(fil->nycklar,nyckel);
	AddTail((struct List *)&Servermem->areor[area].ar_list,(struct Node *)fil);
	if(writefiles(area)) {
		printf("Kunde inte öppna datafilen\n");
		mess->rm_Result1=6;
		mess->rm_Result2=NULL;
		return;
	}
	mess->rm_Result1=0;
	if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
		printf("Kunde inte allokera en ArgString\n");
}

int valnamn(char *namn,int area) {
	if(strlen(namn)>25) return(-6);
	if(strpbrk(namn," #?*/:()[]\"")) return(-5);
	if(parsefil(namn,area)) return(-2);
	return(0);
}
