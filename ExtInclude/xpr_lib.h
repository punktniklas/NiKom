/* For use in comm programs written in Lattice C; allows calling XProtocol
   library functions directly without glue routines. */

long XProtocolCleanup(struct XPR_IO *);
long XProtocolSetup(struct XPR_IO *);
long XProtocolSend(struct XPR_IO *);
long XProtocolReceive(struct XPR_IO *);

#ifdef __GNUC__
# include "inline/xpr.h"
#else
#pragma libcall XProtocolBase XProtocolCleanup 1e 801
#pragma libcall XProtocolBase XProtocolSetup 24 801
#pragma libcall XProtocolBase XProtocolSend 2a 801
#pragma libcall XProtocolBase XProtocolReceive 30 801
#pragma libcall XProtocolBase XProtocolHostMon 36 109804
#pragma libcall XProtocolBase XProtocolUserMon 3c 109804
#endif
