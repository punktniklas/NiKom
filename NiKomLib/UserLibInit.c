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
#include "funcs.h"

/* För UserLibInit() */
#define ERROR 1
#define OK    0

struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;
struct Library *RexxSysBase, *UtilityBase;

char reggadnamn[60]; /* Namnet på den som reggat NiKom. */

/*
 *  nikom.librarys privata initiuerings/cleanup-rutiner.
 */

void copychrstables(struct NiKomBase *NiKomBase) {

#include "chartabs.h"

	memcpy(NiKomBase->IbmToAmiga,IbmToAmiga,256);
	memcpy(NiKomBase->SF7ToAmiga,SF7ToAmiga,256);
	memcpy(NiKomBase->MacToAmiga,MacToAmiga,256);
	memcpy(NiKomBase->AmigaToIbm,AmigaToIbm,256);
	memcpy(NiKomBase->AmigaToSF7,AmigaToSF7,256);
	memcpy(NiKomBase->AmigaToMac,AmigaToMac,256);
}

int __saveds __asm
__UserLibInit(register __a6 struct NiKomBase * NiKomBase)
{
	/* NiKomBase private initialization */

	if(!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",37L))) return(ERROR);
	if(!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37L))) {
		CloseLibrary((struct Library *)DOSBase);
		return(ERROR);
	}
	if(!(RexxSysBase = OpenLibrary("rexxsyslib.library",36L))) {
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)DOSBase);
		return(ERROR);
	}
	if(!(UtilityBase = OpenLibrary("utility.library",37L))) {
		CloseLibrary(RexxSysBase);
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)DOSBase);
		return(ERROR);
	}
	if(!getlastmatrix(NiKomBase)) {
		CloseLibrary(UtilityBase);
		CloseLibrary(RexxSysBase);
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)DOSBase);
		return(ERROR);
	}
	InitSemaphore(& NiKomBase->sem);
	copychrstables(NiKomBase);
	getkeyfile();

	return (OK);
}

void __saveds __asm
__UserLibCleanup(register __a6 struct NiKomBase * NiKomBase)
{
	CloseLibrary(UtilityBase);
	CloseLibrary(RexxSysBase);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)DOSBase);
}

void __saveds __asm
LIBInitServermem(register __a0 struct System *Servermem, register __a6 struct NiKomBase * NiKomBase) {
	NiKomBase->Servermem = Servermem;
}

int getkeyfile(void) {
	BPTR fh;
	long checksum,serial,cnt,len,countcs=0;
	char tmp;
	if(!(fh=Open("NiKom:NiKom.key",MODE_OLDFILE))) return(1);
	if(Read(fh,&checksum,sizeof(long))==-1) {
		Close(fh);
		return(0);
	}
	if(Read(fh,&serial,sizeof(long))==-1) {
		Close(fh);
		return(0);
	}
	if(serial%42 && Seek(fh,serial%42,OFFSET_CURRENT)==-1) {
		Close(fh);
		return(0);
	}
	Flush(fh);
	if((len=FGetC(fh))==-1) {
		Close(fh);
		return(0);
	}
	for(cnt=0;cnt<len;cnt++) {
		if((reggadnamn[cnt]=FGetC(fh))==-1) {
			Close(fh);
			return(0);
		}
		tmp=~reggadnamn[cnt];
		tmp&=44;
		reggadnamn[cnt]&=~44;
		reggadnamn[cnt]|=tmp;
		countcs+=reggadnamn[cnt];
		if(FGetC(fh)==-1) {
			Close(fh);
			return(0);
		}
		if(FGetC(fh)==-1) {
			Close(fh);
			return(0);
		}
	}
	Close(fh);
	countcs=countcs*serial*4711/33;
	if(countcs!=checksum) return(0);
}
