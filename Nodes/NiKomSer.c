#include "NiKomCompat.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include <proto/exec.h>
#ifdef HAVE_PROTO_ALIB_H
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "InfoFiles.h"
#include "Logging.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "BasicIO.h"
#include "Languages.h"
#include "UserDataUtils.h"
#include "UserMessageUtils.h"

#include "HeartBeat.h"

#define EXIT_ERROR	128

int CXBRK(void) { return(0); }

extern long logintime;
extern int area2, dtespeed, radcnt, nodnr, nodestate;
extern struct MinList aliaslist,edit_list;
extern char outbuffer[];
extern struct System *Servermem;
extern struct Inloggning Statstr;

struct IntuitionBase *IntuitionBase=NULL;
struct RsxLib *RexxSysBase=NULL;
struct Library *UtilityBase, *NiKomBase;
struct Window *NiKwind=NULL;
struct MsgPort *rexxport, *nikomnodeport;
char rexxportnamn[15], pubscreen[40], nikomnodeportnamn[15];
int inloggad = -1, g_userDataSlot = -1, ypos,xpos,ysize,xsize;
struct User g_preLoginUserData;

/* Följande behövs för att SerialIO ska kunna användas även i PreNoden */
char svara[17],init[81],hangup[32],nodid[20];
int highbaud, hst, getty, hangupdelay, gettybps, autoanswer, plussa;

static void freealiasmem(void) {
	struct Alias *pekare;
	while((pekare=(struct Alias *)RemHead((struct List *)&aliaslist)))
		FreeMem(pekare,sizeof(struct Alias));
}

void cleanup(int kod,char *text) {
  CloseCatalog(g_Catalog);
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
	if(LocaleBase) CloseLibrary((struct Library *)LocaleBase);
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	printf("%s",text);
	exit(kod);
}

void saveUserData(void) {
  WriteUser(inloggad, CURRENT_USER, FALSE);
  WriteUnreadTexts(CUR_USER_UNREAD, inloggad);
}

int main(int argc,char *argv[]) {
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
    cleanup(EXIT_ERROR,"Kunde inte öppna intuition.library\n");
  if(!(UtilityBase=OpenLibrary("utility.library",37L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna utility.library\n");
  if(!(LocaleBase=(NiKomLocaleType *)OpenLibrary("locale.library",38L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna locale.library\n");
  if(!(NiKomBase=OpenLibrary("nikom.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna nikom.library\n");

  initnode(NODSPAWNED);
  if(!(nikomnodeport = CreateMsgPort()))
    cleanup(EXIT_ERROR,"Kunde inte skapa NiKomNode-porten");
  sprintf(nikomnodeportnamn,"NiKomNode%d",nodnr);
  nikomnodeport->mp_Node.ln_Name = nikomnodeportnamn;
  nikomnodeport->mp_Node.ln_Pri = 1;
  AddPort(nikomnodeport);
  sprintf(rexxportnamn,"NiKomRexx%d",nodnr);
  if(!(rexxport=(struct MsgPort *)CreateMsgPort()))
    cleanup(EXIT_ERROR,"Kunde inte öppna RexxPort\n");
  rexxport->mp_Node.ln_Name=rexxportnamn;
  rexxport->mp_Node.ln_Pri = 50;
  AddPort(rexxport);
  if(!(RexxSysBase=(struct RsxLib *)OpenLibrary("rexxsyslib.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna rexxsyslib.library\n");
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
    cleanup(EXIT_ERROR,"Kunde inte öppna fönstret\n");
  if(!OpenIO(NiKwind)) cleanup(EXIT_ERROR,"Kunde inte öppna IO\n");
  inloggad = Servermem->nodeInfo[nodnr].userLoggedIn;
  g_userDataSlot = Servermem->nodeInfo[nodnr].userDataSlot;
  conreqtkn();
  serreqtkn();
  StartHeartBeat(TRUE);
  sprintf(titel,"Nod #%d SER: %s #%d",nodnr,CURRENT_USER->namn,inloggad);
  SetWindowTitles(NiKwind,titel,(char *)-1L);
  if(!ReadUnreadTexts(CUR_USER_UNREAD, inloggad)) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't read unread text info for user %d", inloggad);
    DisplayInternalError();
    cleanup(EXIT_ERROR, "Error reading unread text info.\n");
  }

  connection();

  if(nodestate & NIKSTATE_NOCARRIER) {
    conputtekn("\nCarrier dropped\n",-1);
    if(Servermem->cfg->logmask & LOG_CARDROPPED) {
      LogEvent(USAGE_LOG, WARN, "%s släpper carriern (nod %d)",
               getusername(inloggad), nodnr);
    }
    if(Servermem->cfg->ar.cardropped) sendautorexx(Servermem->cfg->ar.cardropped);
  } else {
    if(nodestate & NIKSTATE_AUTOLOGOUT) {
      SendString("\n\n\r*** %s ***\n\n\r", CATSTR(MSG_KOM_AUTO_LOGOUT));
    } else if(nodestate & NIKSTATE_INACTIVITY) {
      SendString("\n\n\r*** %s ***\n\n\r", CATSTR(MSG_KOM_INACTIVITY_LOGOUT));
    }
    radcnt=-174711;
    displaysay();
    if(Servermem->cfg->ar.utlogg) sendautorexx(Servermem->cfg->ar.utlogg);
    SendInfoFile("Logout.txt", 0);
  }
  Servermem->nodeInfo[nodnr].userLoggedIn = -1;
  if(Servermem->cfg->logmask & LOG_UTLOGG) {
    LogEvent(USAGE_LOG, INFO, "%s loggar ut från nod %d (%d skrivna, %d lästa)",
             getusername(inloggad), nodnr, Statstr.write, Statstr.read);
  }
  sprintf(tellstr,"loggade just ut från nod %d",nodnr);
  SendUserMessage(inloggad, -1, tellstr, NIK_MESSAGETYPE_LOGNOTIFY);
  Servermem->nodeInfo[nodnr].action = 0;
  time(&tid);
  CURRENT_USER->senast_in=tid;
  CURRENT_USER->tot_tid+=(tid-logintime);
  CURRENT_USER->loggin++;
  Servermem->info.inloggningar++;
  CURRENT_USER->defarea=area2;
  writesenaste();
  StopHeartBeat();
  saveUserData();
  freealiasmem();

  if(nodestate & NIKSTATE_NOCARRIER) {
    nodestate &= ~NIKSTATE_RELOGIN;
  }
  nodestate &= (NIKSTATE_RELOGIN | NIKSTATE_CLOSESER | NIKSTATE_NOANSWER);
  cleanup(nodestate,"");
  return 0;
}

void HandleHeartBeat(void) {
  CheckInactivity();
  saveUserData();
  HandleKeepAlive();
}
