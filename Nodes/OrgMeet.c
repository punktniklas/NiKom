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
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Stack.h"
#include "Terminal.h"
#include "KOM.h"
#include "ConfCommon.h"
#include "ConfHeaderExtensions.h"
#include "Languages.h"

#include "OrgMeet.h"

#define EKO		1
#define KOM		1
#define EJKOM	0

extern struct System *Servermem;
extern char outbuffer[],*argument,inmat[];
extern int nodnr,senast_text_typ,senast_text_nr,senast_text_mote,nu_skrivs,inloggad,
	rad,mote2;
extern int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf;
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
      SendString("\r\n\nFinns ingen sådan text.\r\n");
      return;
    }
    confId = GetConferenceForText(textId);
    if(!MayBeMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
      SendString("\r\n\nDu har inga rättigheter i mötet där texten finns.\r\n");
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
      if(!MayReplyConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {
        SendString("\r\n\nDu får inte kommentera i kommentarsskyddade möten.\r\n");
        return;
      } else {
        if(GetYesOrNo("\r\n\n", "Vill du verkligen kommentera i ett kommentarsskyddat möte? ",
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
	if(tnr<Servermem->info.lowtext || tnr>Servermem->info.hightext) {
		puttekn("\r\n\nTexten finns inte!\r\n",-1);
		return;
	}
	org_visatext(tnr, verbose);
}

int HasUnreadInOrgConf(int conf) {
  long unreadText;

  unreadText = FindNextUnreadText(0, conf, &Servermem->unreadTexts[nodnr]);
  if(unreadText == -1) {
    Servermem->unreadTexts[nodnr].lowestPossibleUnreadText[conf] =
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
       || !IsTextUnread(textId, &Servermem->unreadTexts[nodnr])
       || !IsMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
      continue;
    }
    StackPush(g_unreadRepliesStack, textId);
  }
}

void displayTextAndClearUnread(int textId) {
  ChangeUnreadTextStatus(textId, 0, &Servermem->unreadTexts[nodnr]);
  if(org_visatext(textId, FALSE)) {
    pushTextRepliesToStack(&readhead);
  }
  g_lastKomTextType = TEXT;
  g_lastKomTextNr = textId;
  g_lastKomTextConf = GetConferenceForText(textId);
}

void NextTextInOrgConf(void) {
  int textId;

  textId = FindNextUnreadText(0, mote2, &Servermem->unreadTexts[nodnr]);
  if(textId == -1) {
    SendString("\n\n\rFinns inga olästa texter i detta möte.\n\r");
    return;
  }
  StackClear(g_unreadRepliesStack);
  displayTextAndClearUnread(textId);
}

void NextReplyInOrgConf(void) {
  if(StackSize(g_unreadRepliesStack) == 0) {
    SendString("\n\n\rFinns inga fler olästa kommentarer\n\r");
    return;
  }
  displayTextAndClearUnread(StackPop(g_unreadRepliesStack));
}

void displayReactions(int textId, int index, char verbose) {
  struct MemHeaderExtension *ext;
  struct MemHeaderExtensionNode *node;
  int i, j, likeCnt = 0, dislikeCnt = 0;
  long *reactionData, userId;
  char *reactionStr;

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
          reactionStr = "hyllats";
          break;
        case EXT_REACTION_DISLIKE:
          dislikeCnt++;
          reactionStr = "dissats";
          break;
        default:
          reactionStr = "fluppats";
        }
        if(verbose) {
          SendString("  - Texten har %s av %s\r\n", reactionStr, getusername(userId));
        }
      }
    }
  }
  DeleteMemHeaderExtension(ext);
  if(likeCnt > 0) {
    SendString("(Texten har hyllats av %d personer)\r\n", likeCnt);
  }
  if(dislikeCnt > 0) {
    SendString("(Texten har dissats av %d personer)\r\n", dislikeCnt);
  }
}

