#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "CharacterSets.h"
#include "Languages.h"

#define EKO		1
#define EJEKO	0

extern struct System *Servermem;
extern int nodnr, inloggad;
extern char outbuffer[],inmat[], *argument;

struct Mote *getmotpek(int confId) {
  struct Mote *conf;
  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(conf->nummer == confId) {
      return conf;
    }
  }
  return NULL;
}

char *getmotnamn(int confId) {
  static char buf[40];
  struct Mote *conf = getmotpek(confId);

  if(!conf) {
    sprintf(buf, "<%s>", CATSTR(MSG_FORUMS_UNKNOWN));
    return buf;
  }
  return conf->namn;
}

struct Kommando *getkmdpek(int cmdId) {
  struct Kommando *cmd;
  ITER_EL(cmd, Servermem->kom_list, kom_node, struct Kommando *) {
    if(cmd->nummer == cmdId) {
      return cmd;
    }
  }
  return NULL;
}

int bytnodtyp(void) {
  int nr,i;
  struct NodeType *nt=NULL;

  SendString("\n\n\r%s\n\n\r", CATSTR(MSG_CHANGE_NODETYPE_HEAD_1));
  SendString(" 0: %s\n\r", CATSTR(MSG_CHANGE_NODETYPE_HEAD_2));
  for(i = 0; i < MAXNODETYPES; i++) {
    if(Servermem->nodetypes[i].nummer == 0) {
      break;
    }
    SendString("%2d: %s\n\r", Servermem->nodetypes[i].nummer, Servermem->nodetypes[i].desc);
  }
  for(;;) {
    SendString("\n\r%s ", CATSTR(MSG_COMMON_CHOICE));
    if(getstring(EKO, 2, NULL)) {
      return 1;
    }
    nr = atoi(inmat);
    if(nr < 0) {
      SendString("\n\r%s\n\r", CATSTR(MSG_CHANGE_NODETYPE_BADNUM));
    } else if(nr == 0) {
      break;
    } else if(!(nt = GetNodeType(atoi(inmat)))) {
      SendString("\n\r%s\n\r", CATSTR(MSG_CHANGE_NODETYPE_NOSUCH));
    } else {
      break;
    }
  }
  if(!nt) {
    Servermem->inne[nodnr].shell = 0;
    SendString("\n\n\r%s\n\r", CATSTR(MSG_CHANGE_NODETYPE_NOSELECT));
  } else {
    Servermem->inne[nodnr].shell = nt->nummer;
    SendString("\n\n\r%s:\n\r%s\n\n\r", CATSTR(MSG_CHANGE_NODETYPE_SELECTED), nt->desc);
  }
  return 0;
}

void dellostsay(void) {
  struct SayString *say, *tmp;
  say = Servermem->say[nodnr];
  Servermem->say[nodnr] = NULL;
  while(say) {
    tmp = say->NextSay;
    FreeMem(say, sizeof(struct SayString));
    say = tmp;
  }
}

void bytteckenset(void) {
  int showExample = FALSE;

  if(argument[0]) {
    switch(argument[0]) {
    case '1' :
      Servermem->inne[nodnr].chrset = CHRS_LATIN1;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_ISO88591));
      return;
    case '2' :
      Servermem->inne[nodnr].chrset = CHRS_CP437;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_CP437));
      return;
    case '3' :
      Servermem->inne[nodnr].chrset = CHRS_MAC;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_MAC));
      return;
    case '4' :
      Servermem->inne[nodnr].chrset = CHRS_SIS7;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_SIS7));
      return;
    case '5' :
      Servermem->inne[nodnr].chrset = CHRS_UTF8;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_UTF8));
      return;
    case '-':
      if(argument[1] == 'e' || argument[1] == 'E') {
        showExample = TRUE;
      } else {
        SendString("\n\n\r%s\n\r", CATSTR(MSG_KOM_INVALID_SWITCH));
        return;
      }
      break;
    default :
      SendStringCat("\n\n\r%s\n\r", CATSTR(MSG_CHRS_BAD), 1, 5);
      return;
    }
  }
  AskUserForCharacterSet(FALSE, showExample);
}

void SaveCurrentUser(int userId, int nodeId) {
  Servermem->inne[nodeId].senast_in = time(NULL);
  writeuser(userId, &Servermem->inne[nodeId]);
  WriteUnreadTexts(&Servermem->unreadTexts[nodeId], userId);
}
