#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "Logging.h"
#include "ConfCommon.h"
#include "StringUtils.h"
#include "ConfHeaderExtensions.h"

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
        SendString("\r\n\nDu får inte kommentera den texten!\r\n\n");
        return;
      }
      if(conf->status & KOMSKYDD) {
        if(GetYesOrNo("\r\n\n", "Vill du verkligen kommentera i ett kommentarsskyddat möte?",
                      NULL, NULL, "Ja", "Nej", "\r\n", FALSE, &isCorrect)) {
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
    SendString("\n\n\rDu har inte läst någon text ännu.\n\r");
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
    SendString("\r\n\nDu får inte kommentera den texten!\r\n\n");
    return;
  }
  if(conf->status & KOMSKYDD) {
    if(GetYesOrNo("\r\n\n", "Vill du verkligen kommentera i ett kommentarsskyddat möte?",
                  NULL, NULL, "Ja", "Nej", "\r\n", FALSE, &isCorrect)) {
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
    SendString("\r\n\nSkriv: Läsa <textnr>\r\n");
    return;
  }
  textId = parseTextNumber(argument, mote2 == -1 ? BREV : 0);
  if(textId == -1) {
    SendString("\r\n\nFinns ingen sådan text.\r\n");
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
    SendString("\n\n\rSkriv: Fotnot <text nummer>\n\r");
    return;
  }
  textId = atoi(argument);
  confId = GetConferenceForText(textId);
  if(confId == -1) {
    SendString("\n\n\rFinns ingen sådan text.\n\r");
    return;
  }
  if(readtexthead(textId,&textHeader)) {
    return;
  }
  if(textHeader.person != inloggad
     && !MayAdminConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu kan bara lägga till fotnoter på dina egna texter.\r\n\n");
    return;
  }
  if(textHeader.footNote != 0) {
    SendString("\n\n\rTexten har redan en fotnot.\n\r");
    return;
  }

  SendString("\n\n\rFotnot till text %d av %s\n\r",
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
      SendString("\r\n\nFelaktig flagga.\r\n\n");
      return;
    }
    argptr = FindNextWord(argptr);
  }
  if(argptr[0] == '\0') {
    SendString("\r\n\nSkriv: Sök [-g] <söksträng>\r\n\n");
    return;
  }
  searchstr = argptr;
  searchstrlen = strlen(searchstr);

  if(!global) {
    if(mote2 == -1) {
      SendString("\r\n\nDu kan inte söka i %s.\r\n\n", Servermem->cfg.brevnamn);
      return;
    }
    conf = getmotpek(mote2);
    if(conf->type == MOTE_FIDO) {
      SendString("\r\n\n Sök fungerar inte i FidoNet-möten.\r\n\n");
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
        SendString("Inga texter hittades.\r\n\n");
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
      if(SendString("\r\x1b\x5b\x4bText: %d skriven %02d%02d%02d av %s\r\n",
                    currentText, ts->tm_year % 100, ts->tm_mon + 1,
                    ts->tm_mday, getusername(textHeader.person))) {
        freeeditlist();
        return;
      }
      if(global) {
        conf = getmotpek(textHeader.mote);
        if(SendString("Möte: %s\r\n", conf->namn)) {
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
      SendString("\r\n\nDu kan inte hylla/dissa brev.\r\n");
      return;
    }
    conf = getmotpek(mote2);
    if(conf->type != MOTE_ORGINAL) {
      SendString("\r\n\nDu kan bara hylla/dissa lokala texter.\r\n\n");
      return;
    }
    textId = parseTextNumber(argument, TEXT);
    if(textId == -1 || (confId = GetConferenceForText(textId)) == -1) {
      SendString("\r\n\nFinns ingen sådan text.\r\n");
      return;
    }
    if(!MayReadConf(confId, inloggad, &Servermem->inne[nodnr])) {
      SendString("\r\n\nDu har ingen rätt att hylla/dissa den texten.\r\n");
      return;
    }
  } else {
    if(!senast_text_typ) {
      SendString("\n\n\rDu har inte läst någon text ännu.\n\r");
      return;
    }
    if(senast_text_typ == BREV) {
      SendString("\r\n\nDu kan inte hylla/dissa brev.\r\n");
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
      SendString("\r\n\nDu kan bara hylla/dissa lokala texter.\r\n\n");
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
    SendString("\r\n\nDu har redan %s text %d.\r\n",
               reaction == EXT_REACTION_LIKE ? "hyllat" : "dissat", textId);
  } else {
    if(SaveHeaderExtension(ext)) {
      DisplayInternalError();
    }
    SendString("\r\n\nDu har %s text %d.\r\n",
               reaction == EXT_REACTION_LIKE ? "hyllat" : "dissat", textId);
  }
  DeleteMemHeaderExtension(ext);
}

void Cmd_Like(void) {
  cmd_Reaction(EXT_REACTION_LIKE);
}

void Cmd_Dislike(void) {
  cmd_Reaction(EXT_REACTION_DISLIKE);
}
