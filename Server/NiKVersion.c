#include <stdlib.h>
#include "NiKomStr.h"
#include "VersionStrings.h"

extern struct System *Servermem;

char dosversion[]="\0$VER: NiKServer " NIKRELEASE " " __AMIGADATE__;

void GetServerversion(void) {
  Servermem->serverBuildTime = __DATE__ " " __TIME__;
}
