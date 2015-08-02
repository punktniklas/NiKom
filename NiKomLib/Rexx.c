#include <exec/types.h>
#include <rexx/storage.h>
#include <dos.h>
#include <string.h>

#include "funcs.h"

/* Glöm inte att lägga till koll om Servermem finns! */

LONG __saveds __asm LIBRexxEntry(register __a0 struct RexxMsg *mess,register __a6 struct NiKomBase *NiKomBase) {
	if(!stricmp(mess->rm_Args[0],"MATRIX2NIKOM")) LIBMatrix2NiKom(NiKomBase);
	else if(!stricmp(mess->rm_Args[0],"DEBUGREQ")) rexxdebugreq(mess);
	else return(1);
	__builtin_putreg(REG_A1,NULL);
	return(0);
}

void rexxdebugreq(struct RexxMsg *mess) {
	debug_req(mess->rm_Args[1],(APTR)mess->rm_Args[2]);
}
