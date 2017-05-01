#include <exec/types.h>
#include <rexx/storage.h>
#include <string.h>

#include "Funcs.h"

#ifdef __GNUC__
# define clear_a1() asm("subal %a1,%a1")
#else
# include <dos.h>
# define clear_a1() putreg(REG_A1, NULL);
#endif

/* Glöm inte att lägga till koll om Servermem finns! */

LONG __saveds AASM LIBRexxEntry(register __a0 struct RexxMsg *mess AREG(a0),register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
	if(!stricmp(mess->rm_Args[0],"MATRIX2NIKOM")) LIBMatrix2NiKom(NiKomBase);
	else return(1);
	clear_a1();
	return(0);
}
