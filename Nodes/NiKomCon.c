#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Terminal.h"
#include "NewUser.h"
#include "Languages.h"

#define EXIT_ERROR	10
#define EXIT_OK	0
#define coneka	eka
#define conputtekn puttekn

int CXBRK(void) { return(0); }

extern long logintime;
extern int area2, nodnr, nodestate;
extern struct MinList aliaslist,edit_list;
extern char outbuffer[], nodid[], inmat[];
extern struct System *Servermem;

struct IntuitionBase *IntuitionBase=NULL;
struct RsxLib *RexxSysBase;
struct Library *UtilityBase, *LocaleBase, *NiKomBase;
struct Window *NiKwind=NULL;
struct MsgPort *rexxport, *nikomnodeport;

char rexxportnamn[15], pubscreen[40], nikomnodeportnamn[15];
int inloggad, ypos, xpos, ysize, xsize;
extern char commandhistory[];

void freealiasmem(void) {
	struct Alias *pekare;
	while(pekare=(struct Alias *)RemHead((struct List *)&aliaslist))
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
	if(LocaleBase) CloseLibrary(LocaleBase);
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	printf("%s",fel);
	exit(kod);
}

void main(int argc, char **argv) {
  int going = TRUE, forsok = 2,car = 1, tmp, ch;
  long tid;
  char tellstr[100],*tmppscreen, titel[80];

  NewList((struct List *)&aliaslist);
  NewList((struct List *)&edit_list);
  if(!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0)))
    cleanup(EXIT_ERROR,"Kunde inte öppna intuition.library\n");
  if(!(UtilityBase=OpenLibrary("utility.library",37L)))
    cleanup(EXIT_ERROR,"Kunde inte öppna utility.library\n");
  if(!(LocaleBase=OpenLibrary("locale.library",38L)))
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
  strcpy(Servermem->nodid[nodnr],nodid);
  sprintf(titel,"Nod #%d CON: <Ingen inloggad>",nodnr);
  SetWindowTitles(NiKwind,titel,(UBYTE *)-1L);
  Servermem->connectbps[nodnr] = -1;
  conreqtkn();
  do {
    Servermem->idletime[nodnr] = time(NULL);
    memset(commandhistory,0,1024);
    Servermem->inne[nodnr].rader=0;
    sendfile("NiKom:Texter/Inlogg.txt");
    if(Servermem->cfg.ar.preinlogg) sendautorexx(Servermem->cfg.ar.preinlogg);
    going=TRUE;
    while(going) {
      Servermem->idletime[nodnr] = time(NULL);
      puttekn("\r\nNamn: ",-1);
      getstring(EKO,40,NULL);
      if(!stricmp(inmat,Servermem->cfg.ny)) {
        tmp = RegisterNewUser();
        if(tmp == 2) {
          goto panik;
        }
        car = tmp ? 0 : 1;
        going=FALSE;
      }
      else if((inloggad=parsenamn(inmat))>=0) {
        if(readuser(inloggad,&Servermem->inne[nodnr])) {
          puttekn("Error reading user data.\r\n", -1);
          goto panik;
        }
        // TODO: Extract password loop. Should be identical to in PreNode/Ser.c
        forsok=2;
        while(forsok) {
          puttekn("\r\nLösen: ",-1);
          if(Servermem->inne[nodnr].flaggor & STAREKOFLAG)
            getstring(STAREKO,15,NULL);
          else
            getstring(EJEKO,15,NULL);
          if(CheckPassword(inmat, Servermem->inne[nodnr].losen)) {
            forsok=FALSE;
            going=FALSE;
          } else {
            forsok--;
          }
        }
      } else if(inloggad==-1) puttekn("\r\nHittar ej namnet\r\n",-1);
    }
    sprintf(titel,"Nod #%d CON: %s #%d",nodnr,Servermem->inne[nodnr].namn,inloggad);
    SetWindowTitles(NiKwind,titel,(UBYTE *)-1L);
    if(!ReadUnreadTexts(&Servermem->unreadTexts[nodnr], inloggad)) {
      puttekn("Error reading unread text info.\r\n", -1);
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't read unread text info for user %d", inloggad);
      goto panik;
    }
    Servermem->inloggad[nodnr]=inloggad;
    Servermem->idletime[nodnr] = time(NULL);
    if(getft("NiKom:Texter/Bulletin.txt")>Servermem->inne[nodnr].senast_in) sendfile("NiKom:Texter/Bulletin.txt");

    connection();

    if(Servermem->cfg.logmask & LOG_UTLOGG) {
      LogEvent(USAGE_LOG, INFO, "%s loggar ut från nod %d",
               getusername(inloggad), nodnr);
    }
    if(Servermem->say[nodnr]) displaysay();
    if(Servermem->cfg.ar.utlogg) sendautorexx(Servermem->cfg.ar.utlogg);
    sendfile("NiKom:Texter/Utlogg.txt");
    sprintf(titel,"Nod #%d CON: <Ingen inloggad>",nodnr);
    SetWindowTitles(NiKwind,titel,(UBYTE *)-1L);
    Servermem->inloggad[nodnr]=-1;
    
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
    freealiasmem();
    sprintf(tellstr,"loggade just ut från nod %d",nodnr);
    tellallnodes(tellstr);
  panik:
    nodestate = 0;
    puttekn("\r\n\nEn inloggning till? (J/n)",-1);
  } while((ch = GetChar()) != 'n' && ch != 'N');
  cleanup(EXIT_OK,"");
}
