#include <exec/types.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

#include "NiKomLib.h"

struct Library *NiKomBase;

void main(int argc, char *argv[]) {
  int nodnr, state = 0, i;
  if(argc < 2) {
    puts("Usage: SetNodeState <nodenr> [SERCLOSED] [NOANSWER] [LOGOUT]");
    return;
  }
  nodnr = atoi(argv[1]);
  for(i = 2; i < argc; i++) {
    if(!stricmp(argv[i],"SERCLOSED")) {
      state |= NIKSTATE_CLOSESER;
    }
    if(!stricmp(argv[i],"NOANSWER")) {
      state |= NIKSTATE_NOANSWER;
    }
    if(!stricmp(argv[i],"LOGOUT")) {
      state |= NIKSTATE_AUTOLOGOUT;
    }
  }
  if(NiKomBase = OpenLibrary("nikom.library",0)) {
    if(SetNodeState(nodnr,state)) {
      puts("Suceeded");
    } else {
      puts("Failed");
    }
    CloseLibrary(NiKomBase);
  } else {
    puts("Kunde inte öppna nikom.library");
  }
}
