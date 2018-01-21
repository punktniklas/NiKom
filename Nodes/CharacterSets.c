#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomLib.h"
#include "Terminal.h"
#include "Languages.h"

extern struct System *Servermem;
extern int nodnr;

int AskUserForCharacterSet(struct User *user, int forceChoice, int showExamples) {
  char *example;
  int chrsetbup, ch;

  chrsetbup = user->chrset;
  if(Servermem->nodeInfo[nodnr].nodeType == NODCON || showExamples) {
    SendString("\n\n\r*** %s\n\r", CATSTR(MSG_CHRS_CON_WARNING_1));
    SendString("*** %s", CATSTR(MSG_CHRS_CON_WARNING_2));
  }
  SendString("\n\n\r%s\n\r", CATSTR(MSG_CHRS_AVAILABLE));
  if(showExamples) {
    SendString("%s\n\r", CATSTR(MSG_CHARS_EXAMPLE_DESC));
  }
  SendString("\n\r");
  if(chrsetbup != 0) {
    SendString("%s\n\n\r", CATSTR(MSG_CHRS_CURRENT_CHOICE));
  }

  if(showExamples) {
    SendString("  %s\n\r", CATSTR(MSG_CHRS_HEAD_WITH_EXAMPLE));
    SendString("----------------------------------------------------------\n\r");
  } else {
    SendString("  %s\n\r", CATSTR(MSG_CHRS_HEAD_WO_EXAMPLE));
    SendString("-------------------------------------------------\n\r");
  }

  example = showExamples ? "åäö ÅÄÖ" : "";

  user->chrset = CHRS_LATIN1;
  SendString("%c 1: %-45s %s\n\r",
             chrsetbup == CHRS_LATIN1 ? '*' : ' ', CATSTR(MSG_CHRS_ISO88591), example);
  user->chrset = CHRS_CP437;
  SendString("%c 2: %-45s %s\n\r",
             chrsetbup == CHRS_CP437 ? '*' : ' ', CATSTR(MSG_CHRS_CP437), example);
  user->chrset = CHRS_MAC;
  SendString("%c 3: %-45s %s\n\r",
             chrsetbup == CHRS_MAC ? '*' : ' ', CATSTR(MSG_CHRS_MAC), example);
  user->chrset = CHRS_SIS7;
  SendString("%c 4: %-45s %s\n\r",
             chrsetbup == CHRS_SIS7 ? '*' : ' ', CATSTR(MSG_CHRS_SIS7), example);
  user->chrset = CHRS_UTF8;
  SendString("%c 5: %-45s %s\n\r",
             chrsetbup == CHRS_UTF8 ? '*' : ' ', CATSTR(MSG_CHRS_UTF8), example);

  user->chrset = chrsetbup;
  SendString("\n\r%s ", CATSTR(MSG_COMMON_CHOICE));
  for(;;) {
    ch = GetChar();
    switch(ch) {
    case GETCHAR_LOGOUT:
      return 1;
    case '1' :
      user->chrset = CHRS_LATIN1;
      SendString("%s\n\r", CATSTR(MSG_CHRS_ISO88591));
      return 0;
    case '2' :
      user->chrset = CHRS_CP437;
      SendString("%s\n\r", CATSTR(MSG_CHRS_CP437));
      return 0;
    case '3' :
      user->chrset = CHRS_MAC;
      SendString("%s\n\r", CATSTR(MSG_CHRS_MAC));
      return 0;
    case '4' :
      user->chrset = CHRS_SIS7;
      SendString("%s\n\r", CATSTR(MSG_CHRS_SIS7));
      return 0;
    case '5' :
      user->chrset = CHRS_UTF8;
      SendString("%s\n\r", CATSTR(MSG_CHRS_UTF8));
      return 0;
    case GETCHAR_RETURN :
      if(!forceChoice) {
        SendString("%s\n\r", CATSTR(MSG_COMMON_ABORTING));
        return 0;
      }
    }
  }
}
