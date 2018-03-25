#include <string.h>
//#include <stdio.h>
#include <math.h>
//#include <proto/exec.h>
#include "NiKomStr.h"

#include "StyleSheets.h"

int insertStyle(char *codeName, int codeNameLen, char *dst, struct StyleSheet *styleSheet) {
  struct StyleCode *styleCode = NULL, *iter;
  char name[20];
  int len;

  if(styleSheet == NULL) {
    return 0;
  }
  
  strncpy(name, codeName, min(codeNameLen, 19));
  name[codeNameLen] = '\0';

  if(strcmp(name, "reset") == 0) {
    strcpy(dst, "\x1b[0m");
    return 4;
  }
  
  ITER_EL(iter, styleSheet->codesList, codeNode, struct StyleCode *) {
    if(strcmp(name, iter->name) == 0) {
      styleCode = iter;
      break;
    }
  }
  if(styleCode == NULL) {
    return 0;
  }
  *(dst++) = 0x1b;
  *(dst++) = 0x5b;
  strcpy(dst, styleCode->ansi);
  len = strlen(styleCode->ansi);
  dst += len;
  *dst = 'm';
  return len + 3;
}

void RenderStyle(char *dst, char *src, struct StyleSheet *styleSheet) {
  char *cur, *tagStart = NULL, len;
  for(cur = src; *cur; cur++) {
    if(tagStart == NULL) {
      if(*cur == (char)0xab) {
        tagStart = cur;
      } else {
        *(dst++) = *cur;
      }
    } else {
      if(*cur == (char)0xbb) {
        dst += insertStyle(tagStart + 1, cur - tagStart - 1, dst, styleSheet);
        tagStart = NULL;
      } else if(*cur < 'a' || *cur > 'z') {
        len = cur - tagStart + 1;
        strncpy(dst, tagStart, len);
        dst += len;
        tagStart = NULL;
      }
    }
  }
  if(tagStart != NULL) {
    len = cur - tagStart + 1;
    strncpy(dst, tagStart, len);
    dst += len;
  }
  *dst = '\0';
}

/*
void testStyle(char *str, struct StyleSheet *styleSheet) {
  char buf[100];
  RenderStyle(buf, str, styleSheet);
  printf("'%s' -> '%s'\n", str, buf);
}

void main(void) {
  struct StyleSheet styleSheet;
  struct StyleCode black, white;

  NewList((struct List *)&styleSheet.codesList);
  strcpy(black.name, "black");
  strcpy(black.ansi, "32");
  strcpy(white.name, "white");
  strcpy(white.ansi, "36");
  AddTail((struct List *)&styleSheet.codesList, (struct Node *)&black);
  AddTail((struct List *)&styleSheet.codesList, (struct Node *)&white);

  testStyle("1 «black» 2 «NO» 3 «white» 4 «foo » 5 «bar", &styleSheet);
  //  testStyle("1 «black» 2", &styleSheet);
}
*/
