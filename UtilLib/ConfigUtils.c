#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "StringUtils.h"

#include "ConfigUtils.h"

char *FindStringCfgValue(char *str) {
  char *tmp;
  int lastpos;

  tmp = strchr(str, '=');
  if(tmp == NULL) {
    printf("Invalid config line, no '=' sign found: %s\n", str);
    return NULL;
  }
  tmp++;
  if(IzSpace(tmp[0])) {
    tmp = FindNextWord(tmp);
  }
  if(IzSpace(tmp[0]) || tmp[0] == '\0') {
    printf("Invalid config line, no value after '=': %s\n", str);
    return NULL;
  }

  lastpos = strlen(tmp) - 1;
  while(IzSpace(tmp[lastpos])) {
    tmp[lastpos--] = '\0';
  }
  return tmp;
}

int GetStringCfgValue(char *str, char *dest, int len) {
  char *valstr;

  if((valstr = FindStringCfgValue(str)) == NULL) {
    return 0;
  }
  strncpy(dest, valstr, len);
  dest[len] = '\0';
  return 1;
}

int GetLongCfgValue(char *str, long *value, int lineCnt) {
  char *valstr, *tmp;

  if((valstr = FindStringCfgValue(str)) == NULL) {
    return 0;
  }
  for(tmp = valstr; *tmp != '\0'; tmp++) {
    if(!IzDigit(*tmp) && *tmp != '-') {
      printf("Invalid config line %d, value must be only digits: %s\n", lineCnt, str);
      return 0;
    }
  }
  *value = atoi(valstr);
  return 1;
}

int GetCharCfgValue(char *str, char *value, int lineCnt) {
  long longval;
  if(!GetLongCfgValue(str, &longval, lineCnt)) {
    return 0;
  }
  if(longval < -128 || longval > 127) {
    printf("Invalid config line %d, value must be between -128 and 127: %s\n", lineCnt, str);
    return 0;
  }
  *value = longval;
  return 1;
}

int GetShortCfgValue(char *str, short *value, int lineCnt) {
  long longval;
  if(!GetLongCfgValue(str, &longval, lineCnt)) {
    return 0;
  }
  if(longval < SHRT_MIN || longval > SHRT_MAX) {
    printf("Invalid config line %d, value must be between %d and %d: %s\n",
           lineCnt, SHRT_MIN, SHRT_MAX, str);
    return 0;
  }
  *value = longval;
  return 1;
}

int GetBoolCfgFlag(char *str, long *flagfield, long flag, int lineCnt) {
  char *valstr;
  int boolvalue;

  if((valstr = FindStringCfgValue(str)) == NULL) {
    return 0;
  }
  if(stricmp(valstr, "YES") == 0 || stricmp(valstr, "JA") == 0) {
    boolvalue = 1;
  } else if(stricmp(valstr, "NO") == 0 || stricmp(valstr, "NEJ") == 0) {
    boolvalue = 0;
  } else {
    printf("Invalid config line %d, invalid boolean value: %s\n", lineCnt, str);
    return 0;
  }
  if(boolvalue) {
    *flagfield |= flag;
  } else {
    *flagfield &= ~flag;
  }
  return 1;
}

