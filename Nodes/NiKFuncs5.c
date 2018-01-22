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
#include "Nodes.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "CharacterSets.h"
#include "Languages.h"

#define EKO		1
#define EJEKO	0

extern struct System *Servermem;
extern int nodnr, inloggad, g_userDataSlot;
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
  ITER_EL(cmd, Servermem->cfg->kom_list, kom_node, struct Kommando *) {
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
    if(Servermem->cfg->nodetypes[i].nummer == 0) {
      break;
    }
    SendString("%2d: %s\n\r", Servermem->cfg->nodetypes[i].nummer, Servermem->cfg->nodetypes[i].desc);
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
    CURRENT_USER->shell = 0;
    SendString("\n\n\r%s\n\r", CATSTR(MSG_CHANGE_NODETYPE_NOSELECT));
  } else {
    CURRENT_USER->shell = nt->nummer;
    SendString("\n\n\r%s:\n\r%s\n\n\r", CATSTR(MSG_CHANGE_NODETYPE_SELECTED), nt->desc);
  }
  return 0;
}

void bytteckenset(void) {
  int showExample = FALSE;

  if(argument[0]) {
    switch(argument[0]) {
    case '1' :
      CURRENT_USER->chrset = CHRS_LATIN1;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_ISO88591));
      return;
    case '2' :
      CURRENT_USER->chrset = CHRS_CP437;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_CP437));
      return;
    case '3' :
      CURRENT_USER->chrset = CHRS_MAC;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_MAC));
      return;
    case '4' :
      CURRENT_USER->chrset = CHRS_SIS7;
      SendString("\n\n\r%s: %s\n\r", CATSTR(MSG_CHRS_NEW), CATSTR(MSG_CHRS_SIS7));
      return;
    case '5' :
      CURRENT_USER->chrset = CHRS_UTF8;
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
  AskUserForCharacterSet(CURRENT_USER, FALSE, showExample);
}
