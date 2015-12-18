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

/*
 * Re-implementations of functions in ctype.h since they don't seem to safe
 * with 8-bit characters. (E.g. isspace('Å') sometimes returns true.)
 * They are named with 'z' to avoid comflicts with functions in locale.library.
 */
int IzSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int IzDigit(char c) {
  return c >= '0' && c <= '9';
}
