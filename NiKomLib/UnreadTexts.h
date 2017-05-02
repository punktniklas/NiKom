#ifndef NIKOMLIB_H
#include "NiKomLib.h"
#endif

void __saveds AASM LIBChangeUnreadTextStatus(
   register __d0 int textNumber AREG(d0),
   register __d1 int markAsUnread AREG(d1),
   register __a0 struct UnreadTexts *unreadTexts AREG(a0),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBIsTextUnread(
  register __d0 int textNumber AREG(d0),
  register __a0 struct UnreadTexts *unreadTexts AREG(a0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBFindNextUnreadText(
  register __d0 int searchStart AREG(d0),
  register __d1 int conf AREG(d1),
  register __a0 struct UnreadTexts *unreadTexts AREG(a0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
void __saveds AASM LIBInitUnreadTexts(
   register __a0 struct UnreadTexts *unreadTexts AREG(a0),
   register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBCountUnreadTexts(
  register __d0 int conf AREG(d0),
  register __a0 struct UnreadTexts *unreadTexts AREG(a0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
void __saveds AASM LIBSetUnreadTexts(
  register __d0 int conf AREG(d0),
  register __d1 int amount AREG(d1),
  register __a0 struct UnreadTexts *unreadTexts AREG(a0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBReadUnreadTexts(
  register __a0 struct UnreadTexts *unreadTexts AREG(a0),
  register __d0 int userId AREG(d0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBWriteUnreadTexts(
  register __a0 struct UnreadTexts *unreadTexts AREG(a0),
  register __d0 int userId AREG(d0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
