#ifndef NIKOMLIB_H
#include "/Include/NiKomLib.h"
#endif

int __saveds __asm LIBGetConferenceForText(
   register __d0 int textNumber,
   register __a6 struct NiKomBase *NiKomBase);
void __saveds __asm LIBSetConferenceForText(
   register __d0 int textNumber,
   register __d1 int conf,
   register __d2 int saveToDisk,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBFindNextTextInConference(
   register __d0 int searchStart,
   register __d1 int conf,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBFindPrevTextInConference(
   register __d0 int searchStart,
   register __d1 int conf,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBWriteConferenceTexts(
   register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBDeleteConferenceTexts(
   register __d0 int numberOfTexts,
   register __a6 struct NiKomBase *NiKomBase);
