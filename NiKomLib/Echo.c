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
#include <math.h>

#include <stdio.h>

#include "NiKomBase.h"
#include "NiKomLib.h"
#include "Funcs.h"
#include "Logging.h"
#include "FidoUtils.h"
#include "BTree.h"

int linkComments(struct ExtMote *conf, struct NiKomBase *NiKomBase) {
  struct BTree *msgidTree, *commentsTree;
  struct FidoText *ft;
  struct FidoLine *fl;
  int i, j, commentedText, forwardComments[FIDO_FORWARD_COMMENTS], fromText, nikomNext;
  char treepath[100], msgPath[100], msgid[FIDO_MSGID_KEYLEN];
  struct TagItem ti = { TAG_DONE };

  ObtainSemaphore(&conf->fidoCommentsSemaphore);
  sprintf(treepath, "NiKom:FidoComments/%s_id", conf->diskConf.tagnamn);
  if((msgidTree = BTreeOpen(treepath)) == NULL) {
    ReleaseSemaphore(&conf->fidoCommentsSemaphore);
    return 5;
  }

  nikomNext = GetNextMsgNum(conf->diskConf.dir);
  fromText = max(nikomNext, conf->diskConf.lowtext);;
  if(fromText > conf->diskConf.texter) {
    BTreeClose(msgidTree);
    ReleaseSemaphore(&conf->fidoCommentsSemaphore);
    return 0;
  }
  LogEvent(NiKomBase->Servermem, FIDO_LOG, INFO, "Linking comments in %s, text %d to %d.",
           conf->diskConf.tagnamn, fromText, conf->diskConf.texter);
  
  for(i = fromText; i <= conf->diskConf.texter; i++) {
    if((ft = LIBReadFidoText(MakeMsgFilePath(conf->diskConf.dir,
                                             i - conf->diskConf.renumber_offset,
                                             msgPath), &ti, NiKomBase)) == NULL) {
      LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
               "FidoInitComments:  Could not open text file %s.", msgPath);
      continue;
    }
    ITER_EL(fl, ft->text, line_node, struct FidoLine *) {
      if(fl->text[0] != 1) {
        break;
      }
      if(strncmp(&fl->text[1], "MSGID:", 6) == 0) {
        strncpy(msgid, &fl->text[8], FIDO_MSGID_KEYLEN);
        if(!BTreeInsert(msgidTree, msgid, &i)) {
          LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
                   "FidoInitComments:  Error Inserting '%s' -> %d. BTree for %s is possibly corrupt.",
                   msgid, i, conf->diskConf.tagnamn);
          LIBFreeFidoText(ft);
          BTreeClose(msgidTree);
          ReleaseSemaphore(&conf->fidoCommentsSemaphore);
          return 6;
        }
        break;
      }
    }
    LIBFreeFidoText(ft);
  }

  sprintf(treepath, "NiKom:FidoComments/%s_c", conf->diskConf.tagnamn);
  if((commentsTree = BTreeOpen(treepath)) == NULL) {
    BTreeClose(msgidTree);
    ReleaseSemaphore(&conf->fidoCommentsSemaphore);
    return 5;
  }

  for(i = fromText; i <= conf->diskConf.texter; i++) {
    if((ft = LIBReadFidoText(MakeMsgFilePath(conf->diskConf.dir,
                                             i - conf->diskConf.renumber_offset,
                                             msgPath), &ti, NiKomBase)) == NULL) {
      LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
               "FidoInitComments:  Could not open text file %s.", msgPath);
      continue;
    }
    ITER_EL(fl, ft->text, line_node, struct FidoLine *) {
      if(fl->text[0] != 1) {
        break;
      }
      if(strncmp(&fl->text[1], "REPLY:", 6) != 0) {
        continue;
      }
      strncpy(msgid, &fl->text[8], FIDO_MSGID_KEYLEN);
      if(BTreeGet(msgidTree, msgid, &commentedText)) {
        if(BTreeGet(commentsTree, &commentedText, forwardComments)) {
          for(j = 0; j < FIDO_FORWARD_COMMENTS && forwardComments[j] != 0; j++);
          if(j == FIDO_FORWARD_COMMENTS) {
            // No room for more comments
            break;
          }
          forwardComments[j] = i;
        } else {
          memset(forwardComments, 0, FIDO_FORWARD_COMMENTS * sizeof(int));
          forwardComments[0] = i;
        }
        if(!BTreeInsert(commentsTree, &commentedText, forwardComments)) {
          LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
                   "FidoInitComments:  Error Inserting forward comments for text %d.  BTree for %s is possibly corrupt.",
                   commentedText, conf->diskConf.tagnamn);
          LIBFreeFidoText(ft);
          BTreeClose(commentsTree);
          BTreeClose(msgidTree);
          ReleaseSemaphore(&conf->fidoCommentsSemaphore);
          return 6;
        }
        break;
      }
    }
    LIBFreeFidoText(ft);
  }
  BTreeClose(commentsTree);
  BTreeClose(msgidTree);
  ReleaseSemaphore(&conf->fidoCommentsSemaphore);
  SetNextMsgNum(conf->diskConf.dir, conf->diskConf.texter + 1);
  return 0;
}

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
  lock = Lock(newmotpek->dir, ACCESS_READ);
  if(lock) {
    buffer = AllocMem(2048, MEMF_CLEAR);
    if(buffer) {
      eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
      if(eac) {
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
  linkComments((struct ExtMote *)newmotpek, NiKomBase);
}

void __saveds AASM LIBUpdateFidoConf(register __a0 struct Mote *motpek AREG(a0),register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  int tmpmin, tmpmax, oldmin, oldmax;
  BPTR lock;
  char fullpath[100];

  if(!NiKomBase->Servermem) return;

  oldmax = motpek->texter;
  tmpmax = motpek->texter + 1 - motpek->renumber_offset;
  for(;;) {
    lock = Lock(MakeMsgFilePath(motpek->dir, tmpmax, fullpath), ACCESS_READ);
    if(lock) {
      UnLock(lock);
      tmpmax++;
    } else break;
  }
  motpek->texter=tmpmax - 1 + motpek->renumber_offset;

  oldmin = motpek->lowtext;
  tmpmin=motpek->lowtext - motpek->renumber_offset;
  while(tmpmin<=motpek->texter) {
    lock = Lock(MakeMsgFilePath(motpek->dir, tmpmin, fullpath), ACCESS_READ);
    if(lock) {
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
  linkComments((struct ExtMote *)motpek, NiKomBase);
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
  char oldpath[100], newpath[100];
  
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
    MakeMsgFilePath(confToRenum->dir, i, oldpath);
    MakeMsgFilePath(confToRenum->dir, i - diff, newpath);
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

/*
 * 1 - No conf with that confId.
 * 2 - The conf is not a FidoNet conf.
 * 3 - Comments are already initiated for this conf.
 * 4 - Couldn't create data structure on disk.
 * 5 - Couldn't open the newly created data structure.
 * 6 - Error inserting a key, init aborted. See log.
 * -1 - No Servermem
 */
int __saveds AASM LIBFidoInitComments(register __d0 int confId AREG(d0),
                                     register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  struct Mote *conf;
  struct BTree *msgidTree;
  char treepath[100];
  
  if(!NiKomBase->Servermem) {
    return -1;
  }

  conf = LIBGetConfPoint(confId, NiKomBase);
  if(conf == NULL) {
    return 1;
  }

  if(conf->type != MOTE_FIDO) {
    return 2;
  }

  sprintf(treepath, "NiKom:FidoComments/%s_id", conf->tagnamn);
  if((msgidTree = BTreeOpen(treepath)) != NULL) {
    BTreeClose(msgidTree);
    return 3;
  }

  if(!BTreeCreate(treepath, 15, FIDO_MSGID_KEYLEN, sizeof(int))) {
    return 4;
  }

  sprintf(treepath, "NiKom:FidoComments/%s_c", conf->tagnamn);
  if(!BTreeCreate(treepath, 15, sizeof(int), FIDO_FORWARD_COMMENTS * sizeof(int))) {
    return 4;
  }

  return linkComments((struct ExtMote *)conf, NiKomBase);
}
