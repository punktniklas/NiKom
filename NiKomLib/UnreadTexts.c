#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"

#include "UnreadTexts.h"
#include "Funcs.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* ******* Local functions ******** */

void shiftBitmap(struct UnreadTexts *unreadTexts, int textNumberToAccomodate);
void trimLowestPossibleUnreadText(struct UnreadTexts *unreadTexts, int conf,
  struct System *Servermem);

/* ******* Library functions ******** */

void __saveds __asm LIBChangeUnreadTextStatus(
   register __d0 int textNumber,
   register __d1 int markAsUnread,
   register __a0 struct UnreadTexts *unreadTexts,
   register __a6 struct NiKomBase *NiKomBase) {

   if(textNumber < unreadTexts->bitmapStartText) {
      return;
   }
   if(textNumber >= (unreadTexts->bitmapStartText + UNREADTEXTS_BITMAPSIZE)) {
     if(markAsUnread) {
       return;
     }
     shiftBitmap(unreadTexts, textNumber);
   }

   if(markAsUnread) {
      BAMSET(unreadTexts->bitmap, textNumber % UNREADTEXTS_BITMAPSIZE);
   } else {
      BAMCLEAR(unreadTexts->bitmap, textNumber % UNREADTEXTS_BITMAPSIZE);
   }
}

int __saveds __asm LIBIsTextUnread(
  register __d0 int textNumber,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase) {

  if(textNumber < unreadTexts->bitmapStartText) {
    return 0;
  }
  if(textNumber >= (unreadTexts->bitmapStartText + UNREADTEXTS_BITMAPSIZE)) {
    return 1;
  }
  return BAMTEST(unreadTexts->bitmap, textNumber % UNREADTEXTS_BITMAPSIZE);
}

void __saveds __asm LIBInitUnreadTexts(
   register __a0 struct UnreadTexts *unreadTexts,
   register __a6 struct NiKomBase *NiKomBase) {
  unreadTexts->bitmapStartText = NiKomBase->Servermem->info.lowtext;
  memset(unreadTexts->bitmap, 0xff, UNREADTEXTS_BITMAPSIZE/8);
  memset(unreadTexts->lowestPossibleUnreadText, 0, MAXMOTE * sizeof(long));
}

/*
 * Doc: If search start is lower than the bitmap it will start from the bitmap.
 * So it's ok to start searching from 0, regardless of how many texts the
 * system has.
 */
int __saveds __asm LIBFindNextUnreadText(
  register __d0 int searchStart,
  register __d1 int conf,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase) {

  int i;

  trimLowestPossibleUnreadText(unreadTexts, conf, NiKomBase->Servermem);
  if(searchStart < unreadTexts->lowestPossibleUnreadText[conf]) {
    searchStart = unreadTexts->lowestPossibleUnreadText[conf];
  }

  for(i = searchStart; i <= NiKomBase->Servermem->info.hightext; i++) {
    if(conf != NiKomBase->Servermem->texts[i%MAXTEXTS]) {
      continue;
    }
    if(i >= (unreadTexts->bitmapStartText + UNREADTEXTS_BITMAPSIZE)) {
      return i;
    }
    if(BAMTEST(unreadTexts->bitmap, i % UNREADTEXTS_BITMAPSIZE)) {
      return i;
    }
  }
  return -1;
}

int __saveds __asm LIBCountUnreadTexts(
  register __d0 int conf,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase) {

  int i, cnt = 0;

  trimLowestPossibleUnreadText(unreadTexts, conf, NiKomBase->Servermem);

  for(i = unreadTexts->lowestPossibleUnreadText[conf];
      i <= NiKomBase->Servermem->info.hightext; i++) {
    if(NiKomBase->Servermem->texts[i % MAXTEXTS] == conf
       && (i >= (unreadTexts->bitmapStartText + UNREADTEXTS_BITMAPSIZE)
           || BAMTEST(unreadTexts->bitmap, i % UNREADTEXTS_BITMAPSIZE))) {
         cnt++;
    }
  }  
  return cnt;
}

