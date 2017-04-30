#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "NiKomBase.h"
#include "NiKomLib.h"
#include "Funcs.h"
#include "Logging.h"

void __saveds AASM LIBReScanFidoConf(register __a0 struct Mote *motpek AREG(a0),
                                      register __d0 int motnr AREG(d0),
                                      register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  struct ExAllData *ead;
  struct ExAllControl *eac;
  void *buffer;
  BPTR lock;
  int min=0,max=0,tmp,more;
  struct Mote *newmotpek;
  
  if(!NiKomBase->Servermem) return;

  if(motpek) newmotpek = motpek;
  else {
    newmotpek=LIBGetConfPoint(motnr,NiKomBase);
    if(!newmotpek) return;
  }
  if(lock = Lock(newmotpek->dir,ACCESS_READ)) {
    if(buffer = AllocMem(2048,MEMF_CLEAR)) {
      if(eac = AllocDosObject(DOS_EXALLCONTROL,NULL)) {
        eac->eac_LastKey = 0;
        eac->eac_MatchString = NULL;
        eac->eac_MatchFunc = NULL;
        do {
          more=ExAll(lock,buffer,2048,ED_NAME,eac);
          if(eac->eac_Entries == 0) continue;
          ead = (struct ExAllData *)buffer;
          while(ead) {
            if(ead->ed_Name[0] >= '0' && ead->ed_Name[0] <= '9') {
              tmp=atoi(ead->ed_Name);
              if(tmp!=1) { /* Vi vill inte ha HWM */
                if(!min || tmp<min) {
                  min=tmp;
                }
                if(tmp>max) {
                  max=tmp;
                }
              }
            }
            ead=ead->ed_Next;
          }
        } while(more);
        FreeDosObject(DOS_EXALLCONTROL,eac);
      }
      FreeMem(buffer,2048);
    }
    UnLock(lock);
  }
  if(min<2) min=2;
  if(!max) max=1;
  newmotpek->lowtext = min + newmotpek->renumber_offset;
  newmotpek->texter = max + newmotpek->renumber_offset;
  LogEvent(NiKomBase->Servermem, FIDO_LOG, INFO,
           "Full scan of area %s. Low %d (%d.msg), high %d (%d.msg)",
           newmotpek->tagnamn, newmotpek->lowtext, min, newmotpek->texter, max);
}

void __saveds AASM LIBUpdateFidoConf(register __a0 struct Mote *motpek AREG(a0),register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  int tmpmin, tmpmax, oldmin, oldmax;
  BPTR lock;
  char filnamn[20],fullpath[100];

  if(!NiKomBase->Servermem) return;

  oldmax = motpek->texter;
  tmpmax = motpek->texter + 1 - motpek->renumber_offset;
  for(;;) {
    strcpy(fullpath,motpek->dir);
    sprintf(filnamn,"%d.msg",tmpmax);
    AddPart(fullpath,filnamn,99);
    if(lock = Lock(fullpath,ACCESS_READ)) {
      UnLock(lock);
      tmpmax++;
    } else break;
  }
  motpek->texter=tmpmax - 1 + motpek->renumber_offset;

  oldmin = motpek->lowtext;
  tmpmin=motpek->lowtext - motpek->renumber_offset;
  while(tmpmin<=motpek->texter) {
    strcpy(fullpath,motpek->dir);
    sprintf(filnamn,"%d.msg",tmpmin);
    AddPart(fullpath,filnamn,99);
    if(lock=Lock(fullpath,ACCESS_READ)) {
      UnLock(lock);
      break;
    } else tmpmin++;
  }
  motpek->lowtext = tmpmin + motpek->renumber_offset;
  LogEvent(NiKomBase->Servermem, FIDO_LOG, INFO,
           "Updated area %s. Low %d (%d.msg) -> %d (%d.msg), high %d (%d.msg) -> %d (%d.msg)"
           " (%d new messages)",
           motpek->tagnamn,
           oldmin, oldmin - motpek->renumber_offset,
           motpek->lowtext, motpek->lowtext - motpek->renumber_offset,
           oldmax, oldmax - motpek->renumber_offset,
           motpek->texter, motpek->texter - motpek->renumber_offset,
           motpek->texter - oldmax);
}

void __saveds AASM LIBUpdateAllFidoConf(register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return;

	motpek = (struct Mote *)NiKomBase->Servermem->mot_list.mlh_Head;
	for(;motpek->mot_node.mln_Succ;motpek = (struct Mote *)motpek->mot_node.mln_Succ) {
		if(motpek->type != MOTE_FIDO) continue;
		LIBUpdateFidoConf(motpek,NiKomBase);
	}
}

