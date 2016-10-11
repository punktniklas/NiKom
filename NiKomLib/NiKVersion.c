#include <exec/types.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"
#include "VersionStrings.h"

const char dosversion[] = "$VER: nikom.library_" NIKRELEASE " " NIKLIBVERSIONSTR "." NIKLIBREVISIONSTR " " __AMIGADATE__;

char * __saveds __asm LIBGetBuildTime(void) {
  return __DATE__ " " __TIME__;
}
