#include <exec/types.h>
#include <rexx/storage.h>
#include <dos.h>
#include <string.h>

#include "Funcs.h"

/* Glöm inte att lägga till koll om Servermem finns! */

LONG __saveds AASM LIBRexxEntry(register __a0 struct RexxMsg *mess AREG(a0),register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
	if(!stricmp(mess->rm_Args[0],"MATRIX2NIKOM")) LIBMatrix2NiKom(NiKomBase);
	else return(1);
	__builtin_putreg(REG_A1,NULL);
	return(0);
}
