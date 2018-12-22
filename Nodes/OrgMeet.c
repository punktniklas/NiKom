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
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Stack.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "KOM.h"
#include "ConfCommon.h"
#include "ConfHeaderExtensions.h"
#include "Languages.h"
#include "StyleSheets.h"
#include "StringUtils.h"
#include "BasicIO.h"

#include "OrgMeet.h"

#define EKO		1
#define KOM		1
#define EJKOM	0

extern struct System *Servermem;
extern char outbuffer[],*argument,inmat[];
extern int nodnr,senast_text_typ,senast_text_nr,senast_text_mote,nu_skrivs,inloggad,
  rad,mote2, senast_text_reply_to;
extern int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf, g_userDataSlot;
extern struct Header readhead,sparhead;
extern struct Inloggning Statstr;
extern struct MinList edit_list;

int org_skriv(void) {
	int editret;
	if(org_initheader(EJKOM)) return(1);
	nu_skrivs=TEXT;
	if((editret=edittext(NULL))==1) return(1);
	if(!editret) org_sparatext();
	return(0);
}

void org_kommentera(void) {
  int textId, editret, confId, isCorrect;
  struct Mote *conf;
  if(argument[0]) {
    textId = parseTextNumber(argument, TEXT);
    if(textId < Servermem->info.lowtext || textId > Servermem->info.hightext) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
      return;
    }
    confId = GetConferenceForText(textId);
    if(!MayBeMemberConf(confId, inloggad, CURRENT_USER)) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMENT_NO_PERMISSIONS));
      return;
    }
    conf = getmotpek(confId);
    if(conf == NULL) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't find conference %d in memory in Cmd_Reply().", confId);
      DisplayInternalError();
      return;
    }
    if(conf->status & KOMSKYDD) {
      if(!MayReplyConf(conf->nummer, inloggad, CURRENT_USER)) {
        SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMENT_NO_COMMENT_IN_FORUM));
        return;
      } else {
        if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMENT_REALLY_COMMENT),
                      NULL, NULL, "Ja", "Nej", "\r\n", FALSE, &isCorrect)) {
          return;
        }
        if(!isCorrect) {
          return;
        }
      }
    }
    if(readtexthead(textId,&readhead)) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't read header for text %d in Cmd_Reply()", textId);
      DisplayInternalError();
      return;
    }
    senast_text_typ = TEXT;
    senast_text_nr = textId;
    senast_text_mote = conf->nummer;
  }
  conf = getmotpek(senast_text_mote);
  if(conf == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't find conference %d in memory in Cmd_Reply().", senast_text_mote);
    DisplayInternalError();
    return;
  }
  if(org_initheader(KOM)) {
    return;
  }
  nu_skrivs = TEXT;
  if((editret = edittext(NULL))==1) {
    return;
  }
  else if(!editret)	{
    org_linkkom();
    org_sparatext();
  }
}

void org_lasa(int tnr, char verbose) {
  if(tnr < Servermem->info.lowtext || tnr > Servermem->info.hightext) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return;
  }
  org_visatext(tnr, verbose);
}

int HasUnreadInOrgConf(int conf) {
  long unreadText;

  unreadText = FindNextUnreadText(0, conf, CUR_USER_UNREAD);
  if(unreadText == -1) {
    CUR_USER_UNREAD->lowestPossibleUnreadText[conf] =
      Servermem->info.hightext + 1;
  }
  return unreadText != -1;
}

void pushTextRepliesToStack(struct Header *textHeader) {
  int i, confId, textId;

  for(i = MAXKOM; i >= 0; i--) {
    if(textHeader->kom_i[i] == -1) {
      continue;
    }
    textId = textHeader->kom_i[i];
    confId = GetConferenceForText(textId);
    if(confId == -1
       || !IsTextUnread(textId, CUR_USER_UNREAD)
       || !IsMemberConf(confId, inloggad, CURRENT_USER)) {
      continue;
    }
    StackPush(g_unreadRepliesStack, textId);
  }
}

void displayTextAndClearUnread(int textId) {
  if(org_visatext(textId, FALSE)) {
    pushTextRepliesToStack(&readhead);
  }
  if(!ImmediateLogout()) {
    ChangeUnreadTextStatus(textId, 0, CUR_USER_UNREAD);
  }
  g_lastKomTextType = TEXT;
  g_lastKomTextNr = textId;
  g_lastKomTextConf = GetConferenceForText(textId);
}