void __saveds __asm LIBSetUnreadTexts(
  register __d0 int conf,
  register __d1 int amount,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase) {

  int i, lowestUnreadText = NiKomBase->Servermem->info.hightext + 1;

  for(i = NiKomBase->Servermem->info.hightext;
      (i >= unreadTexts->bitmapStartText)
        && (i >= NiKomBase->Servermem->info.lowtext);
      i--) {
    if(NiKomBase->Servermem->texts[i % MAXTEXTS] == conf) {
      if(amount) {
        ChangeUnreadTextStatus(i, 1, unreadTexts);
        amount--;
        lowestUnreadText = i;
      } else {
        ChangeUnreadTextStatus(i, 0, unreadTexts);
      }
    }
  }
  unreadTexts->lowestPossibleUnreadText[conf] = lowestUnreadText;
}

struct ConfAndText {
  long conf, text;
};

int __saveds __asm LIBReadUnreadTexts(
  register __a0 struct UnreadTexts *unreadTexts,
  register __d0 int userId,
  register __a6 struct NiKomBase *NiKomBase) {
  BPTR file;
  char filepath[41];
  int readRes;
  struct ConfAndText cat;

  ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);

  MakeUserFilePath(filepath, userId, "Bitmap0");
  if(!(file = Open(filepath, MODE_OLDFILE))) {
    ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);
    return 0;
  }
  if(Read(file, unreadTexts->bitmap, UNREADTEXTS_BITMAPSIZE/8)
     != UNREADTEXTS_BITMAPSIZE/8) {
    Close(file);
    ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);
    return 0;
  }
  unreadTexts->bitmapStartText = NiKomBase->Servermem->info.lowtext;

  memset(unreadTexts->lowestPossibleUnreadText, 0, MAXMOTE * sizeof(long));
  while((readRes = Read(file,&cat,sizeof(struct ConfAndText)))
        == sizeof(struct ConfAndText)) {
    unreadTexts->lowestPossibleUnreadText[cat.conf] = cat.text;
  }

  Close(file);
  ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);
  return readRes == 0;
}

int __saveds __asm LIBWriteUnreadTexts(
  register __a0 struct UnreadTexts *unreadTexts,
  register __d0 int userId,
  register __a6 struct NiKomBase *NiKomBase) {
  BPTR file;
  char filepath[41];
  int writeRes;
  struct Mote *conf;
  struct ConfAndText cat;

  ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);

  MakeUserFilePath(filepath, userId, "Bitmap0");
  if(!(file = Open(filepath, MODE_NEWFILE))) {
    ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);
    return 0;
  }
  if(Write(file, unreadTexts->bitmap, UNREADTEXTS_BITMAPSIZE/8) == -1) {
    Close(file);
    ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);
    return 0;
  }

  ITER_EL(conf, NiKomBase->Servermem->mot_list, mot_node, struct Mote *) {
    if(conf->type != MOTE_FIDO) {
      continue;
    }
    cat.conf = conf->nummer;
    cat.text = unreadTexts->lowestPossibleUnreadText[conf->nummer];
    writeRes = Write(file, &cat, sizeof(struct ConfAndText));
    if(writeRes == -1) {
      break;
    }
  }

  Close(file);
  ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_UNREAD]);
  return writeRes != -1;
}

void shiftBitmap(struct UnreadTexts *unreadTexts, int textNumberToAccomodate) {
  int textsToShiftBitmap, bytesToShiftBitmap, firstNewTextNumber, i;
  textsToShiftBitmap =
    textNumberToAccomodate - (unreadTexts->bitmapStartText + UNREADTEXTS_BITMAPSIZE);
  bytesToShiftBitmap = (textsToShiftBitmap / 8) + 1;
  firstNewTextNumber = unreadTexts->bitmapStartText + UNREADTEXTS_BITMAPSIZE;
  for(i = 0; i < bytesToShiftBitmap; i++) {
    unreadTexts->bitmap[((firstNewTextNumber % UNREADTEXTS_BITMAPSIZE) / 8) + i] = 0xff;
  }
  unreadTexts->bitmapStartText += bytesToShiftBitmap * 8;
}

void trimLowestPossibleUnreadText(struct UnreadTexts *unreadTexts, int conf,
    struct System *Servermem) {
  if(unreadTexts->lowestPossibleUnreadText[conf] < unreadTexts->bitmapStartText) {
    unreadTexts->lowestPossibleUnreadText[conf] = unreadTexts->bitmapStartText;
  }
  if(unreadTexts->lowestPossibleUnreadText[conf] < Servermem->info.lowtext) {
    unreadTexts->lowestPossibleUnreadText[conf] = Servermem->info.lowtext;
  }
}
