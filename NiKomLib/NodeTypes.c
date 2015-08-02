#include <exec/types.h>
#include <dos/dos.h>
#include "/Include/NiKomStr.h"
#include "/Include/NiKomLib.h"
#include "NiKomBase.h"
#include "funcs.h"

struct NodeType * __saveds __asm LIBGetNodeType(register __d0 long number, register __a6 struct NiKomBase *NiKomBase) {
	int x;

	if(!NiKomBase->Servermem) return(NULL);

	for(x=0; x < MAXNODETYPES; x++) {
		if(NiKomBase->Servermem->nodetypes[x].nummer == number) return(&NiKomBase->Servermem->nodetypes[x]);
	}
	return(NULL);
}
