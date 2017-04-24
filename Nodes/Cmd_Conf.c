#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <proto/dos.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "Logging.h"
#include "ConfCommon.h"
#include "StringUtils.h"
#include "ConfHeaderExtensions.h"
#include "Languages.h"

#include "Cmd_Conf.h"

#define TEXTTYPE_REPLY	 1

extern char *argument;
extern int mote2, inloggad, nodnr, senast_text_typ, senast_text_nr, senast_text_mote;
extern struct System *Servermem;
extern struct MinList edit_list;

int saveFootNoteLines(int textId, struct Header *textHeader);

void Cmd_Reply(void) {
  struct Mote *conf;
  int isCorrect;

  if(argument[0]) {
    if(mote2 == -1) {
      brev_kommentera();
      return;
    }
    conf = getmotpek(mote2);
    if(conf->type == MOTE_ORGINAL) {
      org_kommentera();
      return;
    }
    if(conf->type == MOTE_FIDO) {
      if(!MayReplyConf(mote2, inloggad, &Servermem->inne[nodnr])) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMENT_NO_COMMENT_IN_FORUM));
        return;
      }
      if(conf->status & KOMSKYDD) {
        if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMENT_REALLY_COMMENT),
                      NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      "\r\n", FALSE, &isCorrect)) {
          return;
        }
        if(!isCorrect) {
          return;
        }
      }
      fido_skriv(TEXTTYPE_REPLY, atoi(argument));
      return;
    }
  }

  if(!senast_text_typ) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_SKIP_NO_TEXT_READ));
    return;
  }
  if(senast_text_typ == BREV) {
    brev_kommentera();
    return;
  }
  conf = getmotpek(senast_text_mote);
  if(!conf) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Conference for last read text (confId = %d) does not exist.",
             senast_text_mote);
    DisplayInternalError();
    return;
  }
  if(!MayReplyConf(senast_text_mote, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMENT_NO_COMMENT_IN_FORUM));
    return;
  }
  if(conf->status & KOMSKYDD) {
    if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMENT_REALLY_COMMENT),
                  NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                  "\r\n", FALSE, &isCorrect)) {
      return;
    }
    if(!isCorrect) {
      return;
    }
  }
  if(conf->type == MOTE_ORGINAL) {
    org_kommentera();
  } else if(conf->type == MOTE_FIDO) {
    fido_skriv(TEXTTYPE_REPLY, senast_text_nr);
  }
}

void Cmd_Read(void) {
  int textId;
  struct Mote *conf;
  char verbose = FALSE, *flag;
  if(argument[0] == '\0') {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_READ_SYNTAX));
    return;
  }
  textId = parseTextNumber(argument, mote2 == -1 ? BREV : 0);
  if(textId == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return;
  }
  flag = hittaefter(argument);
  if(flag[0] == '-' && (flag[1] == 'v' || flag[1] == 'V')) {
    verbose = TRUE;
  }
  
  if(mote2 == -1) {
    brev_lasa(textId);
  }
  else {
    conf = getmotpek(mote2);
    if(conf == NULL) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't find conference %d in memory in Cmd_Read().", mote2);
      DisplayInternalError();
      return;
    }
    if(conf->type == MOTE_ORGINAL) {
      org_lasa(textId, verbose);
    } else if(conf->type == MOTE_FIDO) {
      fido_lasa(textId,conf);
    }
  }
}

void Cmd_FootNote(void) {
  int textId, confId, editRet;
  struct Header textHeader;

  if(argument[0] == '\0') {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FOOTNOTE_SYNTAX));
    return;
  }
  textId = atoi(argument);
  confId = GetConferenceForText(textId);
  if(confId == -1) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return;
  }
  if(readtexthead(textId,&textHeader)) {
    return;
  }
  if(textHeader.person != inloggad
     && !MayAdminConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FOOTNOTE_ONLY_OWN_TEXTS));
    return;
  }
  if(textHeader.footNote != 0) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FOOTNOTE_ALREADY_HAS));
    return;
  }

  SendStringCat("\n\n\r%s\n\r", CATSTR(MSG_FOOTNOTE_PRE_EDIT),
             textId, getusername(textHeader.person));
  if((editRet = edittext(NULL)) != 0) {
    return;
  }

  if(saveFootNoteLines(textId, &textHeader)) {
    writetexthead(textId, &textHeader);
  }
  freeeditlist();
}

