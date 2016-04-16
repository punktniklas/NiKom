#include <time.h>
#include <limits.h>
#include <string.h>
#include "NiKomLib.h"
#include "NiKomStr.h"
#include "Stack.h"
#include "Logging.h"
#include "StringUtils.h"
#include "OrgMeet.h"
#include "FidoMeet.h"
#include "Brev.h"
#include "Terminal.h"
#include "Cmd_Kom.h"
#include "Cmd_Users.h"
#include "Cmd_Conf.h"
#include "NiKFiles.h"
#include "NiKversion.h"
#include "BasicIO.h"
#include "NiKomFuncs.h"

#include "KOM.h"

#define CMD_NEXTREPLY 211
#define CMD_NEXTTEXT  210
#define CMD_NEXTCONF  221
#define CMD_SEETIME   306
#define CMD_GOMAIL    222
#define MAILBOX_CONFID -1

struct Stack *g_unreadRepliesStack;

extern struct System *Servermem;
extern int inloggad, nodnr, nodestate, mote2;
extern long logintime, extratime;
extern char inmat[], *argument;

struct Kommando internalGoMailCommand = {
  { NULL, NULL }, CMD_GOMAIL, 0, 0, 0, 0, 0, "Gå Brevlådan",
  0, 2, 0, NULL, NULL, 0, NULL, NULL, NULL
};

