#include "NiKomStr.h"
#include "NiKomLib.h"
#include "Terminal.h"

// Temporary hack to avoid including NiKomFuncs.h
char gettekn(void);
int carrierdropped(void);

extern struct System *Servermem;
extern int nodnr;

int AskUserForCharacterSet(int forceChoice) {
  char tkn;
  int chrsetbup;

  chrsetbup = Servermem->inne[nodnr].chrset;
  if(Servermem->nodtyp[nodnr] == NODCON) {
    SendString("\n\n\r*** OBS! Du kör på en CON-nod, alla teckenset kommer "
               "se likadana ut!! ***");
  }
  SendString("\n\n\rDessa teckenset finns. "
             "Ta det alternativ vars svenska tecken ser bra ut.\n\r");
  if(chrsetbup != 0) {
    SendString("* markerar nuvarande val.\n\n\r");
  }
  SendString("  Nr Namn                              Exempel\n\r");
  SendString("----------------------------------------------\n\r");

  Servermem->inne[nodnr].chrset = CHRS_LATIN1;
  SendString("%c 1: ISO 8859-1 (ISO Latin 1)          åäö ÅÄÖ\n\r",
             chrsetbup == CHRS_LATIN1 ? '*' : ' ');
  Servermem->inne[nodnr].chrset = CHRS_CP437;
  SendString("%c 2: IBM CodePage 437 (PC8)            åäö ÅÄÖ\n\r",
             chrsetbup == CHRS_CP437 ? '*' : ' ');
  Servermem->inne[nodnr].chrset = CHRS_MAC;
  SendString("%c 3: Macintosh                         åäö ÅÄÖ\n\r",
             chrsetbup == CHRS_MAC ? '*' : ' ');
  Servermem->inne[nodnr].chrset = CHRS_SIS7;
  SendString("%c 4: SIS-7 (SF7, måsvingar)            åäö ÅÄÖ\n\r",
             chrsetbup == CHRS_SIS7 ? '*' : ' ');

  Servermem->inne[nodnr].chrset = chrsetbup;
  SendString("\n\rOm du bara ser ett alternativ ovan beror det av att ditt terminalprogram\n\r");
  SendString("strippar den 8:e biten i inkommande tecken. Om exemplet ser bra ut, ta\n\r");
  SendString("SIS-7 (dvs, tryck 4).\n\r");
  SendString("\n\rVal: ");
  for(;;) {
    tkn = gettekn();
    if(carrierdropped()) return(1);
    switch(tkn) {
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
      SendString("Macintosh\n\r");
      return 0;
    case '4' :
      Servermem->inne[nodnr].chrset = CHRS_SIS7;
      SendString("SIS-7\n\r");
      return 0;
    case '\r' : case '\n' :
      if(!forceChoice) {
        SendString("Avbryter\n\r");
        return 0;
      }
    }
  }
}
