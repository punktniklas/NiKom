#ifndef NIKOMLIB_H
#include "NiKomLib.h"
#endif

int __saveds AASM LIBGetConferenceForText(
   register __d0 int textNumber,
   register __a6 struct NiKomBase *NiKomBase);
void __saveds AASM LIBSetConferenceForText(
   register __d0 int textNumber,
   register __d1 int conf,
   register __d2 int saveToDisk,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds AASM LIBFindNextTextInConference(
   register __d0 int searchStart,
   register __d1 int conf,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds AASM LIBFindPrevTextInConference(
   register __d0 int searchStart,
   register __d1 int conf,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds AASM LIBWriteConferenceTexts(
   register __a6 struct NiKomBase *NiKomBase);
int __saveds AASM LIBDeleteConferenceTexts(
   register __d0 int numberOfTexts,
   register __a6 struct NiKomBase *NiKomBase);
