#include <dos/dos.h>
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "NiKomStr.h"
#include "Terminal.h"
#include "NiKVersion.h"

extern char outbuffer[];
extern struct System *Servermem;

char dosversion[]="\0$VER: NiKomCon/Ser " NIKRELEASE " " __AMIGADATE__;

void DisplayVersionInfo(void) {
  SendString("\n\n\rNiKom " NIKRELEASE
             " © Niklas Lindholm 1990-1996, 2015 & Tomas Kärki 1996-1998\r\n");
  SendString("  KOM node      built " __DATE__ " " __TIME__ "\r\n");
  SendString("  Server        built %s\r\n",Servermem->serverBuildTime);
  SendString("  nikom.library built %s\r\n", GetBuildTime());
}
