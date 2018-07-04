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
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "InfoFiles.h"
#include "Logging.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "NewUser.h"
#include "Languages.h"
#include "UserDataUtils.h"
#include "UserMessageUtils.h"

#define EXIT_ERROR	10
#define EXIT_OK	0

int CXBRK(void) { return(0); }

extern long logintime;
extern int area2, nodnr, nodestate;
extern struct MinList aliaslist,edit_list;
extern char outbuffer[], nodid[], inmat[];
extern struct System *Servermem;
extern struct Inloggning Statstr;

struct IntuitionBase *IntuitionBase=NULL;
struct RsxLib *RexxSysBase=NULL;
struct Library *UtilityBase, *NiKomBase;
struct Window *NiKwind=NULL;
struct MsgPort *rexxport, *nikomnodeport;

char rexxportnamn[15], pubscreen[40], nikomnodeportnamn[15];
int inloggad = -1, g_userDataSlot = -1, ypos, xpos, ysize, xsize;
struct User g_preLoginUserData;
struct UnreadTexts g_newUserUnreadTexts;
extern char commandhistory[];

static void freealiasmem(void) {
	struct Alias *pekare;
	while((pekare=(struct Alias *)RemHead((struct List *)&aliaslist)))
		FreeMem(pekare,sizeof(struct Alias));
}

void cleanup(int kod,char *fel) {
  CloseCatalog(g_Catalog);
	shutdownnode(NODCON);
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
	if(NiKomBase) CloseLibrary(NiKomBase);
	if(UtilityBase) CloseLibrary(UtilityBase);
	if(LocaleBase) CloseLibrary((struct Library *)LocaleBase);
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	printf("%s",fel);
	exit(kod);
}

