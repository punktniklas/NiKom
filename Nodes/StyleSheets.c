#include <string.h>
//#include <stdio.h>
#include <math.h>
//#include <proto/exec.h>
#include "NiKomStr.h"
#include "Terminal.h"
#include "Languages.h"
#include "Nodes.h"
#include "Trie.h"

#include "StyleSheets.h"

extern struct System *Servermem;
extern int g_userDataSlot;

int insertStyle(char *codeName, int codeNameLen, char *dst, struct StyleSheet *styleSheet) {
  struct StyleCode *styleCode = NULL;
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
  
  if((styleCode = TrieGet(name, styleSheet->codes)) == NULL) {
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

int RenderLength(char *str) {
  int cnt = 0, taglen = 0;
  for(; *str != '\0'; str++) {
    if(taglen > 0) {
      if(*str == (char)0xbb) {
        taglen = 0;
      } else if(*str >= 'a' && *str <= 'z') {
        taglen++;
      } else {
        cnt += taglen + 1;
        taglen = 0;
      }
    } else {
      if(*str == (char)0xab) {
        taglen = 1;
      } else if(*str != '\r' && *str != '\n') {
        cnt++;
      }
    }
  }
  return cnt;
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

void Cmd_ChangeStyleSheet(void) {
  int i, newStyle, ch;
  SendString("\n\n\r%s\n\r", CATSTR(MSG_STYLES_AVAILABLE));
  for(i = 0 ; i < MAXSTYLESHEET; i++) {
    if(Servermem->cfg->styleSheets[i].name[0] == '\0') {
      continue;
    }
    SendString("%c %d: %s\r\n",
               CURRENT_USER->styleSheet == i ? '*' : ' ',
               i, Servermem->cfg->styleSheets[i].name);
  }
  SendString("\n\n\r%s ", CATSTR(MSG_COMMON_CHOICE));
  for(;;) {
    ch = GetChar();
    if(ch == GETCHAR_LOGOUT) {
      return;
    }
    if(ch == GETCHAR_RETURN) {
        SendString("%s\n\r", CATSTR(MSG_COMMON_ABORTING));
        return;
    }
    if(ch < '0' || ch > '7') {
      continue;
    }
    newStyle = ch - '0';
    if(Servermem->cfg->styleSheets[newStyle].name[0] == '\0') {
      continue;
    }
    CURRENT_USER->styleSheet = newStyle;
    SendString("%s\n\r", Servermem->cfg->styleSheets[newStyle].name);
    break;
  }
}
