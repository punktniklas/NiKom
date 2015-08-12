#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"

#define ERROR	128
#define OK	0
#define EKO	1
#define EJEKO	0
#define NOCARRIER	32

int CXBRK(void) { return(0); }

extern long textpek,logintime;
extern int area2, dtespeed, radcnt, nodnr, nodestate;
extern struct MinList aliaslist,edit_list;
extern char outbuffer[];
extern struct System *Servermem;

struct IntuitionBase *IntuitionBase=NULL;
struct RsxLib *RexxSysBase;
struct Library *UtilityBase, *NiKomBase;
struct Window *NiKwind=NULL;
struct MsgPort *rexxport, *nikomnodeport;
char rexxportnamn[15], pubscreen[40], nikomnodeportnamn[15];
int inloggad, ypos,xpos,ysize,xsize;

/* Följande behövs för att SerialIO ska kunna användas även i PreNoden */
char svara[17],init[81],hangup[32],nodid[20];
int highbaud, hst, getty, hangupdelay, gettybps, autoanswer, plussa;

void freealiasmem(void) {
	struct Alias *pekare;
	while(pekare=(struct Alias *)RemHead((struct List *)&aliaslist))
		FreeMem(pekare,sizeof(struct Alias));
}

void cleanup(int kod,char *text)
{
	freealiasmem();
	freeeditlist();
	CloseIO();
	if(NiKwind) CloseWindow(NiKwind);
	if(RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
	if(nikomnodeport) {
		RemPort(nikomnodeport);
		DeleteMsgPort(nikomnodeport);
	}
	if(rexxport) {
		RemPort(rexxport);
		DeleteMsgPort(rexxport);
	}
	shutdownnode(NODSPAWNED);
	if(NiKomBase) CloseLibrary(NiKomBase);
	if(UtilityBase) CloseLibrary(UtilityBase);
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	printf("%s",text);
	exit(kod);
}

void main(int argc,char *argv[]) {
	int x;
	long tid;
	char tellstr[100],*tmppscreen, titel[80], configname[50] = "NiKom:DatoCfg/SerNode.cfg";
	if(argc>1) for(x=1;x<argc;x++) {
		if(argv[x][0]=='-') {
			if(argv[x][1]=='B') dtespeed=atoi(&argv[x][2]);
			else if(argv[x][1]=='N') nodnr = atoi(&argv[x][2]);
		} else strcpy(configname,argv[x]);
	}
	if(nodnr==-1) {
		printf("NiKomSer måste startas från prenoden.\n");
		exit(10);
	}
	NewList((struct List *)&aliaslist);
	NewList((struct List *)&edit_list);
	if(!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0)))
		cleanup(ERROR,"Kunde inte öppna intuition.library\n");
	if(!(UtilityBase=OpenLibrary("utility.library",37L)))
		cleanup(ERROR,"Kunde inte öppna utility.library\n");
	if(!(NiKomBase=OpenLibrary("nikom.library",0L)))
		cleanup(ERROR,"Kunde inte öppna nikom.library\n");

	initnode(NODSPAWNED);
	if(!(nikomnodeport = CreateMsgPort()))
		cleanup(ERROR,"Kunde inte skapa NiKomNode-porten");
	sprintf(nikomnodeportnamn,"NiKomNode%d",nodnr);
	nikomnodeport->mp_Node.ln_Name = nikomnodeportnamn;
	nikomnodeport->mp_Node.ln_Pri = 1;
	AddPort(nikomnodeport);
	sprintf(rexxportnamn,"NiKomRexx%d",nodnr);
	if(!(rexxport=(struct MsgPort *)CreateMsgPort()))
		cleanup(ERROR,"Kunde inte öppna RexxPort\n");
	rexxport->mp_Node.ln_Name=rexxportnamn;
	rexxport->mp_Node.ln_Pri = 50;
	AddPort(rexxport);
	if(!(RexxSysBase=(struct RsxLib *)OpenLibrary("rexxsyslib.library",0L)))
		cleanup(ERROR,"Kunde inte öppna rexxsyslib.library\n");
	getnodeconfig(configname);
	if(pubscreen[0]=='-') tmppscreen=NULL;
	else tmppscreen=pubscreen;
	if(!(NiKwind=(struct Window *)OpenWindowTags(NULL,WA_Left,xpos,
																	  WA_Top,ypos,
																	  WA_Width,xsize,
																	  WA_Height,ysize,
																	  WA_IDCMP,IDCMP_CLOSEWINDOW,
																	  WA_MinWidth,50,
																	  WA_MinHeight,10,
																	  WA_MaxWidth,~0,
																	  WA_MaxHeight,~0,
																	  WA_SizeGadget,TRUE,
																	  WA_SizeBBottom, TRUE,
																	  WA_DragBar,TRUE,
																	  WA_DepthGadget,TRUE,
																	  WA_CloseGadget,TRUE,
																	  WA_SimpleRefresh,TRUE,
																	  WA_ScreenTitle,"NiKom © Tomas Kärki 1996-1998",
																	  WA_AutoAdjust,TRUE,
																	  WA_PubScreenName,tmppscreen,
																	  TAG_DONE)))
		cleanup(ERROR,"Kunde inte öppna fönstret\n");
	if(!getkeyfile())
		cleanup(ERROR,"Korrupt nyckelfil\n");
	if(!OpenIO(NiKwind)) cleanup(ERROR,"Kunde inte öppna IO\n");
	inloggad=Servermem->inloggad[nodnr];
	conreqtkn();
	serreqtkn();
	sprintf(titel,"Nod #%d SER: %s #%d",nodnr,Servermem->inne[nodnr].namn,inloggad);
	SetWindowTitles(NiKwind,titel,(char *)-1L);
        ReadUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad);
	if(getft("NiKom:Texter/Bulletin.txt")>Servermem->inne[nodnr].senast_in) sendfile("NiKom:Texter/Bulletin.txt");
	connection();

	if(nodestate & NIKSTATE_NOCARRIER) {
		conputtekn("\nCarrier dropped\n",-1);
		if(Servermem->cfg.logmask & LOG_CARDROPPED) {
			sprintf(outbuffer,"%s släpper carriern (nod %d)",getusername(inloggad),nodnr);
			logevent(outbuffer);
		}
		if(Servermem->cfg.ar.cardropped) sendrexx(Servermem->cfg.ar.cardropped);
	} else {
		if(nodestate & NIKSTATE_LOGOUT) {
			nodestate &= ~NIKSTATE_LOGOUT;
			puttekn("\n\n\r*** Automagisk utloggning ***\n\n\r",-1);
		} else if(nodestate & NIKSTATE_INACTIVITY) {
			nodestate &= ~NIKSTATE_INACTIVITY;
			puttekn("\n\n\r*** Utloggning p.g.a inaktivitet ***\n\n\r",-1);
		}
		radcnt=-174711;
		if(Servermem->say[nodnr]) displaysay();
		if(Servermem->cfg.ar.utlogg) sendrexx(Servermem->cfg.ar.utlogg);
		sendfile("NiKom:Texter/Utlogg.txt");
	}
	SaveProgramCategory(inloggad);
	Servermem->inloggad[nodnr]=-1;
	if(Servermem->cfg.logmask & LOG_UTLOGG) {
		sprintf(outbuffer,"%s loggar ut från nod %d",getusername(inloggad),nodnr);
		logevent(outbuffer);
	}
	Servermem->action[nodnr]=0;
	Servermem->inne[nodnr].textpek=textpek;
	time(&tid);
	Servermem->inne[nodnr].senast_in=tid;
	Servermem->inne[nodnr].tot_tid+=(tid-logintime);
	Servermem->inne[nodnr].loggin++;
	Servermem->info.inloggningar++;
	Servermem->inne[nodnr].defarea=area2;
	writeuser(inloggad,&Servermem->inne[nodnr]);
        ReadUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad);
	writesenaste();
	abortinactive();
	freealiasmem();
	sprintf(tellstr,"loggade just ut från nod %d",nodnr);
	tellallnodes(tellstr);
	cleanup(nodestate,"");
}

