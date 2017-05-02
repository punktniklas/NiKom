/* "fifo.library"*/
void *OpenFifo(char *, long, long);
void CloseFifo(void *, long);
long ReadFifo(void *, char **, long);
long WriteFifo(void *, char *, long);
void RequestFifo(void *, struct Message *, long);
long BufSizeFifo(void *);

#ifdef __GNUC__
# include "inline/fifo.h"
#else
#pragma libcall FifoBase OpenFifo 1E 81003
#pragma libcall FifoBase CloseFifo 24 1002
#pragma libcall FifoBase ReadFifo 2A 81003
#pragma libcall FifoBase WriteFifo 30 81003
#pragma libcall FifoBase RequestFifo 36 81003
#pragma libcall FifoBase BufSizeFifo 3C 001
#endif
