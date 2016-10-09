#ifndef NIKOMBASE_H

#include <exec/types.h>

#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/semaphores.h>

#ifndef NIKOMSTR_H
#include "NiKomStr.h"
#endif


/*
 *  STRUCT
 *	NiKomBase
 *
 *  DESCRIPTION
 */

struct NiKomBase {
	struct Library lib;

	ULONG  seglist;		/* Used by init/exit-code. */

	struct SignalSemaphore sem;	/* Semaphore to gain exclusive access to NiKomBase */

	struct System *Servermem;
	UBYTE IbmToAmiga[256], AmigaToIbm[256],
			SF7ToAmiga[256], AmigaToSF7[256],
			MacToAmiga[256], AmigaToMac[256];

};

#define LITTLE_ENDIAN NiKomBase->Servermem->fidodata.littleEndianByteOrder

#define NIKOMBASE_H
#endif /* NIKOMBASE */
