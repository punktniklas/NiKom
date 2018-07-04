#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomLib.h"
#include "PreNodeFuncs.h"
#include "Logging.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "NewUser.h"
#include "BasicIO.h"
#include "Languages.h"
#include "UserDataUtils.h"

#include "HeartBeat.h"

#define EXIT_ERROR	10
#define EXIT_OK	0
#define EKO	1
#define EJEKO	0

int CXBRK(void) { return(0); }

struct IntuitionBase *IntuitionBase=NULL;
struct RsxLib *RexxSysBase=NULL;
struct Library *UtilityBase, *NiKomBase;
struct Window *NiKwind=NULL;
struct MsgPort *rexxport, *nikomnodeport;
struct User g_preLoginUserData;

char rexxportnamn[20], pubscreen[40],nodid[20], nikomnodeportnamn[15];
int inloggad, g_userDataSlot = -1, getty,gettybps,relogin=FALSE,ypos,xpos,ysize,xsize;
struct User g_preLoginUserData;
struct UnreadTexts g_newUserUnreadTexts;

extern char commandhistory[], inmat[], outbuffer[];
extern int hangupdelay, dtespeed, highbaud, nodnr, nodestate;
extern struct System *Servermem;

struct NodeType *selectNodeType(void);

struct Window *openmywindow(char *screenname) {
	static char titel[32];
	sprintf(titel,"NiKom PreNode, nod #%d SER:",nodnr);
	return(OpenWindowTags(NULL,WA_Left,xpos,
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
										WA_Title,titel,
										WA_ScreenTitle,"NiKom Prenode",
										WA_AutoAdjust,TRUE,
										WA_PubScreenName,screenname,
										TAG_DONE));
}


void cleanup(int errorCode, char *text) {
  if(errorCode > 0) {
    LogEvent(SYSTEM_LOG, ERROR, "PreNode (%d) is exiting with error code %d: %s",
             nodnr, errorCode, text);
  }

  CloseCatalog(g_Catalog);
  shutdownnode(NODSER);
  CloseIO();
  if(NiKwind) {
    CloseWindow(NiKwind);
  }
  if(LocaleBase) {
    CloseLibrary((struct Library *)LocaleBase);
  }
  if(RexxSysBase) {
    CloseLibrary((struct Library *)RexxSysBase);
  }
  if(nikomnodeport) {
    RemPort(nikomnodeport);
    DeleteMsgPort(nikomnodeport);
  }
  if(rexxport) {
    RemPort(rexxport);
    DeleteMsgPort(rexxport);
  }
  if(NiKomBase) {
    CloseLibrary(NiKomBase);
  }
  if(UtilityBase) {
    CloseLibrary(UtilityBase);
  }
  if(IntuitionBase) {
    CloseLibrary((struct Library *)IntuitionBase);
  }
  printf("%s", text);
  exit(errorCode);
}

struct NodeType *selectNodeType(void) {
  struct NodeType *nt = NULL;
  int going, i, isCorrect;

  if(Servermem->cfg->nodetypes[0].nummer == 0) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't login user, no valid node types found.");
    putstring("\n\n\r*** System error ***\n\n\r", -1, 0);
    return NULL;
  }
  if(Servermem->cfg->nodetypes[1].nummer == 0) {
    if((nt = GetNodeType(Servermem->cfg->nodetypes[0].nummer)) == NULL) {
      LogEvent(SYSTEM_LOG, ERROR,
        "Can't login user, the only configured node type (%d) can not be found.",
        Servermem->cfg->nodetypes[0].nummer);
      putstring("\n\n\r*** System error ***\n\n\r", -1, 0);
    }
    return nt;
  }

  if(CURRENT_USER->shell) {
    nt =  GetNodeType(CURRENT_USER->shell);
    if(nt == NULL) {
      CURRENT_USER->shell=0;
    }
  }
  if(CURRENT_USER->shell == 0) {
    putstring("\n\n\rDu har ingen förinställd nodtyp.\n\n\rVälj mellan:\n\n\r",-1,0);
    for(i = 0; i < MAXNODETYPES; i++) {
      if(Servermem->cfg->nodetypes[i].nummer == 0) {
        break;
      }
      sprintf(outbuffer, "%2d: %s\n\r",
              Servermem->cfg->nodetypes[i].nummer, Servermem->cfg->nodetypes[i].desc);
      putstring(outbuffer, -1, 0);
    }
    going = TRUE;
    while(going) {
      putstring("\n\rVal: ",-1,0);
      if(getstring(EKO, 2, NULL)) {
        return NULL;
      }
      if(atoi(inmat) < 1) {
        putstring("\n\rDu måste ange ett positivt heltal.\n\r",-1,0);
      }
      else if(!(nt = GetNodeType(atoi(inmat)))) {
        putstring("\n\rFinns ingen sådan nodtyp.\n\r",-1,0);
      }
      else {
        going=FALSE;
      }
    }
    if(GetYesOrNo("\n\n\r", "Vill du använda denna nodtyp varje gång du loggar in?",
                  NULL, NULL, "Ja", "Nej", "\r\n", TRUE, &isCorrect)) {
      return NULL;
    }
    if(isCorrect) {
      CURRENT_USER->shell = nt->nummer;
    }
  }
  return nt;
}

