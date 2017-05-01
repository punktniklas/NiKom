#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"

#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "ConferenceTexts.h"

#define CONFTEXTS_DAT "NiKom:DatoCfg/ConferenceTexts.dat"

/* ******* Local functions ******** */

static int writeSingleConfText(int arrayPos, struct System *Servermem);
static int growConfTextsArray(int neededPos, struct System *Servermem);

/* ******* Library functions ******** */

/****** nikom.library/GetConferenceForText *************************

    NAME
        GetConferenceForText -- Returns the conference the given text is in.

    SYNOPSIS
        GetConferenceForText(textNumber)
                             D0
        int GetConferenceForText(int);

    FUNCTION
        Returns the conference the given text is in.

    RESULT
        conf - The conference the text is in.

    INPUTS
        textNumber - the text number.

**********************************************************************/

int __saveds AASM LIBGetConferenceForText(
   register __d0 int textNumber AREG(d0),
   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

  if((textNumber < NiKomBase->Servermem->info.lowtext)
     || (textNumber > NiKomBase->Servermem->info.hightext)) {
    return -1;
  }
  return NiKomBase->Servermem->confTexts
    .texts[textNumber - NiKomBase->Servermem->info.lowtext];
}

/****** nikom.library/SetConferenceForText *************************

    NAME
        SetConferenceForText -- Sets the conference the given text is in.

    SYNOPSIS
        SetConferenceForText(textNumber, conf, saveToDisk)
                             D0          D1    D2
        void SetConferenceForText(int, int, int);

    FUNCTION
        Sets the conference the given text is in. If the text number
        is lower than the lowest text or higher than the highest text
        in the system this function will do nothing.

        If saveToDisk is non zero the change will also be written to
        disk. Not writing to disk can be useful if a large number of
        changes is to be made in which case opening and closing the file
        for every change will be inefficient. In this case the function
        WriteConferenceTexts() should be called when all changes are made.

    INPUTS
        textNumber - the text number.
        conf       - the conference to set for the text.
        saveToDisk - non zero if changes should be written to disk

**********************************************************************/

void __saveds AASM LIBSetConferenceForText(
   register __d0 int textNumber AREG(d0),
   register __d1 int conf AREG(d1),
   register __d2 int saveToDisk AREG(d2),
   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

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

/****** nikom.library/FindNextTextInConference *************************

    NAME
        FindNextTextInConference -- Searches for the next text in the conference.

    SYNOPSIS
        FindNextTextInConference(searchStart, conf)
                                 D0           D1
        int FindNextTextInConference(int, int);

    FUNCTION
        Searches for the next text in the given conference. The search
        starts at (and includes) the given text number. If no text is
        found -1 is returned.

    RESULT
        textNumber - the next text or -1 if none is found.

    INPUTS
        searchStart - The text to start searching at
        conf        - The conference to search for

**********************************************************************/

int __saveds AASM LIBFindNextTextInConference(
   register __d0 int searchStart AREG(d0),
   register __d1 int conf AREG(d1),
   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

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

/****** nikom.library/FindPrevTextInConference *************************

    NAME
        FindPrevTextInConference -- Searches for the previous text in the conference.

    SYNOPSIS
        FindPrevTextInConference(searchStart, conf)
                                 D0           D1
        int FindPrevTextInConference(int, int);

    FUNCTION
        Searches for the previous text in the given conference. The search
        starts at (and includes) the given text number. If no text is
        found -1 is returned.

    RESULT
        textNumber - the previous text or -1 if none is found.

    INPUTS
        searchStart - The text to start searching at
        conf        - The conference to search for

**********************************************************************/

int __saveds AASM LIBFindPrevTextInConference(
   register __d0 int searchStart AREG(d0),
   register __d1 int conf AREG(d1),
   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

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

/****** nikom.library/WriteConferenceTexts *************************

    NAME
        WriteConferenceTexts -- Writes conference text data to disk.

    SYNOPSIS
        WriteConferenceTexts()

        int WriteConferenceTexts(void);

    FUNCTION
        Writes the entire array of conference text data to disk,
        overwriting existing contents. If there are no texts in the
        system this function will do nothing.

    RESULT
        sucess - Zero if writing failed, non zero otherwise.

**********************************************************************/

int __saveds AASM LIBWriteConferenceTexts(
   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
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

/****** nikom.library/DeleteConferenceTexts *************************

    NAME
        DeleteConferenceTexts -- Deletes texts from the conference texts array

    SYNOPSIS
        DeleteConferenceTexts(numberOfTexts)
                              D0
        int DeleteConferenceTexts(int);

    FUNCTION
        Deletes the given number of texts from the conference texts
        array and updates Servermem->info.lowtext. WARNING: This
        should always be followed by deleting files in NiKom:Moten.
        This function will not take care of that.

        The number of texts to delete must be a multiple of 512 and
        can not be higher than the total number of texts in the system.

    RESULT
        success - Zero if bad input or writing the new array failed.

    INPUTS
        numberOfTexts - The number of texts to delete.

**********************************************************************/

int __saveds AASM LIBDeleteConferenceTexts(
   register __d0 int numberOfTexts AREG(d0),
   register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

  int newTotalTexts;

  newTotalTexts = NiKomBase->Servermem->info.hightext
    - NiKomBase->Servermem->info.lowtext
    + 1
    - numberOfTexts;
  if((numberOfTexts % 512) != 0 || newTotalTexts < 0) {
    return 0;
  }

  memmove(NiKomBase->Servermem->confTexts.texts,
          &NiKomBase->Servermem->confTexts.texts[numberOfTexts],
          newTotalTexts * sizeof(short));
  NiKomBase->Servermem->info.lowtext += numberOfTexts;
  return WriteConferenceTexts();
}

static int writeSingleConfText(int arrayPos, struct System *Servermem) {
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

static int growConfTextsArray(int neededPos, struct System *Servermem) {
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
  return 1;
}
