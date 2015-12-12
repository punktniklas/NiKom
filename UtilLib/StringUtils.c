#include <string.h>

#include "StringUtils.h"

/*
 * Returns a pointer to the first non space character found after one or
 * more space or tab characters. Returns a pointer to the ending NUL byte if
 * not found.
 */
char *FindNextWord(char *str) {
  while(*str != ' ' && *str != '\t' && *str != '\0') {
    str++;
  }
  while(*str == ' ' || *str == '\t') {
    str++;
  }
  return str;
}

int StartsWith(char *str, char *prefix) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}