int main(int argc,char *argv[]) {
  int going=TRUE, forsok=2, car=1, x, connectbps=0, i, newUser, tmpUserId;
  struct NodeType *nt;
  char *tmppscreen,commandstring[100], configname[50] = "NiKom:DatoCfg/SerNode.cfg";
  FILE *fil;
  if(argc>1) for(x=1;x<argc;x++) {
      if(argv[x][0]=='-') {
        if(argv[x][1]=='G') getty=TRUE;
        else if(argv[x][1]=='B') gettybps=atoi(&argv[x][2]);
        else if(argv[x][1]=='C') connectbps = atoi(&argv[x][2]);
      } else strcpy(configname,argv[x]);
    }
  if(!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0)))
    cleanup(EXIT_ERROR,"Kunde inte öppna intuition.library\n");
  if(!(UtilityBase=OpenLibrary("utility.library",37L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna utility.library\n");
  if(!(LocaleBase=(NiKomLocaleType *)OpenLibrary("locale.library",38L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna locale.library\n");
  if(!(NiKomBase=OpenLibrary("nikom.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna nikom.library\n");
  if(!initnode(NODSER)) cleanup(EXIT_ERROR,"Kunde inte registrera noden i Servern\n");
  if(!(nikomnodeport = CreateMsgPort()))
    cleanup(EXIT_ERROR,"Kunde inte skapa NiKomNode-porten");
  sprintf(nikomnodeportnamn,"NiKomNode%d",nodnr);
  nikomnodeport->mp_Node.ln_Name = nikomnodeportnamn;
  nikomnodeport->mp_Node.ln_Pri = 1;
  AddPort(nikomnodeport);
  sprintf(rexxportnamn,"NiKomPreRexx%d",nodnr);
  if(!(rexxport=(struct MsgPort *)CreateMsgPort()))
    cleanup(EXIT_ERROR,"Kunde inte öppna RexxPort\n");
  rexxport->mp_Node.ln_Name=rexxportnamn;
  rexxport->mp_Node.ln_Pri=50;
  AddPort(rexxport);
  if(!(RexxSysBase=(struct RsxLib *)OpenLibrary("rexxsyslib.library",0L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna rexxsyslib.library\n");
  getnodeconfig(configname);
  if(pubscreen[0]=='-') tmppscreen=NULL;
  else tmppscreen=pubscreen;
  if(!(NiKwind=openmywindow(tmppscreen)))
    cleanup(EXIT_ERROR,"Kunde inte öppna fönstret\n");
  if(getty) dtespeed = gettybps;
  else dtespeed = highbaud;
  if(!OpenIO(NiKwind)) cleanup(EXIT_ERROR,"Couldn't setup IO");
  strcpy(Servermem->nodeInfo[nodnr].nodeIdStr, nodid);
  conreqtkn();
  serreqtkn();
  Delay(50);
  for(;;) {
    StopHeartBeat();
    inloggad = -1;
    Servermem->nodeInfo[nodnr].lastActiveTime = time(NULL);
    Servermem->nodeInfo[nodnr].userLoggedIn = -1;
    if(getty) {
      Servermem->nodeInfo[nodnr].connectBps = connectbps;
    } else {
      waitconnect();
    }
    Servermem->nodeInfo[nodnr].lastActiveTime = time(NULL);
    Servermem->nodeInfo[nodnr].userLoggedIn = -2; /* Sätt till <Uppringd> för att även hantera -getty-fallet */
  reloginspec:
    StartHeartBeat(TRUE);
    g_preLoginUserData.flaggor = Servermem->cfg->defaultflags;
    if(!getty) Delay(100);
    g_preLoginUserData.chrset = CHRS_LATIN1;
    sendfile("NiKom:Texter/Login.txt");
    if(Servermem->cfg->ar.preinlogg) sendrexx(Servermem->cfg->ar.preinlogg);
    car=TRUE;
    g_preLoginUserData.chrset = 0;
    memset(commandhistory,0,1000);
    going=1;
    newUser = FALSE;
    while(going && going<=Servermem->cfg->logintries) {
      SendStringNoBrk("\r\nName: ");
      if(getstring(EKO,40,NULL)) { car=FALSE; break; }
      if(!stricmp(inmat,Servermem->cfg->ny)
         && !(Servermem->cfg->cfgflags & NICFG_CLOSEDBBS)) {
        tmpUserId = RegisterNewUser(&g_preLoginUserData);
        if(tmpUserId == -2) {
          goto panik;
        }
        car = tmpUserId != -1;
        newUser = TRUE;
        going=FALSE;
      } else if((tmpUserId = parsenamn(inmat)) >= 0) {
        if(!ReadUser(tmpUserId, &g_preLoginUserData)) {
          goto panik;
        }
        // TODO: Extract password loop. Should be identical to in NiKomCon.c
        forsok=2;
        while(forsok) {
          SendStringNoBrk("\r\nPassword: ");
          if(g_preLoginUserData.flaggor & STAREKOFLAG) {
            if(getstring(STAREKO,15,NULL)) { car=FALSE; break; }
          } else {
            if(getstring(EJEKO,15,NULL)) { car=FALSE; break; }
          }
          if(CheckPassword(inmat, g_preLoginUserData.losen)) {
            forsok=FALSE;
            going=FALSE;
          } else {
            forsok--;
          }
        }
        if(going && (Servermem->cfg->logmask & LOG_FAILINLOGG)) {
          LogEvent(USAGE_LOG, WARN, "Nod %d, %s angivet som namn, fel lösen.",
                   nodnr, getusername(inloggad));
        }
        if(going) {
          going++;
        }
      } else if(tmpUserId == -1) {
        SendStringNoBrk("\r\nNo such user\r\n");
      }
    }
    if(!car) { //TODO: Replace with ConnectionList()?
      if(getty) cleanup(EXIT_OK,"");
      disconnect();
      continue;
    }
    if(going) {
      putstring("\n\n\rTyvärr. Du har försökt maximalt antal gånger att logga in. Kopplar ned.\n\r",-1,0);
      goto panik;      /* Urrk vad fult. :-) */
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
    if((nt = selectNodeType()) == NULL) {
      goto panik;
    }
    StopHeartBeat();
    abortserial();
    
    sprintf(commandstring,"%s -N%d -B%d %s",nt->path,nodnr,dtespeed,configname);
    CloseConsole();
    CloseWindow(NiKwind);
    NiKwind = NULL;
    RemPort(nikomnodeport);
    
    i = 0;
    if(Servermem->nodeInfo[nodnr].connectBps > 0)
      {
        while(Servermem->info.bps[i] != Servermem->nodeInfo[nodnr].connectBps && Servermem->info.bps[i] > 0 && i<49)
          i++;
        
        if(i<49)
          {
            if(Servermem->info.bps[i] == Servermem->nodeInfo[nodnr].connectBps)
              Servermem->info.antbps[i]++;
            else
              {
                Servermem->info.bps[i] = Servermem->nodeInfo[nodnr].connectBps;
                Servermem->info.antbps[i]++;
              }
          }
        
        if(!(fil = fopen("NiKom:datocfg/sysinfo.dat","w")))
          {
            /* putstring("Kunde inte spara nya sysinfo.dat..\n",-1,0); */
          }
        
        if(fwrite((void *)&Servermem->info,sizeof(Servermem->info),1,fil) != 1)
          {
            /* putstring("Kunde inte skriva till nya sysinfo.dat....\n",-1,0); */
          }
        fclose(fil);
      }
    
    nodestate = SystemTags(commandstring, SYS_UserShell, TRUE, TAG_DONE);

    ReleaseUserDataSlot(nodnr);
    inloggad = -1;

    AddPort(nikomnodeport);
    if(!getty || (nodestate & NIKSTATE_RELOGIN)) {
      if(!(NiKwind = openmywindow(tmppscreen))) cleanup(EXIT_ERROR,"Kunde inte öppna fönstret\n");
      OpenConsole(NiKwind);
    }
    serreqtkn();
    if(nodestate & NIKSTATE_RELOGIN) goto reloginspec;
  panik:
    Delay(hangupdelay);
    if(getty) cleanup(EXIT_OK,"");
    disconnect();
  }
  return 0;
}

void HandleHeartBeat(void) {
  CheckInactivity();
}