int org_visatext(int textId, char verbose) {
  int i, length, confId, pos;
  struct tm *ts;
  struct EditLine *el;

  Servermem->inne[nodnr].read++;
  Servermem->info.lasta++;
  Statstr.read++;

  if(GetConferenceForText(textId) == -1) {
    SendString("\n\n\rText %d är raderad.\n\n\r", textId);
    if(Servermem->inne[nodnr].status < Servermem->cfg.st.medmoten) {
      return 0;
    }
  }
  if(readtexthead(textId, &readhead)) {
    return 0;
  }
  if(!MayReadConf(readhead.mote, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu har inte rätt att läsa den texten!\r\n\n");
    return 0;
  }
  ts = localtime(&readhead.tid);
  SendStringCat("\r\n\n%s", CATSTR(MSG_ORG_TEXT_LINE1),
             readhead.nummer, getmotnamn(readhead.mote));
  SendString("    %4d%02d%02d %02d:%02d\r\n",
             ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
             ts->tm_min);
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_LINE2), readhead.person !=-1
             ? getusername(readhead.person) : "<raderad användare>");
  if(readhead.kom_till_nr != -1) {
    SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_COMMENT_TO), readhead.kom_till_nr,
               getusername(readhead.kom_till_per));
  }
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_SUBJECT), readhead.arende);
  if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
    length = strlen(readhead.arende) + 8;
    for(i = 0; i < length; i++) {
      outbuffer[i] = '-';
    }
    outbuffer[i] = '\0';
    SendString("%s\r\n\n", outbuffer);
  } else {
    SendString("\n");
  }
  if(readtextlines(readhead.textoffset, readhead.rader, readhead.nummer)) {
    freeeditlist();
    return 0;
  }
  ITER_EL(el, edit_list, line_node, struct EditLine *) {
    if(SendString("%s\r", el->text)) {
      break;
    }
  }
  freeeditlist();
  SendStringCat("\n%s\r\n", CATSTR(MSG_ORG_TEXT_END_OF_TEXT), readhead.nummer,
             getusername(readhead.person));

  for(i = 0; readhead.kom_i[i] != -1; i++) {
    confId = GetConferenceForText(readhead.kom_i[i]);
    if(confId != -1 && IsMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
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
  if(readhead.kom_i[0]!=-1) {
    return 1;
  }
  return 0;
}

void org_sparatext(void) {
	int x, nummer;
	Servermem->inne[nodnr].skrivit++;
	Servermem->info.skrivna++;
	Statstr.write++;
	sparhead.rader=rad;
	for(x=0;x<MAXKOM;x++) {
		sparhead.kom_i[x]=-1;
		sparhead.kom_av[x]=-1;
	}
	nummer = sendservermess(SPARATEXTEN,(long)&sparhead);
	sprintf(outbuffer,"\r\nTexten fick nummer %d\r\n",nummer);
	puttekn(outbuffer,-1);
	if(Servermem->cfg.logmask & LOG_TEXT) {
          LogEvent(USAGE_LOG, INFO, "%s skriver text %d i %s",
                   getusername(inloggad), nummer, getmotnamn(sparhead.mote));
	}
	freeeditlist();
}

void org_linkkom(void) {
	int x=0;
	struct Header linkhead;
	if(readtexthead(readhead.nummer,&linkhead)) return;
	while(linkhead.kom_i[x]!=-1 && x<MAXKOM) x++;
	if(x==MAXKOM) {
		puttekn("\r\n\nRedan maximalt med kommentarer till den texten, sparar texten i alla fall.\r\n\n",-1);
		return;
	}
	linkhead.kom_i[x]=Servermem->info.hightext+1;
	linkhead.kom_av[x]=sparhead.person;
	writetexthead(readhead.nummer,&linkhead);
}

int org_initheader(int komm) {
	int length=0,x=0;
	long tid;
	struct tm *ts;
	sparhead.person=inloggad;
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
	Servermem->action[nodnr] = SKRIVER;
	Servermem->varmote[nodnr] = sparhead.mote;
	time(&tid);
	ts=localtime(&tid);
	sparhead.tid=tid;
	sparhead.textoffset=(long)&edit_list;
        sparhead.extensionIndex = 0;
	sprintf(outbuffer,"\r\n\nMöte: %s    %4d%02d%02d %02d:%02d\r\n",
                getmotnamn(sparhead.mote), ts->tm_year + 1900, ts->tm_mon + 1,
                ts->tm_mday, ts->tm_hour, ts->tm_min);
	puttekn(outbuffer,-1);
	sprintf(outbuffer,"Skriven av %s\r\n",getusername(inloggad));
	puttekn(outbuffer,-1);
	if(komm) {
		sprintf(outbuffer,"Kommentar till text %d av %s\r\n",sparhead.kom_till_nr,getusername(sparhead.kom_till_per));
		puttekn(outbuffer,-1);
	}
	puttekn("Ärende: ",-1);
	if(komm) {
		strcpy(sparhead.arende,readhead.arende);
		puttekn(sparhead.arende,-1);
	} else {
		if(getstring(EKO,40,NULL)) return(1);
		strcpy(sparhead.arende,inmat);
	}
	puttekn("\r\n",-1);
	if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
		length=strlen(sparhead.arende);
		for(x=0;x<length+8;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		puttekn(outbuffer,-1);
		puttekn("\r\n\n",-1);
	} else puttekn("\n",-1);
	return(0);
}

void org_endast(int conf,int amount) {
  SetUnreadTexts(conf, amount, &Servermem->unreadTexts[nodnr]);
}
