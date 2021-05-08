#include <dos/dos.h>
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "NiKomStr.h"
#include "VersionStrings.h"
#include "Terminal.h"
#include "NiKVersion.h"

extern char outbuffer[];
extern struct System *Servermem;

char dosversion[]="\0$VER: NiKomCon/Ser " NIKRELEASE " " __AMIGADATE__;

void DisplayVersionInfo(void) {
  SendString("\n\n\rNiKom " NIKRELEASE
             " © Niklas Lindholm & contributors 1990-2021\r\n");
  SendString("  KOM node      built " __DATE__ " " __TIME__ "\r\n");
  SendString("  Server        built %s\r\n",Servermem->serverBuildTime);
  SendString("  nikom.library built %s\r\n", GetBuildTime());
}