void __saveds AASM LIBReScanAllFidoConf(register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return;

	motpek = (struct Mote *)NiKomBase->Servermem->mot_list.mlh_Head;
	for(;motpek->mot_node.mln_Succ;motpek = (struct Mote *)motpek->mot_node.mln_Succ) {
		if(motpek->type != MOTE_FIDO) continue;
		LIBReScanFidoConf(motpek,0,NiKomBase);
	}
}

/* Wow! Jag dokumentarer en funktion i källkoden!
*
*  Namn: ReNumberConf
*  Parametrar: a0 - En mötespekare
*              d0 - Ett mötesnummer
*              d1 - Ett värde på det nya numret på lägsta texten
*
*  Returvärden: 0 - Allt ok
*               1 - Finns inget möte med angivet mötesnummer
*               2 - Det nya textnumret är inte lägre än det gamla
*               3 - Det angivna mötet är inget Fido-möte.
*               4 - Kunde inte läsa HWM för mötet.
*               5 - Kunde inte skriva HWM för mötet.
*              -1 - Servermem finns inte.
*
*  Beskrivning: Funktionen numrerar om texterna i ett Fido-möte fysiskt på
*               hårddisken.
*/

int __saveds AASM LIBReNumberConf(register __a0 struct Mote *conf AREG(a0),register __d0 int confId AREG(d0),
                                   register __d1 int newlowest AREG(d1),
                                   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  struct Mote *confToRenum;
  int i, reallowest, realhighest, diff, hwm;
  char oldpath[100], newpath[100], filename[20];
  
  if(!NiKomBase->Servermem) {
    return -1;
  }
  
  if(conf) {
    confToRenum = conf;
  } else {
    confToRenum = LIBGetConfPoint(confId, NiKomBase);
    if(!confToRenum) {
      return 1;
    }
  }
  if(confToRenum->type != MOTE_FIDO) {
    return 3;
  }
  reallowest = confToRenum->lowtext - confToRenum->renumber_offset;
  realhighest = confToRenum->texter - confToRenum->renumber_offset;
  if(newlowest >= reallowest) {
    return 2;
  }
  diff = reallowest - newlowest;
  for(i = reallowest; i <= realhighest; i++) {
    sprintf(filename, "%d.msg", i);
    strcpy(oldpath, confToRenum->dir);
    AddPart(oldpath, filename, 99);
    sprintf(filename, "%d.msg", i - diff);
    strcpy(newpath, confToRenum->dir);
    AddPart(newpath, filename, 99);
    Rename(oldpath, newpath);
  }
  confToRenum->renumber_offset += diff;
  LIBWriteConf(confToRenum, NiKomBase);
  if(!(hwm = gethwm(confToRenum->dir, NIK_LITTLE_ENDIAN))) {
    return 4;
  }
  if(!sethwm(confToRenum->dir, hwm - diff, NIK_LITTLE_ENDIAN)) {
    return 5;
  }
  LogEvent(NiKomBase->Servermem, FIDO_LOG, INFO,
           "Renumbered area %s. New lowest: %d.msg (%d lower than before)",
           confToRenum->tagnamn, newlowest, diff);
  return 0;
}


/* Det ska läggas till lite semafortjofräs här innan man kan anse den
*  som riktigt klar. Just nu vill jag dock bara få ReNumberConf() att
*  fungera */

/* Namn: WriteConf
*  Parametrar: a0 - En mötespekare
*
*  Returvärden: 0 - Allt OK.
*               1 - Möten.dat kunde inte öppnas.
*               2 - Rätt ställe i filen kunde inteuppsökas
*               3 - Fel under skrivandet av mötet.
*              -1 - Servermem finns inte.
*
*  Beskrivning: Skriver ner mötet på disk.
*/

int __saveds AASM LIBWriteConf(register __a0 struct Mote *motpek AREG(a0), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
	BPTR fh;

	if(!NiKomBase->Servermem) return(-1);

	if(!(fh=Open("NiKom:DatoCfg/Möten.dat",MODE_OLDFILE))) {
		return(1);
	}
	if(Seek(fh,motpek->nummer*sizeof(struct Mote),OFFSET_BEGINNING)==-1) {
		Close(fh);
		return(2);
	}
	if(Write(fh,(void *)motpek,sizeof(struct Mote))==-1) {
		Close(fh);
		return(3);
	}
	Close(fh);
	return(0);
}