void NextTextInOrgConf(void) {
  int textId;

  textId = FindNextUnreadText(0, mote2, CUR_USER_UNREAD);
  if(textId == -1) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_NEXT_TEXT_NO_TEXTS));
    return;
  }
  StackClear(g_unreadRepliesStack);
  displayTextAndClearUnread(textId);
}

void NextReplyInOrgConf(void) {
  displayTextAndClearUnread(StackPop(g_unreadRepliesStack));
}

void displayReactions(int textId, int index, char verbose) {
  struct MemHeaderExtension *ext;
  struct MemHeaderExtensionNode *node;
  int i, j, likeCnt = 0, dislikeCnt = 0;
  long *reactionData, userId;

  if(index == 0) {
    return;
  }
  if(!(ext = ReadHeaderExtension(textId, readhead.extensionIndex))) {
    DisplayInternalError();
    return;
  }
  ITER_EL(node, ext->nodes, node, struct MemHeaderExtensionNode *) {
    for(i = 0; i < 4; i++) {
      if(node->ext.items[i].type != REACTION) {
        continue;
      }
      for(j = 0; j < 64; j += sizeof(long)) {
        reactionData = (long *)&node->ext.items[i].data[j];
        if(*reactionData == 0) {
          break;
        }
        userId = *reactionData & 0x00ffffff;
        switch(*reactionData & 0xff000000) {
        case EXT_REACTION_LIKE:
          likeCnt++;
          if(verbose) {
            SendStringCat("  - %s\r\n", CATSTR(MSG_REACTION_TEXT_VERBOSE_PRS),
                          getusername(userId));
          }
          break;
        case EXT_REACTION_DISLIKE:
          dislikeCnt++;
          if(verbose) {
            SendStringCat("  - %s\r\n", CATSTR(MSG_REACTION_TEXT_VERBOSE_DIS),
                          getusername(userId));
          }
          break;
        }
      }
    }
  }
  DeleteMemHeaderExtension(ext);
  if(likeCnt > 0) {
    SendStringCat("(%s)\r\n", CATSTR(MSG_REACTION_TEXT_PRAISED), likeCnt);
  }
  if(dislikeCnt > 0) {
    SendStringCat("(%s)\r\n", CATSTR(MSG_REACTION_TEXT_DISSED), dislikeCnt);
  }
}

int org_visatext(int textId, char verbose) {
  int i, length, confId, pos;
  struct tm *ts;
  struct EditLine *el;

  CURRENT_USER->read++;
  Servermem->info.lasta++;
  Statstr.read++;

  if(GetConferenceForText(textId) == -1) {
    SendStringCat("\n\n\r%s\n\n\r", CATSTR(MSG_TEXT_DELETED), textId);
    if(CURRENT_USER->status < Servermem->cfg->st.medmoten) {
      return 0;
    }
  }
  if(readtexthead(textId, &readhead)) {
    return 0;
  }
  if(!MayReadConf(readhead.mote, inloggad, CURRENT_USER)) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_TEXT_NO_PERM));
    return 0;
  }
  ts = localtime(&readhead.tid);
  SendStringCat("\r\n\n%s", CATSTR(MSG_ORG_TEXT_LINE1),
             readhead.nummer, getmotnamn(readhead.mote));
  SendString("    %4d%02d%02d %02d:%02d\r\n",
             ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
             ts->tm_min);
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_LINE2), getusername(readhead.person));
  if(readhead.kom_till_nr != -1) {
    SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_COMMENT_TO), readhead.kom_till_nr,
               getusername(readhead.kom_till_per));
  }
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_SUBJECT), readhead.arende);
  if(CURRENT_USER->flaggor & STRECKRAD) {
    SendRepeatedChr('-', RenderLength(outbuffer));
    SendString("\r\n\n");
  } else {
    SendString("\n");
  }
  if(readtextlines(readhead.textoffset, readhead.rader, readhead.nummer)) {
    freeeditlist();
    return 0;
  }
  ITER_EL(el, edit_list, line_node, struct EditLine *) {
    if(SendString(IsQuote(el->text) ? "«quote»%s«reset»\r" : "%s\r", el->text)) {
      break;
    }
  }
  freeeditlist();
  SendStringCat("\n%s\r\n", CATSTR(MSG_ORG_TEXT_END_OF_TEXT), readhead.nummer,
             getusername(readhead.person));

  for(i = 0; readhead.kom_i[i] != -1; i++) {
    confId = GetConferenceForText(readhead.kom_i[i]);
    if(confId != -1 && IsMemberConf(confId, inloggad, CURRENT_USER)) {
      SendStringCat("  %s\r\n", CATSTR(MSG_ORG_TEXT_COMMENT_IN), readhead.kom_i[i],
                 getusername(readhead.kom_av[i]));
    }
  }

  displayReactions(textId, readhead.extensionIndex, verbose);

  if(readhead.footNote != 0) {
    length = (readhead.footNote >> 24) & 0xff;
    pos = readhead.footNote & 0xffffff;
    if(readtextlines(pos, length, readhead.nummer)) {
      freeeditlist();
      return 0;
    }
    SendString("\r\n%s\r\n", CATSTR(MSG_ORG_TEXT_FOOTNOTE));
    ITER_EL(el, edit_list, line_node, struct EditLine *) {
      if(SendString("  %s\r", el->text)) {
        break;
      }
    }
    freeeditlist();
  }

  senast_text_typ=TEXT;
  senast_text_nr=readhead.nummer;
  senast_text_mote=readhead.mote;
  senast_text_reply_to = readhead.kom_till_nr;
  if(readhead.kom_i[0]!=-1) {
    return 1;
  }
  return 0;
}

