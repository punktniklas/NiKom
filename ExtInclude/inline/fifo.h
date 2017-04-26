#ifndef _INLINE_FIFO_H
#define _INLINE_FIFO_H

#ifndef CLIB_FIFO_PROTOS_H
#define CLIB_FIFO_PROTOS_H
#endif

#include <exec/types.h>

#ifndef FIFO_BASE_NAME
#define FIFO_BASE_NAME FifoBase
#endif

#define OpenFifo(name, bytes, flags) ({ \
  char * _OpenFifo_name = (name); \
  long _OpenFifo_bytes = (bytes); \
  long _OpenFifo_flags = (flags); \
  ({ \
  register char * _OpenFifo__bn __asm("a6") = (char *) (FIFO_BASE_NAME);\
  ((void * (*)(char * __asm("a6"), char * __asm("d0"), long __asm("d1"), long __asm("a0"))) \
  (_OpenFifo__bn - 30))(_OpenFifo__bn, _OpenFifo_name, _OpenFifo_bytes, _OpenFifo_flags); \
});})

#define CloseFifo(fifo, flags) ({ \
  void * _CloseFifo_fifo = (fifo); \
  long _CloseFifo_flags = (flags); \
  ({ \
  register char * _CloseFifo__bn __asm("a6") = (char *) (FIFO_BASE_NAME);\
  ((void (*)(char * __asm("a6"), void * __asm("d0"), long __asm("d1"))) \
  (_CloseFifo__bn - 36))(_CloseFifo__bn, _CloseFifo_fifo, _CloseFifo_flags); \
});})

#define ReadFifo(fifo, buf, bytes) ({ \
  void * _ReadFifo_fifo = (fifo); \
  char ** _ReadFifo_buf = (buf); \
  long _ReadFifo_bytes = (bytes); \
  ({ \
  register char * _ReadFifo__bn __asm("a6") = (char *) (FIFO_BASE_NAME);\
  ((long (*)(char * __asm("a6"), void * __asm("d0"), char ** __asm("d1"), long __asm("a0"))) \
  (_ReadFifo__bn - 42))(_ReadFifo__bn, _ReadFifo_fifo, _ReadFifo_buf, _ReadFifo_bytes); \
});})

#define WriteFifo(fifo, buf, bytes) ({ \
  void * _WriteFifo_fifo = (fifo); \
  char * _WriteFifo_buf = (buf); \
  long _WriteFifo_bytes = (bytes); \
  ({ \
  register char * _WriteFifo__bn __asm("a6") = (char *) (FIFO_BASE_NAME);\
  ((long (*)(char * __asm("a6"), void * __asm("d0"), char * __asm("d1"), long __asm("a0"))) \
  (_WriteFifo__bn - 48))(_WriteFifo__bn, _WriteFifo_fifo, _WriteFifo_buf, _WriteFifo_bytes); \
});})

#define RequestFifo(fifo, msg, req) ({ \
  void * _RequestFifo_fifo = (fifo); \
  struct Message * _RequestFifo_msg = (msg); \
  long _RequestFifo_req = (req); \
  ({ \
  register char * _RequestFifo__bn __asm("a6") = (char *) (FIFO_BASE_NAME);\
  ((void (*)(char * __asm("a6"), void * __asm("d0"), struct Message * __asm("d1"), long __asm("a0"))) \
  (_RequestFifo__bn - 54))(_RequestFifo__bn, _RequestFifo_fifo, _RequestFifo_msg, _RequestFifo_req); \
});})

#define BufSizeFifo(fifo) ({ \
  void * _BufSizeFifo_fifo = (fifo); \
  ({ \
  register char * _BufSizeFifo__bn __asm("a6") = (char *) (FIFO_BASE_NAME);\
  ((long (*)(char * __asm("a6"), void * __asm("d0"))) \
  (_BufSizeFifo__bn - 60))(_BufSizeFifo__bn, _BufSizeFifo_fifo); \
});})

#endif /*  _INLINE_FIFO_H  */
