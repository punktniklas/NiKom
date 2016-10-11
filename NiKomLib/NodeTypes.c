#include <exec/types.h>
#include <dos/dos.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

struct NodeType * __saveds __asm LIBGetNodeType(register __d0 long number, register __a6 struct NiKomBase *NiKomBase) {
	int x;

	if(!NiKomBase->Servermem) return(NULL);

	for(x=0; x < MAXNODETYPES; x++) {
		if(NiKomBase->Servermem->nodetypes[x].nummer == number) return(&NiKomBase->Servermem->nodetypes[x]);
	}
	return(NULL);
}
