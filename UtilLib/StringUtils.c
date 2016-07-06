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

static const int cmpchars[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  ' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
  '@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '[',  '\\', ']',  '^',  '_',
  '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  ' ',  '¡',  '¢',  '£',  '¤',  '¥',  '|',  '§',  '¨',  '©',  'ª',  '«',  '¬',  '-',  '®',  '¯',
  '°',  '±',  '2',  '3',  '\'',  'µ',  '¶',  '.',  '¸', '1',  'º',  '»',  '¼',  '½',  '¾',  '?',
  'a',  'a',  'a',  'a',  'ä',  'å',  'æ',  'c',  'e',  'e',  'e',  'e',  'i',  'i',  'i',  'i',
  'd',  'n',  'o',  'o',  'o',  'o',  'ö',  'x',  'ø',  'u',  'u',  'u',  'ü',  'y',  'þ',  'ß',
  'a',  'a',  'a',  'a',  'ä',  'å',  'æ',  'c',  'e',  'e',  'e',  'e',  'i',  'i',  'i',  'i',
  'd',  'n',  'o',  'o',  'o',  'o',  'ö',  '÷',  'ø',  'u',  'u',  'u',  'ü',  'y',  'þ',  'y'
};

/*
 * Returns the position of needle in hay as found with lenient comparison. (a = A, Ö = ö,
 * e = é etc) Returns -1 of needle is not found.
 */
int LenientFindSubString(char *hay, char *needle) {
  char *hayptr = hay, *tmphayptr, *needleptr;
  while(*hayptr != '\0') {
    needleptr = needle;
    tmphayptr = hayptr;
    while(*needleptr != '\0') {
      if(cmpchars[(unsigned char)*tmphayptr] != cmpchars[(unsigned char)*needleptr]) {
        break;
      }
      tmphayptr++;
      needleptr++;
    }
    if(*needleptr == '\0') {
      return hayptr - hay;
    }
    hayptr++;
  }
  return -1;
}
