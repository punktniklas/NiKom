#include <exec/types.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include "/Include/NiKomStr.h"
#include "/Include/NiKomLib.h"
#include "NiKomBase.h"
#include "funcs.h"

const char dosversion[] = "$VER: nikom.library " NIKLIBVERSION "." NIKLIBREVISION " " __AMIGADATE__;

void __saveds __asm LIBGetNiKomVersion(register __a0 int *version, register __a1 int *revision,
	register __a2 char *release) {

	if(version) *version = atoi(NIKLIBVERSION);
	if(revision) *revision = atoi(NIKLIBREVISION);
	if(release) strcpy(release,NIKRELEASE);
}
