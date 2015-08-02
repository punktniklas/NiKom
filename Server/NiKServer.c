#include <intuition/intuition.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dos.h>
#include <time.h>
#include <rexx/storage.h>
#include "NiKomstr.h"
#include "ServerFuncs.h"
#include "NiKomLib.h"

#define ERROR	10
#define OK	0
#define VERSION	33

void cleanup(int,char *);
void getkmd(void);
void getconfig(void);
void getmoten(void);
void gettextmot(void);
void getnamn(void);
void getinfo(void);
void initflaggor(void);
void sparatext(struct NiKMess *);
void radera(int);

int CXBRK(void) { return(0); }

char pubscreen[40],reggadnamn[60];
int xpos,ypos;
struct MsgPort *NiKPort,*permitport,*rexxport, *nodereplyport;
struct IntuitionBase *IntuitionBase=NULL;
struct RsxLib *RexxSysBase;
struct Library *UtilityBase;
struct Library *NiKomBase;
struct Window *NiKWindow;
struct System *Servermem;

void getkmd(void) {
	BPTR fh;
	struct Kommando *allok;
	char buffer[100],*pekare;
	int x=0, going=TRUE, grupp, len;
	NewList((struct List *)&Servermem->kom_list);
	if(!(fh=Open("NiKom:DatoCfg/Kommandon.cfg",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Kommandon.cfg\n");
	buffer[0]=0;
	while(buffer[0]!='N') {
		if(!(pekare=FGets(fh,buffer,99))) {
			if(!IoErr()) {
				Close(fh);
				printf("Inga kommandon hittade i Kommandon.cfg\n");
				return;
			} else {
				printf("Error while reading Kommandon.cfg\n");
				Close(fh);
				return;
			}
		}
	}
	len = strlen(buffer);
	if(buffer[len - 1] == '\n') buffer[len - 1] = 0;
	if(!(allok=(struct Kommando *)AllocMem(sizeof(struct Kommando),MEMF_CLEAR | MEMF_PUBLIC)))
		cleanup(ERROR,"Kunde inte allokera minne till en komandostruktur\n");
	AddTail((struct List *)&Servermem->kom_list,(struct Node *)allok);
	strncpy(allok->namn,&buffer[2],30);
	x++;
	while(going) {
		if(!(pekare=FGets(fh,buffer,99))) {
			if(!IoErr()) break;
			else {
				printf("Error while reading Kommandon.cfg\n");
				Close(fh);
				return;
			}
		}
		len = strlen(buffer);
		if(buffer[len - 1] == '\n') buffer[len - 1] = 0;
		switch(buffer[0]) {
			case '#' :
				allok->nummer=atoi(&buffer[2]);
				break;
			case 'O' :
				allok->antal_ord=atoi(&buffer[2]);
				break;
			case 'A' :
				if(buffer[2]=='#') allok->argument=KOMARGNUM;
				else allok->argument=KOMARGCHAR;
				break;
			case 'S' :
				allok->status=atoi(&buffer[2]);
				break;
			case 'L' :
				allok->minlogg=atoi(&buffer[2]);
				break;
			case 'D' :
				allok->mindays=atoi(&buffer[2]);
				break;
			case 'X' :
				strncpy(allok->losen,&buffer[2],19);
				break;
			case 'V' :
				if(buffer[2]=='N' || buffer[2]=='n') allok->secret=TRUE;
				break;
			case 'B' :
				strncpy(allok->beskr,&buffer[2],68);
				break;
			case 'W' :
				strncpy(allok->vilkainfo,&buffer[2],49);
				break;
			case 'F' :
				strncpy(allok->logstr,&buffer[2],49);
				break;
			case '(' :
				allok->before=atoi(&buffer[2]);
				break;
			case ')' :
				allok->after=atoi(&buffer[2]);
				break;
			case 'M' :
				strncpy(allok->menu,&buffer[2],49);
				break;
			case 'G' :
				grupp = parsegrupp(&buffer[2]);
				if(grupp == -1) printf("I Kommandon.cfg: Okänd grupp %s\n",&buffer[2]);
				else BAMSET((char *)&allok->grupper,grupp);
				break;
			case 'N' :
				if(!(allok=(struct Kommando *)AllocMem(sizeof(struct Kommando),MEMF_CLEAR | MEMF_PUBLIC)))
					cleanup(ERROR,"Kunde inte allokera minne till en komandostruktur\n");
				AddTail((struct List *)&Servermem->kom_list,(struct Node *)allok);
				strncpy(allok->namn,&buffer[2],30);
				x++;
				break;
		}
	}
	printf("Kommandon.cfg inläst (%d kommandon)\n",x);
	Close(fh);
	Servermem->info.kommandon=x;
}

void getnycklar(void) {
	BPTR fh;
	char buffer[100],*pekare;
	int x=0;
	if(!(fh=Open("NiKom:DatoCfg/Nycklar.cfg",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna Nycklar.cfg\n");
	while(pekare=FGets(fh,buffer,99)) {
		if(x >= MAXNYCKLAR) break;
		if(buffer[0]!=10 && buffer[0]!='*') {
			strncpy(Servermem->Nyckelnamn[x],buffer,40);
			x++;
		}
	}
	if(IoErr()) printf("Fel vid läsandet av Nycklar.cfg\n");
	else printf("Nycklar.cfg inläst (%d)\n",x);
	Close(fh);
	Servermem->info.nycklar=x;
}

void getconfig(void) {
	BPTR fh;
	char buffer[100];
	int status,len;
	struct SpecialLogin *tempnode;
	NewList((struct List *)&Servermem->special_login);
	if(!(fh=Open("NiKom:DatoCfg/System.cfg",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/System.cfg");
	if(getcfgfilestring("DEFAULTFLAGS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.defaultflags=atoi(buffer);
	if(getcfgfilestring("DEFAULTSTATUS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.defaultstatus=atoi(buffer);
	if(getcfgfilestring("DEFAULTRADER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.defaultrader=atoi(buffer);
	for(;;) {
		if(!FGets(fh,buffer,99)) {
			printf("Korrupt fil, hittar inte ENDSTATUS\n");
			Close(fh);
			return;
		}
		if(!strncmp(buffer,"ENDSTATUS",9)) break;
		if(!strncmp(buffer,"STATUS",6)) {
			status=atoi(&buffer[6]);
			if(status<0 || status>100) {
				printf("Man kan inte ha %d som status!\n",status);
				Close(fh);
				return;
			}
			if(getcfgfilestring("MAXTID",fh,buffer)) {
				Close(fh);
				return;
			} else Servermem->cfg.maxtid[status]=atoi(buffer);
			if(getcfgfilestring("ULDL",fh,buffer)) {
				Close(fh);
				return;
			} else Servermem->cfg.uldlratio[status]=atoi(buffer);
			if(getcfgfilestring("INAKTIV",fh,buffer)) {
				Close(fh);
				return;
			} else Servermem->cfg.inaktiv[status]=atoi(buffer);
		}
	}
	if(getcfgfilestring("CLOSEDBBS",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(!(strncmp(buffer,"JA",2))) Servermem->cfg.cfgflags |= NICFG_CLOSEDBBS;
		else Servermem->cfg.cfgflags &= ~NICFG_CLOSEDBBS;
	}
	if(getcfgfilestring("BREVLÅDA",fh,buffer)) {
		Close(fh);
		return;
	} else {
		strncpy(Servermem->cfg.brevnamn,buffer,40);
	}
	if(getcfgfilestring("NY",fh,buffer)) {
		Close(fh);
		return;
	} else {
		strncpy(Servermem->cfg.ny,buffer,20);
	}
	if(getcfgfilestring("DISKFREE",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.diskfree=atoi(buffer);
	if(getcfgfilestring("ULTMP",fh,buffer)) {
		Close(fh);
		return;
	} else {
		len=strlen(buffer);
		if(buffer[len-1]!='/' && buffer[len]!=':') {
			buffer[len]='/';
			buffer[len+1]=0;
		}
		strncpy(Servermem->cfg.ultmp,buffer,99);
	}
	if(getcfgfilestring("PREINLOGG",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.preinlogg=atoi(buffer);
	if(getcfgfilestring("POSTINLOGG",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.postinlogg=atoi(buffer);
	if(getcfgfilestring("UTLOGG",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.utlogg=atoi(buffer);
	if(getcfgfilestring("NYANV",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nyanv=atoi(buffer);
	if(getcfgfilestring("PREUPLOAD1",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.preup1=atoi(buffer);
	if(getcfgfilestring("PREUPLOAD2",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.preup2=atoi(buffer);
	if(getcfgfilestring("POSTUPLOAD1",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.postup1=atoi(buffer);
	if(getcfgfilestring("POSTUPLOAD2",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.postup2=atoi(buffer);
	if(getcfgfilestring("NORIGHT",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.noright=atoi(buffer);
	if(getcfgfilestring("NEXTMEET",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nextmeet=atoi(buffer);
	if(getcfgfilestring("NEXTTEXT",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nexttext=atoi(buffer);
	if(getcfgfilestring("NEXTKOM",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nextkom=atoi(buffer);
	if(getcfgfilestring("SETID",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.setid=atoi(buffer);
	if(getcfgfilestring("NEXTLETTER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nextletter=atoi(buffer);
	if(getcfgfilestring("CARDROPPED",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.cardropped=atoi(buffer);
	for(;;) {
		if(getcfgfilestring("SPECIALLOGIN",fh,buffer)) {
			Close(fh);
			return;
		}
		if(!strnicmp(buffer,"END",3)) break;
		if(!(tempnode=(struct SpecialLogin *)AllocMem(sizeof(struct SpecialLogin),MEMF_PUBLIC|MEMF_CLEAR)))
			cleanup(ERROR,"Kunde inte allokera en SpecialLogin-struktur\n");
		tempnode->bokstav=buffer[0];
		tempnode->rexxprg=atoi(&buffer[1]);
		AddTail((struct List *)&Servermem->special_login,(struct Node *)tempnode);
	}
	if(getcfgfilestring("LOGFILE",fh,buffer)) {
		Close(fh);
		return;
	} else strcpy(Servermem->cfg.logfile,buffer);
	if(getcfgfilestring("LOGMASK",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.logmask=atoi(buffer);
	if(getcfgfilestring("SCREEN",fh,buffer)) {
		Close(fh);
		return;
	} else strcpy(pubscreen,buffer);
	if(getcfgfilestring("YPOS",fh,buffer)) {
		Close(fh);
		return;
	} else ypos=atoi(buffer);
	if(getcfgfilestring("XPOS",fh,buffer)) {
		Close(fh);
		return;
	} else xpos=atoi(buffer);
	if(getcfgfilestring("VALIDERAFILER",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_VALIDATEFILES;
		else Servermem->cfg.cfgflags &= ~NICFG_VALIDATEFILES;
	}
/*	if(getcfgfilestring("STARATNOECHO",fh,buffer)) {
*		Close(fh);
*		return;
*	} else {
*		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_STARATNOECHO;
*		else Servermem->cfg.cfgflags &= ~NICFG_STARATNOECHO;
*	} */
	if(getcfgfilestring("LOGINTRIES",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.logintries=atoi(buffer);
	if(getcfgfilestring("LOCALCOLOURS",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_LOCALCOLOURS;
		else Servermem->cfg.cfgflags &= ~NICFG_LOCALCOLOURS;
	}

	if(getcfgfilestring("CRYPTEDPASSWORDS",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_CRYPTEDPASSWORDS;
		else Servermem->cfg.cfgflags &= ~NICFG_CRYPTEDPASSWORDS;
	}
	if(getcfgfilestring("MAXPRGDATACACHE",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.maxprgdatacache=atoi(buffer);
	Close(fh);
	printf("System.cfg inläst\n");
}

void getmoten(void) {
	BPTR fh;
	int x=0,ret;
	struct Mote *allok,*inspek;
	NewList((struct List *)&Servermem->mot_list);
	if(!(fh=Open("NiKom:DatoCfg/Möten.dat",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Möten.dat");
	if(!(allok=(struct Mote *)AllocMem(sizeof(struct Mote),MEMF_CLEAR | MEMF_PUBLIC)))
		cleanup(ERROR,"Kunde inte allokera en struct Mote\n");
	for(;;) {
		ret=Read(fh,(void *)allok,sizeof(struct Mote));
		if(ret==-1) {
			FreeMem(allok,sizeof(struct Mote));
			Close(fh);
			cleanup(ERROR,"Fel vid läsandet av Möten.dat\n");
		} else if(!ret) {
			FreeMem(allok,sizeof(struct Mote));
			break;
		}
		if(!allok->namn[0]) {
			x++;
			continue;
		}
		allok->nummer=x;
		for(inspek=(struct Mote *)Servermem->mot_list.mlh_Head;inspek->mot_node.mln_Succ;inspek=(struct Mote *)inspek->mot_node.mln_Succ)
			if(inspek->sortpri>allok->sortpri) break;
		Insert((struct List *)&Servermem->mot_list,(struct Node *)allok,(struct Node *)inspek->mot_node.mln_Pred);
		x++;
		if(!(allok=(struct Mote *)AllocMem(sizeof(struct Mote),MEMF_CLEAR | MEMF_PUBLIC)))
			cleanup(ERROR,"Kunde inte allokera en struct Mote\n");
	}
	Close(fh);
	printf("Mötena inlästa (%d st)\n",x);
}

void getareor(void) {
	BPTR fh;
	int x;
	if(!(fh=Open("NiKom:DatoCfg/Areor.dat",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Areor.dat");
	SetIoErr(0L);
	Servermem->info.areor=FRead(fh,(void *)Servermem->areor,sizeof(struct Area),MAXAREA);
	if(IoErr()) printf("Fel vid läsandet av Areor.dat\n");
	else printf("Areor.dat inläst (%d areor)\n",Servermem->info.areor);
	Close(fh);
	for(x=0;x<MAXAREA;x++) NewList((struct List *)&Servermem->areor[x].ar_list);
}

void getfiler(void) {	/* Ändrad för nikfiles.data 960707 JÖ */
	int x;
	BPTR datafil;
	struct Fil *allokpek;
	struct MinNode *sokpek;
	char nikfilename[110];
	int readlen;
	long index;

	for(x=0;x<Servermem->info.areor;x++) {
		if(!Servermem->areor[x].namn[0]) continue;
		printf("Arean %s...\n",Servermem->areor[x].namn);
		sprintf(nikfilename,"nikom:datocfg/areor/%d.dat",x);
		if(!(datafil=Open(nikfilename,MODE_OLDFILE))) {
			printf("Kunde inte öppna %s\n",nikfilename);
			continue;
		}
		index=0;
		for(;;) {
			if(!(allokpek=(struct Fil *)AllocMem(sizeof(struct Fil),MEMF_CLEAR | MEMF_PUBLIC))) {
				printf("Kunde inte allokera minne för filen!\n");
				Close(datafil);
				return;
			}
			if((readlen=Read(datafil,allokpek,sizeof(struct DiskFil)))!=sizeof(struct DiskFil)) {
				if(readlen!=0) printf("Fel vid läsning av %s\n",nikfilename);
				Close(datafil);
				break;
			}
			allokpek->index = index++;
			/* kolla så att inte filen är raderad */
			if(allokpek->namn[0] == 0)
				continue;
			// allokpek->dir=y;
			for(sokpek=Servermem->areor[x].ar_list.mlh_Head;sokpek;sokpek=sokpek->mln_Succ) {
				if(((struct Fil *)sokpek)->tid>allokpek->tid) break;
			}
			if(sokpek) Insert((struct List *)&Servermem->areor[x].ar_list,(struct Node *)allokpek,(struct Node *)sokpek->mln_Pred);
			else AddTail((struct List *)&Servermem->areor[x].ar_list,(struct Node *)allokpek);
		}
	}
	printf("Filerna inlästa\n");
}

void gettextmot(void) {
	BPTR fh;
	int words;
	if(!(fh=Open("NiKom:DatoCfg/Textmot.dat",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Textmot.dat");
	words=FRead(fh,(void *)Servermem->texts,2,MAXTEXTS);
	if(words!=MAXTEXTS) printf("Fel vid läsandet av Textmot.dat\n");
	else printf("Textmot.dat inläst (%d words)\n",words);
	Close(fh);
}

int adduser(int nummer) {
	BPTR fh;
	struct User tempuser;
	struct ShortUser *inspek,*allok;
	char filnamn[100];
/*	if(!reggadnamn[0] && nummer>4) return(1); */
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",nummer/100,nummer);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		printf("Kunde inte öppna %s\n",filnamn);
		return(1);
	}
	if(Read(fh,&tempuser,sizeof(struct User))==-1) {
		printf("Fel vid läsandet av %s\n",filnamn);
		Close(fh);
		return(1);
	}
	Close(fh);
	if(!(allok=(struct ShortUser *)AllocMem(sizeof(struct ShortUser),MEMF_CLEAR | MEMF_PUBLIC)))
		cleanup(ERROR,"Kunde inte allokera en ShortUser-struktur\n");
	for(inspek=(struct ShortUser *)Servermem->user_list.mlh_Head;inspek->user_node.mln_Succ;inspek=(struct ShortUser *)inspek->user_node.mln_Succ)
		if(inspek->nummer>nummer) break;
	Insert((struct List *)&Servermem->user_list,(struct Node *)allok,(struct Node *)inspek->user_node.mln_Pred);
	allok->nummer=nummer;
	allok->status=tempuser.status;
	allok->grupper=tempuser.grupper;
	strcpy(allok->namn,tempuser.namn);
	return(0);
}

void scanuserdir(char *dirnamn) {
	BPTR lock;
	int goon;
	char __aligned buffer[2048];
	struct ExAllControl *ec;
	struct ExAllData *ed;
	char path[100];
	printf("   Läser in användarkatalog %s...\n",dirnamn);
	if(!(ec=(struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL,NULL)))
		cleanup(ERROR,"Kunde inte allokera en ExAllControl-struktur\n");
	sprintf(path,"NiKom:Users/%s",dirnamn);
	if(!(lock=Lock(path,ACCESS_READ))) {
		FreeDosObject(DOS_EXALLCONTROL,ec);
		cleanup(ERROR,"Kunde inte får ett Lock för NiKom:Users/x\n");
	}
	ec->eac_LastKey=0L;
	ec->eac_MatchString=NULL;
	ec->eac_MatchFunc=NULL;
	goon=ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
	ed=(struct ExAllData *)buffer;
	for(;;) {
		if(!ec->eac_Entries) {
			if(!goon) break;
			goon=ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
			ed=(struct ExAllData *)buffer;
			continue;
		}
		adduser(atoi(ed->ed_Name));
		ed=ed->ed_Next;
		if(!ed) {
			if(!goon) break;
			goon=ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
			ed=(struct ExAllData *)buffer;
		}
	}
	UnLock(lock);
	FreeDosObject(DOS_EXALLCONTROL,ec);
}

void getnamn(void) {
	BPTR lock;
	int goon;
	char __aligned buffer[2048];
	struct ExAllControl *ec;
	struct ExAllData *ed;
	NewList((struct List *)&Servermem->user_list);
	if(!(ec=(struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL,NULL)))
		cleanup(ERROR,"Kunde inte allokera en ExAllControl-struktur\n");
	if(!(lock=Lock("NiKom:Users",ACCESS_READ))) {
		FreeDosObject(DOS_EXALLCONTROL,ec);
		cleanup(ERROR,"Kunde inte får ett Lock för NiKom:Users\n");
	}
	ec->eac_LastKey=0L;
	ec->eac_MatchString=NULL;
	ec->eac_MatchFunc=NULL;
	goon=ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
	ed=(struct ExAllData *)buffer;
	for(;;) {
		if(!ec->eac_Entries) {
			if(!goon) break;
			goon=ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
			ed=(struct ExAllData *)buffer;
			continue;
		}
		scanuserdir(ed->ed_Name);
		ed=ed->ed_Next;
		if(!ed) {
			if(!goon) break;
			goon=ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
			ed=(struct ExAllData *)buffer;
		}
	}
	UnLock(lock);
	FreeDosObject(DOS_EXALLCONTROL,ec);
	printf("Användarnamnen inlästa (Högsta nummer: %d)\n",((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer);
	printf("Sysops namn : %s\n",((struct ShortUser *)Servermem->user_list.mlh_Head)->namn);
}

void getgrupper(void)
{
	BPTR fh;
	int x=0;
	struct UserGroup *allok;
	NewList((struct List *)&Servermem->grupp_list);
	if(!(fh=Open("NiKom:DatoCfg/Grupper.dat",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Grupper.dat.");
	if(!(allok=(struct UserGroup *)AllocMem(sizeof(struct UserGroup),MEMF_CLEAR | MEMF_PUBLIC)))
		cleanup(ERROR,"Kunde inte allokera en UserGroup-struktur\n");

	for(;;)
	{
		SetIoErr(0L);
		if(!FRead(fh,(void *)allok,sizeof(struct UserGroup),1))
		{
			if(IoErr())
			{
				FreeMem(allok,sizeof(struct UserGroup));
				Close(fh);
				cleanup(ERROR,"Fel vid läsandet av Grupper.dat\n");
			}
			else
			{
				FreeMem(allok,sizeof(struct UserGroup));
				break;
			}
		}
		if(!allok->namn[0]) {
			x++;
			continue;
		}
		AddTail((struct List *)&Servermem->grupp_list,(struct Node *)allok);
		allok->nummer=x;
		x++;
		if(!(allok=(struct UserGroup *)AllocMem(sizeof(struct UserGroup),MEMF_CLEAR | MEMF_PUBLIC)))
			cleanup(ERROR,"Kunde inte allokera en UserGroup-struktur\n");
	}
	Close(fh);
	printf("Användargrupper inlästa (%d st)\n",x);
}

void getinfo() {
	BPTR fh;
	struct FileInfoBlock *fib;
	BPTR lock;
	ULONG high=0,low=-1L,nr;
	char filnamn[40];
	struct Header readhead;
	if(!(fh=Open("NiKom:DatoCfg/Sysinfo.dat",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Sysinfo.dat\n");
	if(!Read(fh,(void *)&Servermem->info,sizeof(struct SysInfo)))
		printf("Fel vid läsandet av Sysinfo.dat\n");
	else printf("Sysinfo.dat inläst\n");
	Close(fh);

	if(!(fib=(struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock),MEMF_CLEAR))) {
		printf("Kunde inte allokera en FileInfoBlock-struktur\n");
		return;
	}
	if(!(lock=Lock("NiKom:Moten",ACCESS_READ))) {
		printf("Kunde inte få ett lock för NiKom:Moten\n");
		FreeMem(fib,sizeof(struct FileInfoBlock));
		return;
	}
	if(!Examine(lock,fib)) {
		printf("Kunde inte undersöka NiKom:Moten\n");
		FreeMem(fib,sizeof(struct FileInfoBlock));
		UnLock(lock);
		return;
	}
	while(ExNext(lock,fib)) {
		if(strncmp(fib->fib_FileName,"Head",4)) continue;
		nr=atoi(&fib->fib_FileName[4]);
		if(nr>high) high=nr;
		if(nr<low) low=nr;
	}
	UnLock(lock);
	FreeMem(fib,sizeof(struct FileInfoBlock));

	if(low==-1L) {
		printf("Hittar inga Headx.dat. Antar att det inte finns några texter\n");
		Servermem->info.hightext=-1L;
		Servermem->info.lowtext=0L;
	} else {
		sprintf(filnamn,"NiKom:Moten/Head%d.dat",low);
		if(!(fh=Open(filnamn,MODE_OLDFILE))) {
			printf("Kunde inte öppna %s\n",filnamn);
			return;
		}
		if(Seek(fh,0,OFFSET_BEGINNING)==-1) printf("Kunde inte förflytta i Head.dat (1)\n");
		else if(!Read(fh,(void *)&readhead,sizeof(struct Header)))
			printf("Kunde inte läsa Head.dat (1)\n");
		Servermem->info.lowtext=readhead.nummer;
		Close(fh);
		sprintf(filnamn,"NiKom:Moten/Head%d.dat",high);
		if(!(fh=Open(filnamn,MODE_OLDFILE))) {
			printf("Kunde inte öppna %s\n",filnamn);
			return;
		}
		if(Seek(fh,-sizeof(struct Header),OFFSET_END)==-1) printf("Kunde inte förflytta i Head.dat (2)\n");
		else if(!Read(fh,(void *)&readhead,sizeof(struct Header)))
			printf("Kunde inte läsa Head.dat (2)\n");
		Servermem->info.hightext=readhead.nummer;
		Close(fh);
		printf("Lowtext: %d  Hightext: %d\n",Servermem->info.lowtext,Servermem->info.hightext);
	}
}

void getsenaste(void) {
	BPTR fh;
	if(!(fh=Open("NiKom:DatoCfg/Senaste.dat",MODE_OLDFILE))) {
		printf("Kunde inte öppna Senaste.dat\n");
		return;
	}
	if(!Read(fh,(void *)Servermem->senaste,sizeof(struct Inloggning)*MAXSENASTE)) printf("Fel under skrivandet av Senaste.dat\n");
	else printf("Senaste.dat inläst\n");
	Close(fh);
}

int getcfgfilestring(char *str,BPTR fh,char *vart) {
	char buffer[100];
	for(;;) {
		if(!FGets(fh,buffer,99)) {
			printf("Korrupt fil, hittade inte %s\n",str);
			return(1);
		}
		if(!(strncmp(buffer,str,strlen(str)))) {
			strcpy(vart,&buffer[strlen(str)+1]);
			if(vart[strlen(vart)-1]=='\n') vart[strlen(vart)-1]=0;
			return(0);
		}
	}
}

void getstatus(void) {
	BPTR fh;
	char buffer[100];
	if(!(fh=Open("NiKom:DatoCfg/Status.cfg",MODE_OLDFILE))) {
		printf("Kunde inte öppna NiKom:DatoCfg/Status.cfg");
		return;
	}
	if(getcfgfilestring("SKRIV",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.skriv=atoi(buffer);
	if(getcfgfilestring("TEXTER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.texter=atoi(buffer);
	if(getcfgfilestring("BREV",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.brev=atoi(buffer);
	if(getcfgfilestring("MEDMÖTEN",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.medmoten=atoi(buffer);
	if(getcfgfilestring("RADMÖTEN",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.radmoten=atoi(buffer);
	if(getcfgfilestring("SESTATUS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.sestatus=atoi(buffer);
	if(getcfgfilestring("ANVÄNDARE",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.anv=atoi(buffer);
	if(getcfgfilestring("ÄNDSTATUS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.chgstatus=atoi(buffer);
	if(getcfgfilestring("LÖSEN",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.psw=atoi(buffer);
	if(getcfgfilestring("BYTAREA",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.bytarea=atoi(buffer);
	if(getcfgfilestring("RADAREA",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.radarea=atoi(buffer);
	if(getcfgfilestring("FILER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.filer=atoi(buffer);
	if(getcfgfilestring("LADDANER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.laddaner=atoi(buffer);
	if(getcfgfilestring("EDITOR",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.editor=atoi(buffer);
	if(getcfgfilestring("GRUPPER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.grupper=atoi(buffer);
	Close(fh);
	printf("Status.cfg inläst\n");
}

void initflaggor(void) {
	strcpy(Servermem->flaggnamn[0],"Skyddad status");
	strcpy(Servermem->flaggnamn[1],"Streck under ärendet");
	strcpy(Servermem->flaggnamn[2],"Skriv ut stjärnor vid lösenordspromptar");
	strcpy(Servermem->flaggnamn[3],"Ingen automagisk hjälptext");
	strcpy(Servermem->flaggnamn[4],"Fullskärmseditor");
	strcpy(Servermem->flaggnamn[5],"Automatisk fillista vid lista nyheter");
	strcpy(Servermem->flaggnamn[6],"Mellanslag som paustangent");
	strcpy(Servermem->flaggnamn[7],"Lapp vid brev");
	strcpy(Servermem->flaggnamn[8],"ANSI-sekvenser skickas (markörförflyttningar etc)");
	strcpy(Servermem->flaggnamn[9],"Visa FidoNet Kludge-rader");
	strcpy(Servermem->flaggnamn[10],"Töm skärmen innan en Fido-text");
	strcpy(Servermem->flaggnamn[11],"Inga meddelanden om in/utloggningar");
	strcpy(Servermem->flaggnamn[12],"Färger");
}

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

void getfidocfg(void) {
	BPTR fh;
	int x;
	char buffer[100],*foo1,*foo2;
	if(!(fh=Open("NiKom:DatoCfg/NiKomFido.cfg",MODE_OLDFILE))) {
		printf("Kunde inte läsa in NiKomFido.cfg\n");
		return;
	}
	printf("Läser in NiKomFido.cfg\n");
	for(x = 0; x < 10; x++) Servermem->fidodata.fd[x].domain[0] = 0;
	for(x = 0; x < 20; x++) Servermem->fidodata.fa[x].namn[0] = 0;
	Servermem->fidodata.mailgroups = 0;

	while(FGets(fh,buffer,99)) {
		x=strlen(buffer);
		if(buffer[x-1]=='\n') buffer[x-1]=0;
		if(!strncmp(buffer,"DOMAIN",6)) {
			for(x=0;x<10;x++) if(!Servermem->fidodata.fd[x].domain[0]) break;
			if(x<10) {
				foo1=hittaefter(buffer);
				Servermem->fidodata.fd[x].nummer = atoi(foo1);
				foo1=hittaefter(foo1);
				foo2=hittaefter(foo1);
				foo2[-1]=0;
				strncpy(Servermem->fidodata.fd[x].domain,foo1,19);
				Servermem->fidodata.fd[x].zone=getzone(foo2);
				Servermem->fidodata.fd[x].net=getnet(foo2);
				Servermem->fidodata.fd[x].node=getnode(foo2);
				Servermem->fidodata.fd[x].point=getpoint(foo2);
				foo1=hittaefter(foo2);
				strncpy(Servermem->fidodata.fd[x].zones,foo1,49);
			}
		}
		else if(!strncmp(buffer,"ALIAS",5)) {
			for(x=0;x<20;x++) if(!Servermem->fidodata.fa[x].namn[0]) break;
			if(x<20) {
				foo1=hittaefter(buffer);
				Servermem->fidodata.fa[x].nummer = atoi(foo1);
				foo1=hittaefter(foo1);
				strncpy(Servermem->fidodata.fa[x].namn,foo1,35);
			}
		}
		else if(!strncmp(buffer,"BOUNCE",6)) {
			if(buffer[7]=='Y' || buffer[7]=='y') Servermem->fidodata.bounce=TRUE;
		}
		else if(!strncmp(buffer,"MATRIXDIR",9)) strncpy(Servermem->fidodata.matrixdir,&buffer[10],99);
		else if(!strncmp(buffer,"MAILGROUP",9)) {
			x=parsegrupp(&buffer[10]);
			if(x==-1) printf("I NiKomFido.cfg: Okänd grupp %s\n",&buffer[10]);
			else BAMSET((char *)&Servermem->fidodata.mailgroups,x);
		}
		else if(!strncmp(buffer,"MAILSTATUS",10)) Servermem->fidodata.mailstatus=atoi(&buffer[11]);
		else if(!strncmp(buffer,"FIDOLOGFILE",11)) strncpy(Servermem->fidodata.fidologfile,&buffer[12],99);
		else if(!strncmp(buffer,"DEFAULTORIGIN",13)) strncpy(Servermem->fidodata.defaultorigin,&buffer[14],69);
		else if(!strncmp(buffer,"CRASHSTATUS",11)) Servermem->fidodata.crashstatus = atoi(&buffer[12]);
	}
	Close(fh);
}

void scanfidomoten(void) {
	struct Mote *motpek;
	printf("Scannar Fido-möten...\n");
	motpek = (struct Mote *) Servermem->mot_list.mlh_Head;
	for(;motpek->mot_node.mln_Succ;motpek = (struct Mote *)motpek->mot_node.mln_Succ) {
		if(motpek->type != MOTE_FIDO) continue;
		printf("   %s...\n",motpek->namn);
		ReScanFidoConf(motpek,0);
	}
}

void getnodetypescfg(void) {
	BPTR fh;
	int x,y;
	char buffer[100],*foo1,*foo2;
	if(!(fh=Open("NiKom:DatoCfg/NodeTypes.cfg",MODE_OLDFILE))) {
		printf("Kunde inte läsa in NodeTypes.cfg\n");
		return;
	}
	for(x=0;x<MAXNODETYPES;x++) Servermem->nodetypes[x].nummer=0;
	printf("Läser in NodeTypes.cfg\n");
	while(FGets(fh,buffer,99)) {
		x=strlen(buffer);
		if(buffer[x-1]=='\n') buffer[x-1]=0;
		if(!strncmp(buffer,"NODETYPE",6)) {
			for(x=0;x<MAXNODETYPES;x++) if(!Servermem->nodetypes[x].nummer) break;
			if(x<MAXNODETYPES) {
				foo1=hittaefter(buffer);
				Servermem->nodetypes[x].nummer = atoi(foo1);
				foo1=hittaefter(foo1);
				foo2=hittaefter(foo1);
				for(y=-1; foo2[y]==' ' && &foo2[y] > foo1; y--) foo2[y]=0;
				strcpy(Servermem->nodetypes[x].path,foo1);
				strcpy(Servermem->nodetypes[x].desc,foo2);
			}
		}
	}
	Close(fh);
}

void sparatext(struct NiKMess *message) {
	BPTR fh;
	struct MinList *listpek;
	struct EditLine *elpek;
	struct Header *headpek=(struct Header *)message->data;
	long position;
	int cnt,len;
	char filnamn[40];
	listpek=(struct MinList *)headpek->textoffset;
	headpek->nummer=Servermem->info.hightext+1;
	sprintf(filnamn,"NiKom:Moten/Text%d.dat",headpek->nummer/512);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		if(!(fh=Open(filnamn,MODE_NEWFILE))) {
			printf("Kunde inte öppna %s\n",filnamn);
			return;
		}
	}
	if(Seek(fh,0,OFFSET_END)==-1) {
		printf("Fel vi sökandet av %s\n",filnamn);
		Close(fh);
		return;
	}
	position=Seek(fh,0,OFFSET_CURRENT);
	cnt=0;
	for(elpek=(struct EditLine *)listpek->mlh_Head;elpek->line_node.mln_Succ;elpek=(struct EditLine *)elpek->line_node.mln_Succ) {
		len=strlen(elpek->text);
		elpek->text[len]='\n';
		if(Write(fh,elpek->text,len+1)==-1) {
			printf("Error while saving message\n");
			break;
		}
		cnt++;
	}
	Close(fh);
	sprintf(filnamn,"NiKom:Moten/Head%d.dat",headpek->nummer/512);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		if(!(fh=Open(filnamn,MODE_NEWFILE))) {
			printf("Kunde inte öppna %s\n",filnamn);
			return;
		}
	}
	headpek->textoffset=position;
	headpek->rader=cnt;
	if(Seek(fh,0,OFFSET_END)==-1) {
		printf("Fel vid sökandet i %s\n",filnamn);
		Close(fh);
		return;
	}
	if(Write(fh,(void *)headpek,sizeof(struct Header))==-1) {
		printf("Fel vid skrivandet av %s\n",filnamn);
		Close(fh);
		return;
	}
	Close(fh);
	Servermem->info.hightext++;
	Servermem->texts[Servermem->info.hightext%MAXTEXTS]=headpek->mote;
	if(!(fh=Open("NiKom:DatoCfg/Textmot.dat",MODE_OLDFILE))) {
		printf("Kunde inte öppna Textmot.dat\n");
		return;
	}
	if(Seek(fh,(Servermem->info.hightext%MAXTEXTS)*2,OFFSET_BEGINNING)==-1) {
		printf("Kunde inte söka i Textmot.dat\n",-1);
		Close(fh);
		return;
	}
	if(Write(fh,(void *)&headpek->mote,2)==-1)
		printf("Fel vid skrivandet av Textmot.dat\n");
	Close(fh);
	if(message->nod>=0) BAMCLEAR(Servermem->bitmaps[message->nod],Servermem->info.hightext%MAXTEXTS);
	message->data=Servermem->info.hightext;
	if(Servermem->info.hightext-Servermem->info.lowtext+1==MAXTEXTS) radera(512);
}

void radera(int texter) {
	BPTR fh;
	long oldlowtext=Servermem->info.lowtext;
	int x;
	char filnamn[40];
	char userbitmap[MAXTEXTS/8];
	struct ShortUser *scan;
	if(texter>(Servermem->info.hightext-Servermem->info.lowtext)) {
		printf("Kan inte radera så många texter!\n");
		return;
	}
	if(!texter) {
		printf("Det är ganska meningslöst att radera 0 texter, eller hur..\n");
		return;
	}

	for(x=0;x<texter;x+=512) {
		sprintf(filnamn,"NiKom:Moten/Head%d.dat",(oldlowtext+x)/512);
		DeleteFile(filnamn);
		sprintf(filnamn,"NiKom:Moten/Text%d.dat",(oldlowtext+x)/512);
		DeleteFile(filnamn);
	}
	Servermem->info.lowtext+=texter;
	for(scan=(struct ShortUser *)Servermem->user_list.mlh_Head;scan->user_node.mln_Succ;scan=(struct ShortUser *)scan->user_node.mln_Succ) {
		sprintf(filnamn,"NiKom:Users/%d/%d/Bitmap0",scan->nummer/100,scan->nummer);
		if(!(fh=Open(filnamn,MODE_OLDFILE))) {
			printf("Kunde inte öppna %s\n",filnamn);
			break;
		}
		if(Read(fh,(void *)userbitmap,MAXTEXTS/8)==-1) {
			printf("Fel vid läsandet av %s\n",filnamn);
			Close(fh);
			break;
		}
		if((oldlowtext+texter)%MAXTEXTS > oldlowtext%MAXTEXTS) memset(&userbitmap[(oldlowtext%MAXTEXTS)/8],~0,texter/8);
		else {
			memset(&userbitmap[(oldlowtext%MAXTEXTS)/8],~0,(MAXTEXTS-(oldlowtext%MAXTEXTS))/8);
			memset(userbitmap,~0,(texter-(MAXTEXTS-(oldlowtext%MAXTEXTS)))/8);
		}
		if(Seek(fh,0,OFFSET_BEGINNING)==-1) {
			printf("Kunde inte söka i %s\n",filnamn);
			Close(fh);
			break;
		}
		if(Write(fh,(void *)userbitmap,MAXTEXTS/8)==-1) {
			printf("Fel vid skrivandet av %s\n",filnamn);
			Close(fh);
			break;
		}
		Close(fh);
	}
	for(x=0;x<MAXNOD;x++) {
		if((oldlowtext+texter)%MAXTEXTS > oldlowtext%MAXTEXTS) memset(&Servermem->bitmaps[x][(oldlowtext%MAXTEXTS)/8],~0,texter/8);
		else {
			memset(&Servermem->bitmaps[x][(oldlowtext%MAXTEXTS)/8],~0,(MAXTEXTS-(oldlowtext%MAXTEXTS))/8);
			memset(Servermem->bitmaps[x],~0,(texter-(MAXTEXTS-(oldlowtext%MAXTEXTS)))/8);
		}
	}
}

void writeinfo(void) {
	BPTR fh;
	if(!(fh=Open("NiKom:DatoCfg/Sysinfo.dat",MODE_NEWFILE)))
		printf("Kunde inte öppna Sysinfo.dat\n");
	else {
		if(Write(fh,(void *)&Servermem->info,sizeof(struct SysInfo))==-1)
			printf("Kunde inte skriva Sysinfo.dat\n");
		Close(fh);
	}
}

void addhost(int add) {
	struct RexxMsg *mess;
	struct MsgPort *rexxmastport;
	if(!(mess=(struct RexxMsg *)AllocMem(sizeof(struct RexxMsg),MEMF_CLEAR | MEMF_PUBLIC)))
		cleanup(ERROR,"Kunde inte allokera RexxMsg");
	mess->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
	mess->rm_Node.mn_Length = sizeof(struct RexxMsg);
	mess->rm_Node.mn_ReplyPort=rexxport;
	if(add) mess->rm_Action=RXADDFH;
	else mess->rm_Action=RXREMLIB;
	mess->rm_Args[0]="NIKOMREXXHOST";
	mess->rm_Args[1]=0L;
	Forbid();
	rexxmastport=(struct MsgPort *)FindPort("REXX");
	if(rexxmastport) {
		PutMsg((struct MsgPort *)rexxmastport,(struct Message *)mess);
		Permit();
	} else {
		Permit();
		FreeMem(mess,sizeof(struct RexxMsg));
		if(add) cleanup(ERROR,"Hittar inte ARexx-servern RexxMast");
		else return;
	}
	WaitPort(rexxport);
	GetMsg(rexxport);
	FreeMem(mess,sizeof(struct RexxMsg));
}

void freefilemem(void) {
	int x;
	struct Fil *pekare;
	for(x=0;x<MAXAREA;x++) {
		while(Servermem->areor[x].ar_list.mlh_Head->mln_Succ) {
			pekare=(struct Fil *)Servermem->areor[x].ar_list.mlh_Head;
			RemHead((struct List *)&Servermem->areor[x].ar_list);
			FreeMem(pekare,sizeof(struct Fil));
		}
	}
}

void freecommandmem(void) {
	struct Kommando *pekare;
	while(Servermem->kom_list.mlh_Head->mln_Succ) {
		pekare=(struct Kommando *)Servermem->kom_list.mlh_Head;
		RemHead((struct List *)&Servermem->kom_list);
		FreeMem(pekare,sizeof(struct Kommando));
	}
}

void freemotmem(void) {
	struct Mote *pek;
	while(pek=(struct Mote *)RemHead((struct List *)&Servermem->mot_list))
		FreeMem(pek,sizeof(struct Mote));
}

void freeshortusermem(void) {
	struct ShortUser *pekare;
	while(Servermem->user_list.mlh_Head->mln_Succ) {
		pekare=(struct ShortUser *)Servermem->user_list.mlh_Head;
		RemHead((struct List *)&Servermem->user_list);
		FreeMem(pekare,sizeof(struct ShortUser));
	}
}

void freeloginmem(void) {
	struct SpecialLogin *pek;
	while(pek=(struct SpecialLogin *)RemHead((struct List *)&Servermem->special_login))
		FreeMem(pek,sizeof(struct SpecialLogin));
}

void freegroupmem(void) {
	struct UserGroup *pekare;
	while(Servermem->grupp_list.mlh_Head->mln_Succ) {
		pekare=(struct UserGroup *)Servermem->grupp_list.mlh_Head;
		RemHead((struct List *)&Servermem->grupp_list);
		FreeMem(pekare,sizeof(struct UserGroup));
	}
}

int getkeyfile(void) {
	BPTR fh;
	long checksum,serial,cnt,len,countcs=0;
	char tmp;
	if(!(fh=Open("NiKom:NiKom.key",MODE_OLDFILE))) return(1);
	if(Read(fh,&checksum,sizeof(long))==-1) {
		Close(fh);
		return(0);
	}
	if(Read(fh,&serial,sizeof(long))==-1) {
		Close(fh);
		return(0);
	}
	if(serial%42 && Seek(fh,serial%42,OFFSET_CURRENT)==-1) {
		Close(fh);
		return(0);
	}
	Flush(fh);
	if((len=FGetC(fh))==-1) {
		Close(fh);
		return(0);
	}
	for(cnt=0;cnt<len;cnt++) {
		if((reggadnamn[cnt]=FGetC(fh))==-1) {
			Close(fh);
			return(0);
		}
		tmp=~reggadnamn[cnt];
		tmp&=44;
		reggadnamn[cnt]&=~44;
		reggadnamn[cnt]|=tmp;
		countcs+=reggadnamn[cnt];
		if(FGetC(fh)==-1) {
			Close(fh);
			return(0);
		}
		if(FGetC(fh)==-1) {
			Close(fh);
			return(0);
		}
	}
	Close(fh);
	countcs=countcs*serial*4711/33;
	if(countcs!=checksum) return(0);
}

void cleanup(int kod,char *fel) {
	if(rexxport) {
		addhost(FALSE);
		DeletePort(rexxport);
	}
	if(NiKomBase) InitServermem(NULL); /* Stänger av nikom.library */
	if(Servermem) {
		writeinfo();
		freegroupmem();
		freefilemem();
		freecommandmem();
		freemotmem();
		freeshortusermem();
		freeloginmem();
		FreeMem(Servermem,sizeof(struct System));
	}
	if(NiKWindow) CloseWindow(NiKWindow);
	if(nodereplyport) DeleteMsgPort(nodereplyport);
	if(permitport) DeletePort(permitport);
	if(NiKPort) DeletePort(NiKPort);
	if(NiKomBase) CloseLibrary(NiKomBase);
	if(UtilityBase) CloseLibrary(UtilityBase);
	if(RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	printf("%s",fel);
	exit(kod);
}

void main() {
	int Going=TRUE,noder=0,x=0,wheight;
	struct MsgPort *port;
	long windmask, portmask, signals, rexxmask, nodereplymask;
	struct NiKMess *MyNiKMess,*dummymess;
	struct IntuiMessage *myintmess;
	struct RexxMsg *rexxmess;
	struct Screen *lockscreen;
	char titel[100],*tmppscreen,nikomrel[20];
	Forbid();
	port=(struct MsgPort *)FindPort("NiKomPort");
	Permit();
	if(port) {
		printf("Det finns redan en server igång.\n");
		exit(10);
	}
	if(!getkeyfile()) {
		printf("Korrupt nyckelfil.\n");
		exit(10);
	}
	if(!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",VERSION)))
		cleanup(ERROR,"Kunde inte öppna intuition.library");
	if(!(RexxSysBase=(struct RsxLib *)OpenLibrary("rexxsyslib.library",0L)))
		cleanup(ERROR,"Kunde inte öppna rexxsys.library");
	if(!(UtilityBase=OpenLibrary("utility.library",37L)))
		cleanup(ERROR,"Kunde inte öppna utility.library");
	if(!(NiKomBase = OpenLibrary("nikom.library",19L)))
		cleanup(ERROR,"Kunde inte öppna nikom.library");
	if(!(NiKPort = (struct MsgPort *)CreatePort("NiKomPort",0)))
		cleanup(ERROR,"Kunde inte öppna NiKPort");
	if(!(permitport = (struct MsgPort *)CreatePort("NiKomPermit",0)))
		cleanup(ERROR,"Kunde inte öppna permitport");
	if(!(nodereplyport = CreateMsgPort()))
		cleanup(ERROR,"Kunde inte öppna nodsvarsporten.");
	if(!(Servermem = (struct System *)AllocMem(sizeof(struct System),MEMF_PUBLIC | MEMF_CLEAR)))
		cleanup(ERROR,"Ej tillräckligt med minne");
	portmask = 1L << NiKPort->mp_SigBit;
	NewList((struct List *)&Servermem->user_list);
	NewList((struct List *)&Servermem->grupp_list);
	NewList((struct List *)&Servermem->shell_list);
	NewList((struct List *)&Servermem->kom_list);
	NewList((struct List *)&Servermem->mot_list);
	for(x=0;x<MAXAREA;x++) NewList((struct List *)&Servermem->areor[x].ar_list);
	for(x=0; x < NIKSEM_NOOF; x++) InitSemaphore(&Servermem->semaphores[x]);
	getinfo();
	getgrupper();
	getkmd();
	getconfig();
	getmoten();
	gettextmot();
	getnamn();
	getfidocfg();
	getnodetypescfg();
	getsenaste();
	initflaggor();
	getnycklar();
	getstatus();
	getareor();
	getfiler();
	InitServermem(Servermem); /* Kör igång nikom.library */
	GetServerversion();
	scanfidomoten();

	for(x=0;x<MAXNOD;x++) {
		Servermem->nodtyp[x]=0;
		Servermem->inloggad[x]=-1;
		Servermem->action[x]=0;
		Servermem->maxinactivetime[x]=5;
/*		Servermem->PrgCat[x] = NULL; */
		Servermem->watchserial[x] = 1;
/*		Servermem->UserCached[x].usernumber = -1;
		Servermem->UserCached[x].LatestUsedUserCache = -1; */
	}
	GetNiKomVersion(NULL,NULL,nikomrel);
	sprintf(titel,"NiKom %s © Tomas Kärki 0 noder aktiva", nikomrel);
	if(pubscreen[0]=='-') tmppscreen=NULL;
	else tmppscreen=pubscreen;
	if(!(lockscreen=LockPubScreen(tmppscreen)))
		cleanup(ERROR,"Kunde inte låsa angiven Public Screen\n");
	wheight=lockscreen->WBorTop + lockscreen->Font->ta_YSize + 1;
	if(!(NiKWindow = (struct Window *)OpenWindowTags(NULL,WA_Left,xpos,
																			WA_Top,ypos,
																			WA_Width,500,
																			WA_Height,wheight,
																			WA_IDCMP,IDCMP_CLOSEWINDOW,
																			WA_Title,titel,
																			WA_SizeGadget,FALSE,
																			WA_DragBar,TRUE,
																			WA_DepthGadget,TRUE,
																			WA_CloseGadget,TRUE,
																			WA_NoCareRefresh,TRUE,
																			WA_ScreenTitle,"NiKom © Tomas Kärki 1996-1998.",
																			WA_AutoAdjust,TRUE,
																			WA_PubScreen,lockscreen))) {
		UnlockPubScreen(NULL,lockscreen);
		cleanup(ERROR,"Kunde inte öppna fönstret");
	}
	UnlockPubScreen(NULL,lockscreen);
	windmask = 1L << NiKWindow->UserPort->mp_SigBit;
	if(!(rexxport=(struct MsgPort *)CreatePort("NIKOMREXXHOST",0)))
		cleanup(ERROR,"Kunde inte öppna rexxporten");
	rexxmask = 1L << rexxport->mp_SigBit;
	nodereplymask = 1l << nodereplyport->mp_SigBit;
	addhost(TRUE);
	printf("NiKServer klar.\n");
	while(Going) {
		signals=Wait(windmask | portmask | rexxmask | nodereplymask | SIGBREAKF_CTRL_C);
		if(signals & windmask) {
			myintmess=(struct IntuiMessage *)GetMsg(NiKWindow->UserPort);
			ReplyMsg((struct Message *)myintmess);
			if(!noder) cleanup(OK,"");
		}
		if(signals & SIGBREAKF_CTRL_C) if(!noder) cleanup(OK,"");
		if(signals & portmask) {
			while(MyNiKMess=(struct NiKMess *)GetMsg(NiKPort)) {
				switch(MyNiKMess->kommando) {
					case NYNOD :
						if(noder<MAXNOD) {
							noder++;
							x=0;
							while(Servermem->nodtyp[x++]);
							Servermem->nodtyp[--x]=MyNiKMess->data;
							MyNiKMess->nod=x;
							MyNiKMess->data=(long)Servermem;
						} else {
							MyNiKMess->data=NULL;
						}
						sprintf(titel,"NiKom %s © Tomas Kärki  %d noder aktiva", nikomrel, noder);
						SetWindowTitles(NiKWindow,titel,(UBYTE *)-1L);
						break;

					case NODSLUTAR :
						noder--;
						Servermem->nodtyp[MyNiKMess->nod]=0;
						sprintf(titel,"NiKom %s © Tomas Kärki  %d noder aktiva", nikomrel, noder);
						SetWindowTitles(NiKWindow,titel,(UBYTE *)-1L);
						break;

					case SPARATEXTEN :
						sparatext(MyNiKMess);
					/*	if(Servermem->info.hightext-Servermem->info.lowtext+1==MAXTEXTS) radera(512); */
					/* TK970313: Adderat i slutet av sparatext() funktionen istället. */
						break;
					case FORBID :
						ReplyMsg((struct Message *)MyNiKMess);
						WaitPort(permitport);
						dummymess=(struct NiKMess *)GetMsg(permitport);
						break;
					case RADERATEXTER :
						radera(MyNiKMess->data);
						break;
					case READCFG :
						getconfig();
						freecommandmem();
						getkmd();
						getnycklar();
						getstatus();
						getnodetypescfg();
						getfidocfg();
						break;
					case WRITEINFO :
						writeinfo();
						break;
					case GETADRESS :
						MyNiKMess->data=(long)Servermem;
						break;
					case NIKMESS_SETNODESTATE :
						setnodestate(MyNiKMess);
						break;
				}
				ReplyMsg((struct Message *)MyNiKMess);
			}
		}
		if(signals & rexxmask) {
			while(rexxmess=(struct RexxMsg *)GetMsg(rexxport)) {
				handlerexx(rexxmess);
				ReplyMsg((struct Message *)rexxmess);
			}
		}
		if(signals & nodereplymask) {
			while(MyNiKMess = (struct NiKMess *) GetMsg(nodereplyport))
				FreeMem(MyNiKMess,sizeof(struct NiKMess));
		}
	}
}
