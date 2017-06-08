#include <exec/types.h>
#include <dos/dos.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

struct NodeType * __saveds AASM LIBGetNodeType(register __d0 long number AREG(d0), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
	int x;

	if(!NiKomBase->Servermem) return(NULL);

	for(x=0; x < MAXNODETYPES; x++) {
		if(NiKomBase->Servermem->cfg->nodetypes[x].nummer == number) return(&NiKomBase->Servermem->cfg->nodetypes[x]);
	}
	return(NULL);
}