int hasUnreadInConf(int confId) {
  struct Mote *conf;
  if(confId == MAILBOX_CONFID) {
    return HasUnreadMail();
  }
  if(!IsMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
    return 0;
  }
  if((conf = getmotpek(confId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't find struct for confId %d in hasUnreadConf()", confId);
    return 0;
  }
  switch(conf->type) {
  case MOTE_ORGINAL:
    return HasUnreadInOrgConf(confId);
  case MOTE_FIDO:
    return HasUnreadInFidoConf(conf);
  default:
    LogEvent(SYSTEM_LOG, ERROR,
             "Undefined type %d for conf %d in hasUnreadConf()", conf->type, confId);
  }
  return 0;
}

/*
 * Returns the id of the next conference with unread texts, starting the search
 * from currentConfId. Returns -2 if there is no conference with unread texts.
 */
int FindNextUnreadConf(int currentConfId) {
  struct Mote *conf;
  int tmpConfId = 0;
  if(currentConfId == MAILBOX_CONFID) {
    conf = (struct Mote *)Servermem->mot_list.mlh_Head;
  } else {
    conf = getmotpek(currentConfId);
    conf = (struct Mote *)conf->mot_node.mln_Succ;
    if(conf->mot_node.mln_Succ == NULL) {
      conf = (struct Mote *)Servermem->mot_list.mlh_Head;
    }
  }
  tmpConfId = conf->nummer;
  while(tmpConfId != currentConfId) {
    if(!(conf->status & SUPERHEMLIGT) && hasUnreadInConf(tmpConfId)) {
      return tmpConfId;
    }
    conf = (struct Mote *)conf->mot_node.mln_Succ;
    if(conf->mot_node.mln_Succ == NULL) {
      if(currentConfId == MAILBOX_CONFID) {
        return -2;
      }
      conf = (struct Mote *)Servermem->mot_list.mlh_Head;
    }
    tmpConfId = conf->nummer;
  }
  return -2;
}

int isUserOutOfTime(void) {
  int limitSeconds, secondsLoggedIn;
  limitSeconds = 60 * Servermem->cfg.maxtid[Servermem->inne[nodnr].status]
    + extratime;
  if(Servermem->cfg.maxtid[Servermem->inne[nodnr].status] != 0) {
    secondsLoggedIn = time(NULL) - logintime;
    if(secondsLoggedIn > limitSeconds) {
      sendfile("NiKom:Texter/TidenSlut.txt");
      nodestate = NIKSTATE_OUTOFTIME;
      return -1;
    }
    return (limitSeconds - secondsLoggedIn) / 60;
  }
  return INT_MAX;
}

struct Kommando *getCommandToExecute(int defaultCmd) {
  int parseRes, cmdId;
  struct Alias *alias;
  static badCommandCnt = 0;
  static char aliasbuf[1081];

  if(GetString(999, NULL)) {
    return NULL;
  }
  if(inmat[0]=='.' || inmat[0]==' ') {
    return NULL;
  }
  if(inmat[0] && (alias = parsealias(inmat))) {
    strcpy(aliasbuf, alias->blirtill);
    strcat(aliasbuf, " ");
    strncat(aliasbuf, FindNextWord(inmat),980);
    strcpy(inmat,aliasbuf);
  }
  
  parseRes = parse(inmat);
  switch(parseRes) {
  case -1:
    SendString("\r\n\nFelaktigt kommando!\r\n");
    if(++badCommandCnt >= 2 && !(Servermem->inne[nodnr].flaggor & INGENHELP)) {
      sendfile("NiKom:Texter/2fel.txt");
    }
    return NULL;

  case -2:
    return NULL; // Ambigous command

  case -3:
    cmdId = defaultCmd;
    break;

  case -4:
    SendString("\r\n\nDu har ingen rätt att utföra det kommandot!\r\n");
    if(Servermem->cfg.ar.noright) {
      sendautorexx(Servermem->cfg.ar.noright);
    }
    return NULL;

  case -5:
    SendString("\r\n\nFelaktigt lösen!\r\n");
    return NULL;

  default:
    cmdId = parseRes;
    badCommandCnt = 0;
    break;
  }
  if(cmdId == CMD_GOMAIL) {
    return &internalGoMailCommand;
  }
  return getkmdpek(cmdId);
}

void DoExecuteCommand(struct Kommando *cmd) {
  switch(cmd->nummer) {
  case 201: Cmd_GoConf(); break; // ga(argument);
  case 210: Cmd_NextText(); break;
  case 211: Cmd_NextReply(); break;
  case 221: Cmd_NextConf(); break;
  case 222: GoConf(MAILBOX_CONFID); break;
  case 301: Cmd_Logout(); break;
  case 101: listmot(argument); break;
  case 102: Cmd_ListUsers(); break;
  case 103: listmed(); break;
  case 104: sendfile("NiKom:Texter/ListaKommandon.txt"); break;
  case 105: listratt(); break;
  case 106: listasenaste(); break;
  case 107: listnyheter(); break;
  case 108: listaarende(); break;
  case 109: listflagg(); break;
  case 111: listarea(); break;
  case 112: listnyckel(); break;
  case 113: listfiler(); break;
  case 114: listagrupper(); break;
  case 115: listgruppmed(); break;
  case 116: listabrev(); break;
  case 202: skriv(); break;
  case 203: Cmd_Reply(); break;
  case 204: personlig(); break;
  case 205: skickabrev(); break;
  case 206: igen(); break;
  case 207: atersekom(); break;
  case 208: medlem(argument); break;
  case 209: uttrad(argument); break;
  case 212: Cmd_Read(); break;
  case 213: endast(); break;
  case 214: Cmd_SkipReplies(); break;
  case 215: addratt(); break;
  case 216: subratt(); break;
  case 217: radtext(); break;
  case 218: skapmot(); break;
  case 219: radmot(); break;
  case 220: var(mote2); break;
  case 223: andmot(); break;
  case 224: radbrev(); break;
  case 225: rensatexter(); break;
  case 226: rensabrev(); break;
  case 227: gamlatexter(); break;
  case 228: gamlabrev(); break;
  case 229: dumpatext(); break;
  case 231: movetext(); break;
  case 232: motesstatus(); break;
  case 233: hoppaarende(); break;
  case 234: flyttagren(); break;
  case 235: Cmd_FootNote(); break;
  case 302: sendfile("NiKom:Texter/Help.txt"); break;
  case 303: Cmd_ChangeUser(); break;
  case 304: slaav(); break;
  case 305: slapa(); break;
  case 306: tiden(); break;
  case 307: ropa(); break;
  case 308: Cmd_Status(); break;
  case 309: Cmd_DeleteUser(); break;
  case 310: vilka(); break;
  case 311: visainfo(); break;
  case 312: getconfig(); break;
  case 313: writeinfo(); break;
  case 314: sag(); break;
  case 315: skrivlapp(); break;
  case 316: radlapp(); break;
  case 317: grab(); break;
  case 318: skapagrupp(); break;
  case 319: andragrupp(); break;
  case 320: raderagrupp(); break;
  case 321: adderagruppmedlem(); break;
  case 322: subtraheragruppmedlem(); break;
  case 323: DisplayVersionInfo(); break;
  case 324: alias(); break;
  case 325: Cmd_ReLogin(); break; // nodestate |= NIKSTATE_RELOGIN; return(-3); }
  case 326: bytnodtyp(); break;
  case 327: bytteckenset(); break;
  case 328: SaveCurrentUser(inloggad, nodnr); break;
  case 401: bytarea(); break;
  case 402: filinfo(); break;
  case 403: upload(); break;
  case 404: download(); break;
  case 405: Cmd_CreateArea(); break;
  case 406: radarea(); break;
  case 407: andraarea(); break;
  case 408: skapafil(); break;
  case 409: radfil(); break;
  case 410: andrafil(); break;
  case 411: lagrafil(); break;
  case 412: flyttafil(); break;
  case 413: sokfil(); break;
  case 414: filstatus(); break;
  case 415: typefil(); break;
  case 416: nyafiler(); break;
  case 417: validerafil(); break;
  default:
    if(cmd->nummer >= 500) {
      sendrexx(cmd->nummer);
    } else {
      LogEvent(SYSTEM_LOG, ERROR,
               "Trying to execute undefined command %d", cmd->nummer);
      DisplayInternalError();
    }
  }
}

void executeCommand(struct Kommando *cmd) {
  int afterRexxScript;
  if(cmd->before) {
    sendautorexx(cmd->before);
  }
  if(cmd->logstr[0]) {
    LogEvent(USAGE_LOG, INFO, "%s %s", getusername(inloggad), cmd->logstr);
  }
  if(cmd->vilkainfo[0]) {
    Servermem->action[nodnr] = GORNGTANNAT;
    Servermem->vilkastr[nodnr] = cmd->vilkainfo;
  }
  // Save 'after' in case the command to execute is to reload the config and
  // cmd is not a valid pointer anymore when DoExecuteCommand() returns.
  afterRexxScript = cmd->after; 
  DoExecuteCommand(cmd);
  if(afterRexxScript) {
    sendautorexx(afterRexxScript);
  }
}

void displayPrompt(int defaultCmd) {
  int minutesLeft;
  char *cmdStr, goMailStr[50];
  struct Kommando *cmd;

  if(Servermem->say[nodnr]) {
    displaysay();
  }
  if((minutesLeft = isUserOutOfTime()) == -1) {
    return;
  }

  Servermem->idletime[nodnr] = time(NULL);
  Servermem->action[nodnr] = LASER;
  Servermem->varmote[nodnr] = mote2;
  switch(defaultCmd) {
  case CMD_NEXTCONF:
    if(Servermem->cfg.ar.nextmeet) {
      sendautorexx(Servermem->cfg.ar.nextmeet);
    }
    cmdStr = "(Gå till) nästa möte";
    break;
  case CMD_NEXTTEXT:
    if(mote2 == MAILBOX_CONFID) {
      if(Servermem->cfg.ar.nextletter) {
        sendautorexx(Servermem->cfg.ar.nextletter);
      }
      cmdStr = "(Läsa) nästa brev";
    } else {
      if(Servermem->cfg.ar.nexttext) {
        sendautorexx(Servermem->cfg.ar.nexttext);
      }
      cmdStr = "(Läsa) nästa text";
    }
    break;
  case CMD_NEXTREPLY:
    if(Servermem->cfg.ar.nextkom) {
      sendautorexx(Servermem->cfg.ar.nextkom);
    }
    cmdStr = "(Läsa) nästa kommentar";
    break;
  case CMD_SEETIME:
    Servermem->action[nodnr] = INGET;
    if(Servermem->cfg.ar.setid) {
      sendautorexx(Servermem->cfg.ar.setid);
    }
    cmdStr = "(Se) tiden";
    break;
  case CMD_GOMAIL:
    if(Servermem->cfg.ar.nextmeet) {
      sendautorexx(Servermem->cfg.ar.nextmeet);
    }
    sprintf(goMailStr, "Gå (till) %s", Servermem->cfg.brevnamn);
    cmdStr = goMailStr;
    break;
  default:
    cmdStr = "*** Undefined default command ***";
  }
  if(minutesLeft > 4) {
    SendString("\r\n%s %s ", cmdStr, Servermem->inne[nodnr].prompt);
  } else {
    SendString("\r\n%s (%d) %s ", cmdStr, minutesLeft, Servermem->inne[nodnr].prompt);
  }

  if((cmd = getCommandToExecute(defaultCmd)) == NULL) {
    return;
  }
  executeCommand(cmd);
}

int shouldLogout(void) {
  return (nodestate & (NIKSTATE_USERLOGOUT | NIKSTATE_OUTOFTIME))
    || ImmediateLogout();
}

void KomLoop(void) {
  int defaultCmd;
  g_unreadRepliesStack = CreateStack();

  for(;;) {
    if(StackSize(g_unreadRepliesStack) > 0) {
      defaultCmd = CMD_NEXTREPLY;
    } else if(hasUnreadInConf(mote2)) {
      defaultCmd = CMD_NEXTTEXT;
    } else if(HasUnreadMail()) {
      defaultCmd = CMD_GOMAIL;
    } else if(FindNextUnreadConf(mote2) >= 0) {
      defaultCmd = CMD_NEXTCONF;
    } else {
      defaultCmd = CMD_SEETIME;
    }

    displayPrompt(defaultCmd);
    if(shouldLogout()) {
      break;
    }
  }

  DeleteStack(g_unreadRepliesStack);
}

void GoConf(int confId) {
  mote2 = confId;
  StackClear(g_unreadRepliesStack);
  var(mote2);
}
