#ifndef NIKOMLIB_H
#include "NiKomLib.h"
#endif

int __saveds AASM LIBGetConferenceForText(
   register __d0 int textNumber AREG(d0),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
void __saveds AASM LIBSetConferenceForText(
   register __d0 int textNumber AREG(d0),
   register __d1 int conf AREG(d1),
   register __d2 int saveToDisk AREG(d2),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBFindNextTextInConference(
   register __d0 int searchStart AREG(d0),
   register __d1 int conf AREG(d1),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBFindPrevTextInConference(
   register __d0 int searchStart AREG(d0),
   register __d1 int conf AREG(d1),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBWriteConferenceTexts(
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBDeleteConferenceTexts(
   register __d0 int numberOfTexts AREG(d0),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
