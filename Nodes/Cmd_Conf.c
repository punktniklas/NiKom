#include <stdlib.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "Logging.h"
#include "ConfCommon.h"

#include "Cmd_Conf.h"

#define TEXTTYPE_REPLY	 1

extern char *argument;
extern int mote2, inloggad, nodnr, senast_text_typ, senast_text_nr, senast_text_mote;
extern struct System *Servermem;

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
        if(GetYesOrNo(
           "\r\n\nVill du verkligen kommentera i ett kommentarsskyddat möte? ",
           'j', 'n', "Ja\r\n", "Nej\r\n", FALSE, &isCorrect)) {
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
    if(GetYesOrNo(
       "\r\n\nVill du verkligen kommentera i ett kommentarsskyddat möte? ",
       'j', 'n', "Ja\r\n", "Nej\r\n", FALSE, &isCorrect)) {
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
  if(argument[0] == '\0') {
    SendString("\r\n\nSkriv: Läsa <textnr>\r\n");
    return;
  }
  textId = parseTextNumber(argument, mote2 == -1 ? BREV : 0);
  if(textId == -1) {
    SendString("\r\n\nFinns ingen sådan text.\r\n");
    return;
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
      org_lasa(textId);
    } else if(conf->type == MOTE_FIDO) {
      fido_lasa(textId,conf);
    }
  }
}

