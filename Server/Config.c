#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "NiKomStr.h"
#include "Config.h"

extern struct System *Servermem;

int initLegacyConversionData(void) {
  BPTR file;
  char line[100];
  int putsRes = 0;

  Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted = -1;

  if(!(file = Open("NiKom:DatoCfg/LegacyConversion.dat", MODE_READWRITE))) {
    return 0;
  }
  while(FGets(file, line, 100)) {
    if(strncmp(line, "LowTextWhenBitmap0ConversionStarted", 35) == 0) {
      Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted =
        atoi(&line[36]);
    }
  }

  if(Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted == -1) {
    Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted =
      Servermem->info.lowtext;
    sprintf(line, "LowTextWhenBitmap0ConversionStarted=%d\n",
            Servermem->info.lowtext);
    putsRes = FPuts(file, line);
  }
  Close(file);

  printf("Read LegacyConversion.dat\n");

  return putsRes == 0;
}
