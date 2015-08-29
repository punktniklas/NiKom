#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"

#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "ConferenceTexts.h"

#define CONFTEXTS_DAT "NiKom:DatoCfg/ConferenceTexts.dat"

int writeSingleConfText(int arrayPos, struct System *Servermem);
int growConfTextsArray(int neededPos, struct System *Servermem);

int __saveds __asm LIBGetConferenceForText(
   register __d0 int textNumber,
   register __a6 struct NiKomBase *NiKomBase) {

  if((textNumber < NiKomBase->Servermem->info.lowtext)
     || (textNumber > NiKomBase->Servermem->info.hightext)) {
    return -1;
  }
  return NiKomBase->Servermem->confTexts
    .texts[textNumber - NiKomBase->Servermem->info.lowtext];
}

void __saveds __asm LIBSetConferenceForText(
   register __d0 int textNumber,
   register __d1 int conf,
   register __d2 int saveToDisk,
   register __a6 struct NiKomBase *NiKomBase) {

  int arrayPos;

  if((textNumber < NiKomBase->Servermem->info.lowtext)
     || (textNumber > NiKomBase->Servermem->info.hightext)) {
    return;
  }

  arrayPos = textNumber - NiKomBase->Servermem->info.lowtext;
  if(arrayPos >= NiKomBase->Servermem->confTexts.arraySize) {
    growConfTextsArray(arrayPos, NiKomBase->Servermem);
  }
  NiKomBase->Servermem->confTexts.texts[arrayPos] = conf;

  if(saveToDisk) {
    writeSingleConfText(arrayPos, NiKomBase->Servermem);
  }
}

int __saveds __asm LIBFindNextTextInConference(
   register __d0 int searchStart,
   register __d1 int conf,
   register __a6 struct NiKomBase *NiKomBase) {

  int i, textIndex;

  if(searchStart < NiKomBase->Servermem->info.lowtext) {
    searchStart = NiKomBase->Servermem->info.lowtext;
  }
  for(i = searchStart; i <= NiKomBase->Servermem->info.hightext; i++) {
    textIndex = i - NiKomBase->Servermem->info.lowtext;
    if(NiKomBase->Servermem->confTexts.texts[textIndex] == conf) {
      return i;
    }
  }
  return -1;
}

int __saveds __asm LIBFindPrevTextInConference(
   register __d0 int searchStart,
   register __d1 int conf,
   register __a6 struct NiKomBase *NiKomBase) {

  int i, textIndex;

  if(searchStart > NiKomBase->Servermem->info.hightext) {
    searchStart = NiKomBase->Servermem->info.hightext;
  }
  for(i = searchStart; i >= NiKomBase->Servermem->info.lowtext; i--) {
    textIndex = i - NiKomBase->Servermem->info.lowtext;
    if(NiKomBase->Servermem->confTexts.texts[textIndex] == conf) {
      return i;
    }
  }
  return -1;
}

int __saveds __asm LIBWriteConferenceTexts(
   register __a6 struct NiKomBase *NiKomBase) {
  BPTR file;
  int writeRes, textsToWrite;

  textsToWrite = NiKomBase->Servermem->info.hightext
    - NiKomBase->Servermem->info.lowtext
    + 1;
  if(textsToWrite == 0) {
    return 1;
  }
  ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_CONFTEXTS]);

  if(!(file = Open(CONFTEXTS_DAT, MODE_NEWFILE))) {
    ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_CONFTEXTS]);
    return 0;
  }
  writeRes = Write(file, NiKomBase->Servermem->confTexts.texts,
                   textsToWrite * sizeof(short));
  Close(file);
  ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_CONFTEXTS]);
  return writeRes == textsToWrite * sizeof(short);
}

int writeSingleConfText(int arrayPos, struct System *Servermem) {
  BPTR file;
  int writeRes;

  ObtainSemaphore(&Servermem->semaphores[NIKSEM_CONFTEXTS]);
  if(!(file = Open(CONFTEXTS_DAT, MODE_OLDFILE))) {
    ReleaseSemaphore(&Servermem->semaphores[NIKSEM_CONFTEXTS]);
    return 0;
  }
  if(Seek(file, arrayPos * sizeof(short), OFFSET_BEGINNING) == -1) {
    Close(file);
    ReleaseSemaphore(&Servermem->semaphores[NIKSEM_CONFTEXTS]);
    return 0;
  }
  writeRes = Write(file, &Servermem->confTexts.texts[arrayPos], sizeof(short));
  Close(file);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_CONFTEXTS]);
  return writeRes == sizeof(short);
}

int growConfTextsArray(int neededPos, struct System *Servermem) {
  int newArraySize, oldArraySize;
  short *newArray, *oldArray;

  newArraySize = neededPos + 1000;
  if(!(newArray = AllocMem(newArraySize * sizeof(short), MEMF_CLEAR | MEMF_PUBLIC))) {
    return 0;
  }
  oldArray = Servermem->confTexts.texts;
  oldArraySize = Servermem->confTexts.arraySize;
  memcpy(newArray, oldArray, oldArraySize * sizeof(short));
  Servermem->confTexts.texts = newArray;
  Servermem->confTexts.arraySize = newArraySize;
  FreeMem(oldArray, oldArraySize * sizeof(short));
}
