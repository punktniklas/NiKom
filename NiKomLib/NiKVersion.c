#include <exec/types.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"
#include "VersionStrings.h"

const char dosversion[] = DOSVERSION;

char * __saveds AASM LIBGetBuildTime(void) {
  return __DATE__ " " __TIME__;
}
