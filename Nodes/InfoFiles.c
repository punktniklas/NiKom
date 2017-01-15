#include <stdio.h>
#include "NiKomStr.h"
#include "Terminal.h"

#include "InfoFiles.h"

extern struct System *Servermem;
extern int nodnr;

char *CreateLocalizedInfoFilePath(char *fileName, char *buf) {
  sprintf(buf, "NiKom:Texter/%s/%s", Servermem->languages[Servermem->inne[nodnr].language], fileName);
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

