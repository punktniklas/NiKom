#include <stdlib.h>
#include "StringUtils.h"

#include "ConfCommon.h"

extern int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf;

int parseTextNumber(char *str, int requestedType) {
  if(IzDigit(str[0])) {
    return atoi(str);
  }
  if(str[0] == 's' || str[0] == 'S') { // "Senaste"
    if(requestedType != 0 && g_lastKomTextType != requestedType) {
      return -1;
    }
    return g_lastKomTextNr;
  }
  return -1;
}
