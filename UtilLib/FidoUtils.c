#include <stdlib.h>
#include "StringUtils.h"

#include "FidoUtils.h"

/*
 * Parses a FidoNet address into its numeric components. The format
 * is Zone:Net/Node.Point (e.g. 2:201/420.0) where zone and point is optional.
 * Zone defaults to 1 and point to 0. "123/456" is equal to "1:123/456.0".
 * The parsed result is stored in the four first positions of the given
 * results array.
 * The string to parsed can be followed by a NUL byte, whitespace or '@'.
 * Other characters will result in a parsing error.
 *
 * Returns 0 if parsing fails and non zero if successful.
 */
int ParseFidoAddress(char *str, int result[]) {
  int foundZone = 0, foundNet = 0, foundNode = 0;
  char *start, *search;
  start = search = str;
  for(;;) {
    if(*search == '\0' || IzSpace(*search) || *search == '@') {
      break;
    }
    switch(*search) {
    case ':':
      if(foundZone || foundNet || foundNode) {
        return 0;
      }
      result[0] = atoi(start);
      foundZone = 1;
      start = search + 1;
      break;
    case '/':
      if(foundNet || foundNode) {
        return 0;
      }
      result[1] = atoi(start);
      foundNet = 1;
      if(!foundZone) {
        result[0] = 1;
        foundZone = 1;
      }
      start = search + 1;
      break;
    case '.':
      if(!foundNet || foundNode) {
        return 0;
      }
      result[2] = atoi(start);
      foundNode = 1;
      start = search + 1;
      break;
    default:
      if(!IzDigit(*search)) {
        return 0;
      }
    }
    search++;
  }
  if(!foundNet) {
    return 0;
  }
  if(foundNode) {
    result[3] = atoi(start);
  } else {
    result[2] = atoi(start);
    result[3] = 0;
  }
  return 1;
}

/*
 * Returns true if the given zone is found in the given zone string. The zone string
 * is a list of zones separated by white space. Each zone is the zone number with a
 * trailing ':'. E.g. "1: 2: 3:"
 * If emptyMatchesAll is true then an empty zone string will match any zone.
 */
int IsZoneInStr(int zone, char *str, int emptyMatchesAll) {
  if(emptyMatchesAll && str[0] == '\0') {
    return 1;
  }
  while(str[0]) {
    if(atoi(str) == zone) {
      return 1;
    }
    str = FindNextWord(str);
  }
  return 0;
}