int saveFootNoteLines(int textId, struct Header *textHeader) {
  int len, lineCnt = 0;
  BPTR fh;
  char filename[30];
  struct EditLine *line;

  NiKForbid();
  sprintf(filename, "NiKom:Moten/Text%d.dat", textId/512);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open %s to add footnote.", filename);
    DisplayInternalError();
    NiKPermit();
    return 0;
  }
  Seek(fh, 0, OFFSET_END);
  textHeader->footNote = Seek(fh, 0, OFFSET_CURRENT);
  ITER_EL(line, edit_list, line_node, struct EditLine *) {
    len = strlen(line->text);
    line->text[len] = '\n';
    if(Write(fh, line->text, len + 1) != len + 1) {
      LogEvent(SYSTEM_LOG, ERROR, "Error writing to %s when adding footnote.",
               filename);
      DisplayInternalError();
      Close(fh);
      NiKPermit();
      return 0;
    }
    if(++lineCnt >= 255) {
      break;
    }
  }
  Close(fh);
  NiKPermit();
  textHeader->footNote |= (lineCnt << 24);
  return 1;
}

void Cmd_Search(void) {
  char *argptr = argument, buf[100], *searchstr;
  int global = FALSE, currentText, pos, searchstrlen, cnt = 0, foundsomething = 0, confId, i;
  struct Mote *conf;
  struct Header textHeader;
  struct EditLine *el;
  struct tm *ts;

  global = FALSE;
  if(argptr[0] == '-') {
    if(argptr[1] == 'g') {
      global = TRUE;
    } else {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_KOM_INVALID_SWITCH));
      return;
    }
    argptr = FindNextWord(argptr);
  }
  if(argptr[0] == '\0') {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_SEARCH_SYNTAX));
    return;
  }
  searchstr = argptr;
  searchstrlen = strlen(searchstr);

  if(!global) {
    if(mote2 == -1) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_SEARCH_NOT_MAILBOX));
      return;
    }
    conf = getmotpek(mote2);
    if(conf->type == MOTE_FIDO) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_SEARCH_NOT_FIDO));
      return;
    }
  }

  currentText = Servermem->info.hightext + 1;
  SendString("\r\n\n");

  for(;;) {
    if((++cnt % 100) == 0) {
      if(SendString(".")) {
        return;
      }
    }
    if(global) {
      for(i = currentText - 1; i >= Servermem->info.lowtext; i--) {
        if((confId = GetConferenceForText(i)) == -1) {
          continue;
        }
        if(!MayReadConf(confId, inloggad, &Servermem->inne[nodnr])) {
          continue;
        }
        break;
      }
      currentText = i < Servermem->info.lowtext ? -1 : i;
    } else {
      currentText = FindPrevTextInConference(currentText - 1, mote2);
    }
    if(currentText == -1) {
      SendString("\r\x1b\x5b\x4b");
      if(!foundsomething) {
        SendString("%s\r\n\n", CATSTR(MSG_SEARCH_NO_TEXTS_FOUND));
      }
      return;
    }
    if(readtexthead(currentText, &textHeader)) {
      return;
    }
    if(readtextlines(textHeader.textoffset, textHeader.rader, textHeader.nummer)) {
      freeeditlist();
      return;
    }
    ITER_EL(el, edit_list, line_node, struct EditLine *) {
      if((pos = LenientFindSubString(el->text, searchstr)) == -1) {
        continue;
      }
      foundsomething = 1;
      ts = localtime(&textHeader.tid);
      if(SendStringCat("\r\x1b\x5b\x4b%s\r\n", CATSTR(MSG_SEARCH_RESULT_LINE1),
                    currentText, ts->tm_year % 100, ts->tm_mon + 1,
                    ts->tm_mday, getusername(textHeader.person))) {
        freeeditlist();
        return;
      }
      if(global) {
        conf = getmotpek(textHeader.mote);
        if(SendStringCat("%s\r\n", CATSTR(MSG_SEARCH_RESULT_LINE2), conf->namn)) {
          freeeditlist();
          return;
        }
      }
      SendStringNoBrk("   ");
      if(pos > 0) {
        strncpy(buf, el->text, pos);
        buf[pos] = '\0';
        SendStringNoBrk(buf);
      }
      strncpy(buf, el->text + pos, searchstrlen);
      buf[searchstrlen] = '\0';
      SendStringNoBrk("\x1b\x5b\x31\x6d%s\x1b\x5b\x30\x6d", buf);
      if(el->text[pos + searchstrlen] != '\0') {
        SendStringNoBrk(&el->text[pos + searchstrlen]);
      }
      if(SendString("\r\n\n")) {
        freeeditlist();
        return;
      }
      break;
    }
    freeeditlist();
  }
}

/*
 * Returns 0 if the user already has that reaction. Otherwise the reaction
 * is stored and 1 is returned.
 */
