#include "NiKomStr.h"
#include "NiKomLib.h"
#include "Terminal.h"

extern struct System *Servermem;
extern int nodnr;

int AskUserForCharacterSet(int forceChoice, int showExamples) {
  char *example;
  int chrsetbup, ch;

  chrsetbup = Servermem->inne[nodnr].chrset;
  if(Servermem->nodtyp[nodnr] == NODCON || showExamples) {
    SendString("\n\n\r*** OBS! Du kör på en CON-nod, exemplen för alla teckenset\n\r");
    SendString("***      kommer se likadana ut!!");
  }
  SendString("\n\n\rDessa teckenset finns.\n\r");
  if(showExamples) {
    SendString("Välj det alternativ vars svenska tecken ser bra ut.\n\r");
  }
  SendString("\n\r");
  if(chrsetbup != 0) {
    SendString("* markerar nuvarande val.\n\n\r");
  }

  if(showExamples) {
    SendString("  Nr Namn                                  Exempel\n\r");
    SendString("--------------------------------------------------\n\r");
  } else {
    SendString("  Nr Namn\n\r");
    SendString("-----------------------------------------\n\r");
  }

  example = showExamples ? "åäö ÅÄÖ" : "";

  Servermem->inne[nodnr].chrset = CHRS_LATIN1;
  SendString("%c 1: ISO 8859-1 (ISO Latin 1)              %s\n\r",
             chrsetbup == CHRS_LATIN1 ? '*' : ' ', example);
  Servermem->inne[nodnr].chrset = CHRS_CP437;
  SendString("%c 2: IBM CodePage 437 (PC8)                %s\n\r",
             chrsetbup == CHRS_CP437 ? '*' : ' ', example);
  Servermem->inne[nodnr].chrset = CHRS_MAC;
  SendString("%c 3: Mac OS Roman (gamla Mac, innan OS X)  %s\n\r",
             chrsetbup == CHRS_MAC ? '*' : ' ', example);
  Servermem->inne[nodnr].chrset = CHRS_SIS7;
  SendString("%c 4: SIS-7 (SF7, måsvingar)                %s\n\r",
             chrsetbup == CHRS_SIS7 ? '*' : ' ', example);

  Servermem->inne[nodnr].chrset = chrsetbup;
  SendString("\n\rVal: ");
  for(;;) {
    ch = GetChar();
    switch(ch) {
    case GETCHAR_LOGOUT:
      return 1;
    case '1' :
      Servermem->inne[nodnr].chrset = CHRS_LATIN1;
      SendString("ISO 8859-1\n\r");
      return 0;
    case '2' :
      Servermem->inne[nodnr].chrset = CHRS_CP437;
      SendString("IBM CodePage 437\n\r");
      return 0;
    case '3' :
      Servermem->inne[nodnr].chrset = CHRS_MAC;
      SendString("Mac OS Roman\n\r");
      return 0;
    case '4' :
      Servermem->inne[nodnr].chrset = CHRS_SIS7;
      SendString("SIS-7\n\r");
      return 0;
    case GETCHAR_RETURN :
      if(!forceChoice) {
        SendString("Avbryter\n\r");
        return 0;
      }
    }
  }
}
