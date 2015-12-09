/*
 * Returns a pointer to the first non space character found after one or
 * more space characters. Returns a pointer to the ending NUL byte if
 * not found.
 */
char *FindNextWord(char *str) {
  while(*str != ' ' && *str != '\0') {
    str++;
  }
  while(*str == ' ') {
    str++;
  }
  return str;
}
