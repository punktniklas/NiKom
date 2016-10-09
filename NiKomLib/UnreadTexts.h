#ifndef NIKOMLIB_H
#include "NiKomLib.h"
#endif

void __saveds __asm LIBChangeUnreadTextStatus(
   register __d0 int textNumber,
   register __d1 int markAsUnread,
   register __a0 struct UnreadTexts *unreadTexts,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBIsTextUnread(
  register __d0 int textNumber,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBFindNextUnreadText(
  register __d0 int searchStart,
  register __d1 int conf,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase);
void __saveds __asm LIBInitUnreadTexts(
   register __a0 struct UnreadTexts *unreadTexts,
   register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBCountUnreadTexts(
  register __d0 int conf,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase);
void __saveds __asm LIBSetUnreadTexts(
  register __d0 int conf,
  register __d1 int amount,
  register __a0 struct UnreadTexts *unreadTexts,
  register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBReadUnreadTexts(
  register __a0 struct UnreadTexts *unreadTexts,
  register __d0 int userId,
  register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBWriteUnreadTexts(
  register __a0 struct UnreadTexts *unreadTexts,
  register __d0 int userId,
  register __a6 struct NiKomBase *NiKomBase);
