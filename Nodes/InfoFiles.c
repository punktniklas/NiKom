#include <stdio.h>
#include "NiKomStr.h"
#include "Nodes.h"
#include "Terminal.h"

#include "InfoFiles.h"

#define EFFECTIVE_USER  (inloggad == -1 ? &g_preLoginUserData : &Servermem->userData[g_userDataSlot])

extern struct System *Servermem;
extern int nodnr, g_userDataSlot, inloggad;
extern struct User g_preLoginUserData;

char *CreateLocalizedInfoFilePath(char *fileName, char *buf) {
  sprintf(buf, "NiKom:Texter/%s/%s", Servermem->languages[EFFECTIVE_USER->language], fileName);
  return buf;
}

void SendInfoFile(char *fileName, int leastTimeStamp) {
  char buf[100];

  CreateLocalizedInfoFilePath(fileName, buf);
  if(leastTimeStamp != 0 && leastTimeStamp > getft(buf)) {
    return;
  }
  SendString("\r\n\n");
  sendfile(buf);
}

