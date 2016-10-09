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
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Terminal.h"
#include "BasicIO.h"

#define EXIT_ERROR	128

int CXBRK(void) { return(0); }

extern long logintime;
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

/* F�ljande beh�vs f�r att SerialIO ska kunna anv�ndas �ven i PreNoden */
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
    printf("NiKomSer m�ste startas fr�n prenoden.\n");
    exit(10);
  }
  NewList((struct List *)&aliaslist);
  NewList((struct List *)&edit_list);
  if(!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0)))
    cleanup(EXIT_ERROR,"Kunde inte �ppna intuition.library\n");
  if(!(UtilityBase=OpenLibrary("utility.library",37L)))
    cleanup(EXIT_ERROR,"Kunde inte �ppna utility.library\n");
  if(!(NiKomBase=OpenLibrary("nikom.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte �ppna nikom.library\n");

  initnode(NODSPAWNED);
  if(!(nikomnodeport = CreateMsgPort()))
    cleanup(EXIT_ERROR,"Kunde inte skapa NiKomNode-porten");
  sprintf(nikomnodeportnamn,"NiKomNode%d",nodnr);
  nikomnodeport->mp_Node.ln_Name = nikomnodeportnamn;
  nikomnodeport->mp_Node.ln_Pri = 1;
  AddPort(nikomnodeport);
  sprintf(rexxportnamn,"NiKomRexx%d",nodnr);
  if(!(rexxport=(struct MsgPort *)CreateMsgPort()))
    cleanup(EXIT_ERROR,"Kunde inte �ppna RexxPort\n");
  rexxport->mp_Node.ln_Name=rexxportnamn;
  rexxport->mp_Node.ln_Pri = 50;
  AddPort(rexxport);
  if(!(RexxSysBase=(struct RsxLib *)OpenLibrary("rexxsyslib.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte �ppna rexxsyslib.library\n");
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
                                               WA_ScreenTitle,"NiKomSer",
                                               WA_AutoAdjust,TRUE,
                                               WA_PubScreenName,tmppscreen,
                                               TAG_DONE)))
    cleanup(EXIT_ERROR,"Kunde inte �ppna f�nstret\n");
  if(!OpenIO(NiKwind)) cleanup(EXIT_ERROR,"Kunde inte �ppna IO\n");
  inloggad=Servermem->inloggad[nodnr];
  conreqtkn();
  serreqtkn();
  UpdateInactive();
  sprintf(titel,"Nod #%d SER: %s #%d",nodnr,Servermem->inne[nodnr].namn,inloggad);
  SetWindowTitles(NiKwind,titel,(char *)-1L);
  if(!ReadUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad)) {
    puttekn("Error reading unread text info.\r\n", -1);
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't read unread text info for user %d", inloggad);
    cleanup(EXIT_ERROR, "Error reading unread text info.\n");
  }
  if(getft("NiKom:Texter/Bulletin.txt")>Servermem->inne[nodnr].senast_in) {
    sendfile("NiKom:Texter/Bulletin.txt");
  }

  connection();

  if(nodestate & NIKSTATE_NOCARRIER) {
    conputtekn("\nCarrier dropped\n",-1);
    if(Servermem->cfg.logmask & LOG_CARDROPPED) {
      LogEvent(USAGE_LOG, WARN, "%s sl�pper carriern (nod %d)",
               getusername(inloggad), nodnr);
    }
    if(Servermem->cfg.ar.cardropped) sendautorexx(Servermem->cfg.ar.cardropped);
  } else {
    if(nodestate & NIKSTATE_AUTOLOGOUT) {
      puttekn("\n\n\r*** Automagisk utloggning ***\n\n\r",-1);
    } else if(nodestate & NIKSTATE_INACTIVITY) {
      puttekn("\n\n\r*** Utloggning p.g.a inaktivitet ***\n\n\r",-1);
    }
    radcnt=-174711;
    if(Servermem->say[nodnr]) displaysay();
    if(Servermem->cfg.ar.utlogg) sendautorexx(Servermem->cfg.ar.utlogg);
    sendfile("NiKom:Texter/Utlogg.txt");
  }
  Servermem->inloggad[nodnr]=-1;
  if(Servermem->cfg.logmask & LOG_UTLOGG) {
    LogEvent(USAGE_LOG, INFO, "%s loggar ut fr�n nod %d",
             getusername(inloggad), nodnr);
  }
  Servermem->action[nodnr]=0;
  time(&tid);
  Servermem->inne[nodnr].senast_in=tid;
  Servermem->inne[nodnr].tot_tid+=(tid-logintime);
  Servermem->inne[nodnr].loggin++;
  Servermem->info.inloggningar++;
  Servermem->inne[nodnr].defarea=area2;
  writeuser(inloggad,&Servermem->inne[nodnr]);
  WriteUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad);
  writesenaste();
  AbortInactive();
  freealiasmem();
  sprintf(tellstr,"loggade just ut fr�n nod %d",nodnr);
  tellallnodes(tellstr);

  if(nodestate & NIKSTATE_NOCARRIER) {
    nodestate &= ~NIKSTATE_RELOGIN;
  }
  nodestate &= (NIKSTATE_RELOGIN | NIKSTATE_CLOSESER | NIKSTATE_NOANSWER);
  cleanup(nodestate,"");
}