int main(int argc, char **argv) {
  int going = TRUE, forsok = 2, ch, newUser, tmpUserId;
  long tid;
  char tellstr[100],*tmppscreen, titel[80];

  NewList((struct List *)&aliaslist);
  NewList((struct List *)&edit_list);
  if(!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0)))
    cleanup(EXIT_ERROR,"Kunde inte öppna intuition.library\n");
  if(!(UtilityBase=OpenLibrary("utility.library",37L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna utility.library\n");
  if(!(LocaleBase=(NiKomLocaleType *)OpenLibrary("locale.library",38L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna locale.library\n");
  if(!(NiKomBase=OpenLibrary("nikom.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna nikom.linbrary");
  getnodeconfig("NiKom:Datocfg/ConNode.cfg");
  if(!(initnode(NODCON))) cleanup(EXIT_ERROR,"Kunde inte registrera noden i Servern\n");
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
  rexxport->mp_Node.ln_Pri=50;
  AddPort(rexxport);
  if(!(RexxSysBase=(struct RsxLib *)OpenLibrary("rexxsyslib.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna rexxsyslib.library\n");
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
                                               WA_ScreenTitle,"NiKomCon",
                                               WA_AutoAdjust,TRUE,
                                               WA_PubScreenName,tmppscreen,
                                               TAG_DONE)))
    cleanup(EXIT_ERROR,"Kunde inte öppna fönstret\n");
  if(!OpenIO(NiKwind)) cleanup(EXIT_ERROR,"Kunde inte öppna IO\n");
  strcpy(Servermem->nodeInfo[nodnr].nodeIdStr, nodid);
  sprintf(titel,"Nod #%d CON: <Ingen inloggad>",nodnr);
  SetWindowTitles(NiKwind,titel,(UBYTE *)-1L);
  Servermem->nodeInfo[nodnr].connectBps = -1;
  conreqtkn();
  do {
    Servermem->nodeInfo[nodnr].lastActiveTime = time(NULL);
    memset(commandhistory,0,1024);
    sendfile("NiKom:Texter/Login.txt");
    if(Servermem->cfg->ar.preinlogg) sendautorexx(Servermem->cfg->ar.preinlogg);
    going=TRUE;
    newUser = FALSE;
    while(going) {
      Servermem->nodeInfo[nodnr].lastActiveTime = time(NULL);
      SendStringNoBrk("\r\nName: ");
      getstring(EKO,40,NULL);
      if(!stricmp(inmat,Servermem->cfg->ny)) {
        tmpUserId = RegisterNewUser(&g_preLoginUserData);
        if(tmpUserId == -2) {
          goto panik;
        }
        newUser = TRUE;
        going=FALSE;
      }
      else if((tmpUserId = parsenamn(inmat)) >= 0) {
        if(!ReadUser(tmpUserId, &g_preLoginUserData)) {
          goto panik;
        }
        // TODO: Extract password loop. Should be identical to in PreNode/Ser.c
        forsok=2;
        while(forsok) {
          SendStringNoBrk("\r\nPassword: ");
          if(g_preLoginUserData.flaggor & STAREKOFLAG)
            getstring(STAREKO,15,NULL);
          else
            getstring(EJEKO,15,NULL);
          if(CheckPassword(inmat, g_preLoginUserData.losen)) {
            forsok=FALSE;
            going=FALSE;
          } else {
            forsok--;
          }
        }
      } else if(tmpUserId == -1) {
        SendStringNoBrk("\r\nNo such user\r\n");
      }
    }

    inloggad = tmpUserId;
    Servermem->nodeInfo[nodnr].userLoggedIn = inloggad;
    switch(AllocateUserDataSlot(nodnr, inloggad)) {
    case 0:
      LogEvent(SYSTEM_LOG, ERROR, "Can't allocate userDataSlot for user %d", inloggad);
      DisplayInternalError();
      goto panik;
    case 1:
      break;
    case 2:
      memcpy(&Servermem->userData[Servermem->nodeInfo[nodnr].userDataSlot], &g_preLoginUserData, sizeof(struct User));
      if(newUser) {
        memcpy(&Servermem->unreadTexts[Servermem->nodeInfo[nodnr].userDataSlot],
               &g_newUserUnreadTexts, sizeof(struct UnreadTexts));
      } else {
        if(!ReadUnreadTexts(&Servermem->unreadTexts[Servermem->nodeInfo[nodnr].userDataSlot], inloggad)) {
          LogEvent(SYSTEM_LOG, ERROR,
                   "Can't read unread text info for user %d", inloggad);
          DisplayInternalError();
          goto panik;
        }
      }
      break;
    }
    g_userDataSlot = Servermem->nodeInfo[nodnr].userDataSlot;
    Servermem->nodeInfo[nodnr].lastActiveTime = time(NULL);
    if(newUser) {
      if(Servermem->cfg->ar.nyanv) {
        sendrexx(Servermem->cfg->ar.nyanv);
      }
    }

    sprintf(titel,"Nod #%d CON: %s #%d",nodnr,CURRENT_USER->namn,inloggad);
    SetWindowTitles(NiKwind,titel,(UBYTE *)-1L);
    SendInfoFile("Bulletin.txt", CURRENT_USER->senast_in);

    connection();

    if(Servermem->cfg->logmask & LOG_UTLOGG) {
      LogEvent(USAGE_LOG, INFO, "%s loggar ut från nod %d (%d skrivna, %d lästa)",
               getusername(inloggad), nodnr, Statstr.write, Statstr.read);
    }
    displaysay();
    if(Servermem->cfg->ar.utlogg) sendautorexx(Servermem->cfg->ar.utlogg);
    SendInfoFile("Logout.txt", 0);
    sprintf(tellstr,"loggade just ut från nod %d",nodnr);
    SendUserMessage(inloggad, -1, tellstr, NIK_MESSAGETYPE_LOGNOTIFY);
    sprintf(titel,"Nod #%d CON: <Ingen inloggad>",nodnr);
    SetWindowTitles(NiKwind,titel,(UBYTE *)-1L);
    Servermem->nodeInfo[nodnr].userLoggedIn = -1;
    
    Servermem->nodeInfo[nodnr].action = 0;
    time(&tid);
    CURRENT_USER->senast_in=tid;
    CURRENT_USER->tot_tid+=(tid-logintime);
    CURRENT_USER->loggin++;
    Servermem->info.inloggningar++;
    CURRENT_USER->defarea=area2;
    WriteUser(inloggad, CURRENT_USER, FALSE);
    WriteUnreadTexts(CUR_USER_UNREAD, inloggad);
    ReleaseUserDataSlot(nodnr);
    inloggad = -1;
    writesenaste();
    freealiasmem();
  panik:
    nodestate = 0;
    SendString("\r\n\nOne more login? (Y/n)",-1);
  } while((ch = GetChar()) != 'n' && ch != 'N');
  cleanup(EXIT_OK,"");
  return 0;
}