int setReactionInExtension(struct MemHeaderExtension *ext, long newReaction) {
  struct MemHeaderExtensionNode *extNode;
  int i, j;
  long userId, oldReaction;
  struct HeaderExtension_Reaction *extReact;
  
  ITER_EL(extNode, ext->nodes, node, struct MemHeaderExtensionNode *) {
    for(i = 0; i < 4; i++) {
      if(extNode->ext.items[i].type == REACTION) {
        extReact = (struct HeaderExtension_Reaction *) extNode->ext.items[i].data;
        for(j = 0; j < 16; j += 1) {
          oldReaction = extReact->reactions[j] & 0xff000000;
          userId = extReact->reactions[j] & 0x00ffffff;
          if(extReact->reactions[j] != 0) {
            if(userId != inloggad) {
              continue;
            }
            if(newReaction == oldReaction) {
              return 0;
            }
          }
          extReact->reactions[j] = newReaction | inloggad;
          extNode->dirty = 1;
          return 1;
        }
      } else if(extNode->ext.items[i].type == NONE) {
        extNode->ext.items[i].type = REACTION;
        *((long *)extNode->ext.items[i].data) = newReaction | inloggad;
        extNode->dirty = 1;
        return 1;
      }
    }
  }
  if(!(extNode = AddMemHeaderExtensionNode(ext))) {
    DeleteMemHeaderExtension(ext);
    LogEvent(SYSTEM_LOG, ERROR, "Can't allocate MemHeaderExtensionNode in Cmd_Like()");
    DisplayInternalError();
    return 1;
  }
  extNode->ext.items[0].type = REACTION;
  *((long *)extNode->ext.items[0].data) = newReaction | inloggad;
  return 1;
}

void cmd_Reaction(long reaction) {
  struct Mote *conf;
  int textId, confId;
  struct MemHeaderExtension *ext;
  struct Header textHeader;
  
  if(argument[0]) {
    if(mote2 == -1) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_REACTION_NO_MAIL));
      return;
    }
    conf = getmotpek(mote2);
    if(conf->type != MOTE_ORGINAL) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_REACTION_ONLY_LOCAL));
      return;
    }
    textId = parseTextNumber(argument, TEXT);
    if(textId == -1 || (confId = GetConferenceForText(textId)) == -1) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
      return;
    }
    if(!MayReadConf(confId, inloggad, &Servermem->inne[nodnr])) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_REACTION_NO_PERMISSION));
      return;
    }
  } else {
    if(!senast_text_typ) {
      SendString("\n\n\r%s\n\r", CATSTR(MSG_SKIP_NO_TEXT_READ));
      return;
    }
    if(senast_text_typ == BREV) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_REACTION_NO_MAIL));
      return;
    }
    conf = getmotpek(senast_text_mote);
    if(!conf) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Conference for last read text (confId = %d) does not exist.",
               senast_text_mote);
      DisplayInternalError();
      return;
    }
    if(conf->type != MOTE_ORGINAL) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_REACTION_ONLY_LOCAL));
      return;
    }
    textId = senast_text_nr;
  }

  if(readtexthead(textId, &textHeader)) {
    DisplayInternalError();
    return;
  }

  if(textHeader.extensionIndex > 0) {
    if(!(ext = ReadHeaderExtension(textId, textHeader.extensionIndex))) {
      DisplayInternalError();
      return;
    }
  } else {
    if(!(ext = CreateMemHeaderExtension(textId))) {
      LogEvent(SYSTEM_LOG, ERROR, "Can't allocate MemHeaderExtension in Cmd_Like()");
      DisplayInternalError();
      return;
    }
  }
  if(!setReactionInExtension(ext, reaction)) {
    SendStringCat("\r\n\n%s\r\n", reaction == EXT_REACTION_LIKE
                  ? CATSTR(MSG_REACTION_ALREADY_PRS) : CATSTR(MSG_REACTION_ALREADY_DIS),
                  textId);
  } else {
    if(SaveHeaderExtension(ext)) {
      DisplayInternalError();
    }
    SendStringCat("\r\n\n%s\r\n", reaction == EXT_REACTION_LIKE
                  ? CATSTR(MSG_REACTION_DONE_PRS) : CATSTR(MSG_REACTION_DONE_DIS),
                  textId);
  }
  DeleteMemHeaderExtension(ext);
}

void Cmd_Like(void) {
  cmd_Reaction(EXT_REACTION_LIKE);
}

void Cmd_Dislike(void) {
  cmd_Reaction(EXT_REACTION_DISLIKE);
}