void org_sparatext(void) {
  int i, nummer;
  CURRENT_USER->skrivit++;
  Servermem->info.skrivna++;
  Statstr.write++;
  sparhead.rader=rad;
  for(i = 0; i < MAXKOM; i++) {
    sparhead.kom_i[i] = -1;
    sparhead.kom_av[i] = -1;
  }
  nummer = sendservermess(SPARATEXTEN, (long)&sparhead);
  SendStringCat("\r\n%s\r\n", CATSTR(MSG_WRITE_TEXT_GOT_NUMBER), nummer);
  if(Servermem->cfg->logmask & LOG_TEXT) {
    LogEvent(USAGE_LOG, INFO, "%s skriver text %d i %s",
             getusername(inloggad), nummer, getmotnamn(sparhead.mote));
  }
  freeeditlist();
}

void org_linkkom(void) {
  int i = 0;
  struct Header linkhead;
  if(readtexthead(readhead.nummer, &linkhead)) {
    return;
  }
  while(linkhead.kom_i[i] != -1 && i < MAXKOM) {
    i++;
  }
  if(i == MAXKOM) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_WRITE_TOO_MANY_COMMENTS));
    return;
  }
  linkhead.kom_i[i] = Servermem->info.hightext+1;
  linkhead.kom_av[i] = sparhead.person;
  writetexthead(readhead.nummer, &linkhead);
}

int org_initheader(int komm) {
  int length = 0, i = 0;
  long tid;
  struct tm *ts;
  sparhead.person = inloggad;
  if(komm) {
    sparhead.kom_till_nr = readhead.nummer;
    sparhead.kom_till_per = readhead.person;
    sparhead.mote = readhead.mote;
    sparhead.root_text = readhead.root_text;
  } else {
    sparhead.kom_till_nr = -1;
    sparhead.kom_till_per = -1;
    sparhead.mote = mote2;
    sparhead.root_text = 0;
  }
  Servermem->nodeInfo[nodnr].action = SKRIVER;
  Servermem->nodeInfo[nodnr].currentConf = sparhead.mote;
  time(&tid);
  ts = localtime(&tid);
  sparhead.tid = tid;
  sparhead.textoffset = (long)&edit_list;
  sparhead.extensionIndex = 0;
  SendStringCat("\r\n\n%s", CATSTR(MSG_WRITE_FORUM), getmotnamn(sparhead.mote));
  SendString("    %4d%02d%02d %02d:%02d\r\n",
             ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
             ts->tm_min);
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_LINE2), getusername(inloggad));
  if(komm) {
    SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_COMMENT_TO), sparhead.kom_till_nr,
                  getusername(sparhead.kom_till_per));
  }
  SendString("%s ", CATSTR(MSG_WRITE_SUBJECT));
  if(komm) {
    strcpy(sparhead.arende, readhead.arende);
    SendString(sparhead.arende);
  } else {
    if(getstring(EKO, 40, NULL)) {
      return 1;
    }
    strcpy(sparhead.arende,inmat);
  }
  SendString("\r\n");
  if(CURRENT_USER->flaggor & STRECKRAD) {
    length = strlen(sparhead.arende);
    for(i = 0; i < length + 8; i++) {
      outbuffer[i] = '-';
    }
    outbuffer[i] = 0;
    SendString("%s\r\n\n", outbuffer);
  } else {
    SendString("\n");
  }
  return 0;
}

void org_endast(int conf,int amount) {
  SetUnreadTexts(conf, amount, CUR_USER_UNREAD);
}
