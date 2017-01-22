#include <stdlib.h>
#include "StringUtils.h"
#include "Languages.h"
#include "NiKomFuncs.h"
#include "NiKomStr.h"

#include "ConfCommon.h"

extern int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf, senast_text_typ, senast_text_mote;
extern struct ReadLetter brevread;
extern struct Header readhead;
extern struct System *Servermem;
extern char *argument;

int parseTextNumber(char *str, int requestedType) {
  struct Mote *conf;
  int mailId, userIdOwningMail;
  static char fakeArgumentBuf[20];
  
  if(IzDigit(str[0])) {
    return atoi(str);
  }
  if(InputMatchesWord(str, CATSTR(MSG_TEXT_PARSE_LATEST))) {
    if(requestedType != 0 && g_lastKomTextType != requestedType) {
      return -1;
    }
    return g_lastKomTextNr;
  }
  if(InputMatchesWord(str, CATSTR(MSG_TEXT_PARSE_COMMENTED))) {
    switch(senast_text_typ) {
    case BREV:
      if(!brevread.reply[0]) {
        return -1;
      }
      // This is kind of ugly. We create a fake argument string with the arguments to
      // the Read command as if the user had typed it in to read the commented mail.
      userIdOwningMail = atoi(brevread.reply);
      mailId = atoi(FindNextWord(brevread.reply));
      sprintf(fakeArgumentBuf, "%d %d", mailId, userIdOwningMail);
      argument = fakeArgumentBuf;
      return mailId;
    case TEXT:
      conf = getmotpek(senast_text_mote);
      if(conf->type==MOTE_ORGINAL) {
        if(readhead.kom_till_nr == -1 || readhead.kom_till_nr < Servermem->info.lowtext) {
          return -1;
        }
        return readhead.kom_till_nr;
      }
      return -1;
    default:
      return -1;
    }
  }
  return -1;
}
