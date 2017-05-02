#ifndef _INLINE_XPR_H
#define _INLINE_XPR_H

#ifndef CLIB_XPR_PROTOS_H
#define CLIB_XPR_PROTOS_H
#endif

#include <exec/types.h>

#ifndef XPR_BASE_NAME
#define XPR_BASE_NAME XProtocolBase
#endif

#define XProtocolCleanup(XIO) ({ \
  struct XPR_IO * _XProtocolCleanup_XIO = (XIO); \
  ({ \
  register char * _XProtocolCleanup__bn __asm("a6") = (char *) (XPR_BASE_NAME);\
  ((long (*)(char * __asm("a6"), struct XPR_IO * __asm("a0"))) \
  (_XProtocolCleanup__bn - 30))(_XProtocolCleanup__bn, _XProtocolCleanup_XIO); \
});})

#define XProtocolSetup(XIO) ({ \
  struct XPR_IO * _XProtocolSetup_XIO = (XIO); \
  ({ \
  register char * _XProtocolSetup__bn __asm("a6") = (char *) (XPR_BASE_NAME);\
  ((long (*)(char * __asm("a6"), struct XPR_IO * __asm("a0"))) \
  (_XProtocolSetup__bn - 36))(_XProtocolSetup__bn, _XProtocolSetup_XIO); \
});})

#define XProtocolSend(XIO) ({ \
  struct XPR_IO * _XProtocolSend_XIO = (XIO); \
  ({ \
  register char * _XProtocolSend__bn __asm("a6") = (char *) (XPR_BASE_NAME);\
  ((long (*)(char * __asm("a6"), struct XPR_IO * __asm("a0"))) \
  (_XProtocolSend__bn - 42))(_XProtocolSend__bn, _XProtocolSend_XIO); \
});})

#define XProtocolReceive(XIO) ({ \
  struct XPR_IO * _XProtocolReceive_XIO = (XIO); \
  ({ \
  register char * _XProtocolReceive__bn __asm("a6") = (char *) (XPR_BASE_NAME);\
  ((long (*)(char * __asm("a6"), struct XPR_IO * __asm("a0"))) \
  (_XProtocolReceive__bn - 48))(_XProtocolReceive__bn, _XProtocolReceive_XIO); \
});})

#endif /*  _INLINE_XPR_H  */
