#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "Startup.h"
#include "Config.h"

#include "Shutdown.h"

#define OK	     0

// TODO: Get rid of these prototypes
void writeinfo(void);

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

void freegroupmem(void) {
	struct UserGroup *pekare;
	while(Servermem->grupp_list.mlh_Head->mln_Succ) {
		pekare=(struct UserGroup *)Servermem->grupp_list.mlh_Head;
		RemHead((struct List *)&Servermem->grupp_list);
		FreeMem(pekare,sizeof(struct UserGroup));
	}
}

void cleanup(int exitCode,char *errorMsg) {
  if(rexxport) {
    RegisterARexxFunctionHost(FALSE);
    DeletePort(rexxport);
  }
  if(NiKomBase) {
    InitServermem(NULL); /* Stänger av nikom.library */
  }
  if(Servermem) {
    if(exitCode == OK) {
      writeinfo();
    }
    freegroupmem();
    freefilemem();
    FreeCommandMem();
    freemotmem();
    freeshortusermem();
    if(Servermem->confTexts.texts != NULL) {
      FreeMem(Servermem->confTexts.texts,
              Servermem->confTexts.arraySize * sizeof(short));
    }
    FreeMem(Servermem,sizeof(struct System));
  }
  if(NiKWindow) CloseWindow(NiKWindow);
  if(nodereplyport) DeleteMsgPort(nodereplyport);
  if(permitport) DeletePort(permitport);
  if(NiKomBase) CloseLibrary(NiKomBase);
  if(UtilityBase) CloseLibrary(UtilityBase);
  if(RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
  if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
  if(NiKPort) DeletePort(NiKPort);

  if(exitCode != OK) {
    printf("\n\nShutting down NiKServer with error code %d: %s\n",
           exitCode, errorMsg);
  }
  exit(exitCode);
}
