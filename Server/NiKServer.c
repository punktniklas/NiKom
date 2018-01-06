#include <intuition/intuition.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <rexx/storage.h>
#include "NiKomStr.h"
#include "ServerFuncs.h"
#include "NiKomLib.h"
#include "Startup.h"
#include "Shutdown.h"
#include "Config.h"

#include "UserNotificationHooks.h"

#define EXIT_OK	     0

void sparatext(struct NiKMess *);
int CXBRK(void) { return(0); }

extern char windowTitle[];

void DisplayInternalError(void) {}

void sparatext(struct NiKMess *message) {
	BPTR fh;
	struct MinList *listpek;
	struct EditLine *elpek;
	struct Header *headpek=(struct Header *)message->data;
	long position;
	int cnt,len;
	char filnamn[40];
	listpek=(struct MinList *)headpek->textoffset;
	headpek->nummer = Servermem->info.hightext + 1;
        if(headpek->root_text == 0) {
          headpek->root_text = headpek->nummer;
        }
	sprintf(filnamn,"NiKom:Moten/Text%ld.dat",headpek->nummer/512);
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
	sprintf(filnamn,"NiKom:Moten/Head%ld.dat",headpek->nummer/512);
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
        SetConferenceForText(Servermem->info.hightext, headpek->mote, TRUE);

	if(message->nod >=0 ) {
          ChangeUnreadTextStatus(Servermem->info.hightext, 0,
            &Servermem->unreadTexts[message->nod]);
        }
	message->data=Servermem->info.hightext;
}

void purgeOldTexts(int numberOfTexts) {
  long oldlowtext = Servermem->info.lowtext;
  int i;
  char filename[40];

  if(!DeleteConferenceTexts(numberOfTexts)) {
    printf("Failed deleting texts from the conference texts array\n");
    return;
  }
  
  for(i=0; i < numberOfTexts; i += 512) {
    sprintf(filename, "NiKom:Moten/Head%ld.dat", (oldlowtext + i) / 512);
    DeleteFile(filename);
    sprintf(filename, "NiKom:Moten/Text%ld.dat", (oldlowtext + i) / 512);
    DeleteFile(filename);
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

int main(void) {
  int Going=TRUE,noder=0,x=0;
  long windmask, nikPortMask, signals, rexxmask, nodereplymask;
  struct NiKMess *MyNiKMess, *dummymess;
  struct IntuiMessage *myintmess;
  struct RexxMsg *rexxmess;

  startupNiKom();

  printf("NiKServer up and running.\n");

  windmask = 1L << NiKWindow->UserPort->mp_SigBit;
  nikPortMask = 1L << NiKPort->mp_SigBit;
  rexxmask = 1L << rexxport->mp_SigBit;
  nodereplymask = 1l << nodereplyport->mp_SigBit;

  while(Going) {
    signals=Wait(windmask | nikPortMask | rexxmask | nodereplymask | SIGBREAKF_CTRL_C);
    if(signals & windmask) {
      myintmess=(struct IntuiMessage *)GetMsg(NiKWindow->UserPort);
      ReplyMsg((struct Message *)myintmess);
      if(!noder) cleanup(EXIT_OK,"");
    }
    if(signals & SIGBREAKF_CTRL_C) if(!noder) cleanup(EXIT_OK,"");
    if(signals & nikPortMask) {
      while((MyNiKMess=(struct NiKMess *)GetMsg(NiKPort))) {
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
          sprintf(windowTitle, "NiKom %s,  %d noder aktiva", NiKomReleaseStr, noder);
          SetWindowTitles(NiKWindow, windowTitle, (UBYTE *)-1L);
          break;
          
        case NODSLUTAR :
          noder--;
          Servermem->nodtyp[MyNiKMess->nod]=0;
          sprintf(windowTitle, "NiKom %s,  %d noder aktiva", NiKomReleaseStr, noder);
          SetWindowTitles(NiKWindow, windowTitle, (UBYTE *)-1L);
          break;
          
        case SPARATEXTEN :
          sparatext(MyNiKMess);
          break;
        case FORBID :
          ReplyMsg((struct Message *)MyNiKMess);
          WaitPort(permitport);
          dummymess=(struct NiKMess *)GetMsg(permitport);
          break;
        case RADERATEXTER :
          purgeOldTexts(MyNiKMess->data);
          break;
        case READCFG :
          MyNiKMess->data = ReReadConfigs();
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
      while((rexxmess=(struct RexxMsg *)GetMsg(rexxport))) {
        handlerexx(rexxmess);
        ReplyMsg((struct Message *)rexxmess);
      }
    }
    if(signals & nodereplymask) {
      while((MyNiKMess = (struct NiKMess *) GetMsg(nodereplyport)))
        FreeMem(MyNiKMess,sizeof(struct NiKMess));
    }
  }
  return 0;
}
