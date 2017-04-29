#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "NiKomBase.h"
#include "Funcs.h"

/*
 *  FUNCTION
 *      LockNikBase
 *
 *  DESCRIPTION
 *      Gain exclusive rights to NikBase
 */

void __saveds AASM
LIBLockNiKomBase(register __a6 struct NiKomBase * NiKomBase)
{
    ObtainSemaphore(& NiKomBase->sem);
}


/*
 *  FUNCTION
 *      UnLockNikBase
 *
 *  DESCRIPTION
 *      Make everybody else capable of using NikBase.
 */

void __saveds AASM
LIBUnLockNiKomBase(register __a6 struct NiKomBase * NiKomBase)
{
    ReleaseSemaphore(& NiKomBase->sem);
}
