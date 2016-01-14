#include "Stack.h"
#include "Logging.h"
#include "KOM.h"
#include "NiKomLib.h"
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "OrgMeet.h"
#include "FidoMeet.h"
#include "Brev.h"

extern struct System *Servermem;
extern int inloggad, nodnr, mote2, nodestate, senast_text_typ, senast_text_nr;
extern char *argument;
extern struct Header readhead;

void Cmd_GoConf(void) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  int parsedConfId, newConfId = -2, becomeMember;
  char buffer[121];
  struct Mote *conf;

  if(matchar(argument, Servermem->cfg.brevnamn)) {
    newConfId = -1;
  } else {
    parsedConfId = parsemot(argument);
    if(parsedConfId == -3) {
      SendString("\r\n\nSkriv: Gå <mötesnamn>\r\n\n");
      return;
    }
    if(parsedConfId == -1) {
      SendString("\r\n\nFinns inget sådant möte!\r\n\n");
      return;
    }
    newConfId = parsedConfId;

    if(!IsMemberConf(newConfId, inloggad, &Servermem->inne[nodnr])) {
      conf = getmotpek(newConfId);
      if(MayBeMemberConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {

        sprintf(buffer, "\r\n\nDu är inte medlem i mötet %s, vill du bli medlem?",
                conf->namn);
        if(GetYesOrNo(buffer, 'j', 'n', "Ja\r\n", "Nej\r\n", TRUE, &becomeMember)) {
          return;
        }
        if(!becomeMember) {
          return;
        }
        BAMSET(Servermem->inne[nodnr].motmed, parsedConfId);
        if(conf->type == MOTE_ORGINAL) {
          unreadTexts->lowestPossibleUnreadText[parsedConfId] = 0;
        }
        else if(conf->type == MOTE_FIDO) {
          unreadTexts->lowestPossibleUnreadText[parsedConfId] = conf->lowtext;
        }
      } else {
        SendString("\r\n\nDu är inte medlem och har inte rätt att "
                   "gå med i %s.\r\n\n", conf->namn);
        return;
      }
    }
  }

  if(newConfId != -2) {
    GoConf(newConfId);
  }
}


void Cmd_NextText(void) {
  struct Mote *conf;
  if(mote2 == -1) {
    NextMail();
    return;
  }
  conf = getmotpek(mote2);
  if(conf == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Couldn't find conference %d in Cmd_NextText.", mote2);
    DisplayInternalError();
  }
  switch(conf->type) {
  case MOTE_ORGINAL:
    NextTextInOrgConf();
    break;
  case MOTE_FIDO:
    NextTextInFidoConf(conf);
    break;
  default:
    LogEvent(SYSTEM_LOG, ERROR, "Unknown type for conf %d: %d.", mote2, conf->type);
    DisplayInternalError();
  }
}

void Cmd_NextReply(void) {
  NextReplyInOrgConf();
}

void Cmd_NextConf(void) {
  int newConfId;

  newConfId = FindNextUnreadConf(mote2);
  if(newConfId == -2) {
    SendString("\n\n\rFinns inget mer möte med olästa texter!\n\r");
    return;
  }
  mote2 = newConfId;
  StackClear(g_unreadRepliesStack);
  var(mote2);
}

void Cmd_Logout(void) {
  nodestate |= NIKSTATE_USERLOGOUT;
}

void Cmd_ReLogin(void) {
  nodestate |= NIKSTATE_USERLOGOUT | NIKSTATE_RELOGIN;
}

void Cmd_SkipReplies(void) {
  struct Stack *skipStack = CreateStack();
  struct Header skipHeader;
  int textId, i, cnt = 0;

  if(senast_text_typ == 0) {
    SendString("\r\n\nDu har inte läst någon text ännu.\r\n");
    return;
  }
  if(senast_text_typ != TEXT) {
    SendString("\r\n\nDu kan bara använda 'Hoppa Över' på texter i vanliga möten.\r\n");
    return;
  }
  
  StackPush(skipStack, senast_text_nr);
  while(StackSize(skipStack) > 0) {
    textId = StackPop(skipStack);
    if(IsTextUnread(textId, &Servermem->unreadTexts[nodnr])) {
      cnt++;
    }
    ChangeUnreadTextStatus(textId, 0, &Servermem->unreadTexts[nodnr]);
    if(readtexthead(textId, &skipHeader)) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Couldn't read text %d in Cmd_SkipReplies()", textId);
      DisplayInternalError();
      break;
    }
    for(i = 0; i < MAXKOM; i++) {
      if(skipHeader.kom_i[i] == -1) {
        break;
      }
      StackPush(skipStack, skipHeader.kom_i[i]);
    }
  }
  DeleteStack(skipStack);
  while(StackSize(g_unreadRepliesStack) > 0) {
    if(!IsTextUnread(StackPeek(g_unreadRepliesStack),
                    &Servermem->unreadTexts[nodnr])) {
      StackPop(g_unreadRepliesStack);
    } else {
      break;
    }
  }
  SendString("\r\n\nDu hoppade över %d texter.\r\n", cnt);
}
