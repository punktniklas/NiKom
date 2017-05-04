#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <libraries/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include "NiKomBase.h"
#include "Funcs.h"

/* För UserLibInit() */
#define RET_ERROR 1
#define RET_OK    0

struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;
struct Library *RexxSysBase, *UtilityBase;

/*
 *  nikom.librarys privata initiuerings/cleanup-rutiner.
 */

void copychrstables(struct NiKomBase *NiKomBase) {

#include "CharTabs.h"

	memcpy(NiKomBase->IbmToAmiga,IbmToAmiga,256);
	memcpy(NiKomBase->SF7ToAmiga,SF7ToAmiga,256);
	memcpy(NiKomBase->MacToAmiga,MacToAmiga,256);
	memcpy(NiKomBase->CP850ToAmiga,CP850ToAmiga,256);
	memcpy(NiKomBase->CP866ToAmiga,CP866ToAmiga,256);
	memcpy(NiKomBase->AmigaToIbm,AmigaToIbm,256);
	memcpy(NiKomBase->AmigaToSF7,AmigaToSF7,256);
	memcpy(NiKomBase->AmigaToMac,AmigaToMac,256);
	memcpy(NiKomBase->AmigaToCP850,AmigaToCP850,256);
	memcpy(NiKomBase->AmigaToCP866,AmigaToCP866,256);
}

int __saveds AASM
__UserLibInit(register __a6 struct NiKomBase * NiKomBase AREG(a6))
{
	/* NiKomBase private initialization */

	if(!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",37L))) return(RET_ERROR);
	if(!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37L))) {
		CloseLibrary((struct Library *)DOSBase);
		return(RET_ERROR);
	}
	if(!(RexxSysBase = OpenLibrary("rexxsyslib.library",36L))) {
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)DOSBase);
		return(RET_ERROR);
	}
	if(!(UtilityBase = OpenLibrary("utility.library",37L))) {
		CloseLibrary(RexxSysBase);
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)DOSBase);
		return(RET_ERROR);
	}
	InitSemaphore(& NiKomBase->sem);
	copychrstables(NiKomBase);

	return (RET_OK);
}

void __saveds AASM
__UserLibCleanup(register __a6 struct NiKomBase * NiKomBase AREG(a6))
{
	CloseLibrary(UtilityBase);
	CloseLibrary(RexxSysBase);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)DOSBase);
}

void __saveds AASM
LIBInitServermem(register __a0 struct System *Servermem AREG(a0), register __a6 struct NiKomBase * NiKomBase AREG(a6)) {
	NiKomBase->Servermem = Servermem;
}
