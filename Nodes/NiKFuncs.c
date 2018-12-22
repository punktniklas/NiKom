#include "NiKomCompat.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#ifdef HAVE_PROTO_ALIB_H
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "StringUtils.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "Cmd_Users.h"
#include "NiKVersion.h"
#include "BasicIO.h"
#include "KOM.h"
#include "Languages.h"
#include "UserDataUtils.h"
#include "UserMessageUtils.h"
#include "Notifications.h"

#if defined(__GNUC__) && !defined(max)
#define max(a, b) \
  ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b);  \
    _a > _b ? _a : _b; })
#endif

#define EKO             1
#define EJEKO   0
#define KOM             1
#define EJKOM   0

extern struct System *Servermem;
extern int nodnr,inloggad,radcnt,rxlinecount, nodestate, g_userDataSlot;
extern char inmat[],backspace[],commandhistory[][1024];
extern struct ReadLetter brevread;
extern struct MinList edit_list;

long logintime, extratime;
int mote2, rad, senast_text_typ, nu_skrivs, area2, senast_brev_nr, senast_brev_anv,
  senast_text_nr, senast_text_mote, senast_text_reply_to;
int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf;
char *monthNames[2][12] =
  {{ "January", "February", "March", "April", "May", "June",
     "July", "August", "September", "October", "November", "December" },
   { "januari", "februari", "mars", "april", "maj", "juni",
     "juli", "augusti", "september", "oktober", "november", "december" }};
char *weekdayNames[2][7] =
  {{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
   { "Söndag", "Måndag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lördag" }};
char *argument,usernamebuf[50],vilkabuf[50];
struct Header sparhead,readhead;
struct Inloggning Statstr;
struct MinList aliaslist;

void igen(void) {
  struct Mote *conf;
  if(senast_text_typ == BREV) {
    visabrev(senast_brev_nr, senast_brev_anv);
  } else if(senast_text_typ == TEXT) {
    conf = getmotpek(senast_text_mote);
    if(conf->type == MOTE_ORGINAL) {
      org_visatext(senast_text_nr, FALSE);
    } else if(conf->type == MOTE_FIDO) {
      fido_visatext(senast_text_nr, conf, NULL);
    }
  }
  else {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_SKIP_NO_TEXT_READ));
  }
}

int skriv(void) {
  struct Mote *conf;
  if(mote2 == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_WRITE_NO_MAIL));
    return 0;
  }
  conf = getmotpek(mote2);
  if(!conf) {
    LogEvent(SYSTEM_LOG, ERROR, "User is in conf %d which doesn't seem to exist.", mote2);
    DisplayInternalError();
    return 0;
  }
  if(!MayWriteConf(conf->nummer, inloggad, CURRENT_USER)) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_WRITE_NO_PERM_IN_FORUM));
    return 0;
  }
  if(conf->type == MOTE_ORGINAL) {
    return org_skriv();
  } else if(conf->type == MOTE_FIDO) {
    return fido_skriv(EJKOM,0);
  }
  return 0;
}

void var(int mot) {
  if(mot == -1) {
    varmail();
  } else {
    varmote(mot);
  }
}

void varmote(int mote) {
  int cnt;
  struct Mote *conf;
  conf = getmotpek(mote);
  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_WHERE_YOU_ARE_IN), conf->namn);
  if(conf->type == MOTE_FIDO) {
    if(conf->lowtext > conf->texter) {
      SendString("%s\r\n", CATSTR(MSG_WHERE_NO_TEXTS));
    } else {
      SendStringCat("%s\r\n", CATSTR(MSG_WHERE_TEXTS_NUMBERED), conf->lowtext, conf->texter);
    }
  }
  cnt = countunread(mote);
  if(cnt == 0) {
    SendString("%s\r\n", CATSTR(MSG_WHERE_NO_UNREAD_TEXTS));
  } else if(cnt == 1) {
    SendString("%s\r\n", CATSTR(MSG_WHERE_ONE_UNREAD_TEXT));
  } else {
    SendStringCat("%s\r\n", CATSTR(MSG_WHERE_MANY_UNREAD_TEXTS), cnt);
  }
}

void tiden(void) {
  long now, timeLimitSeconds;
  struct tm *ts;
  time(&now);
  ts=localtime(&now);
  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_KOM_TIME_NOW),
          weekdayNames[CURRENT_USER->language][ts->tm_wday], ts->tm_mday,
          monthNames[CURRENT_USER->language][ts->tm_mon],
          1900 + ts->tm_year, ts->tm_hour, ts->tm_min);
  if(Servermem->cfg->maxtid[CURRENT_USER->status]) {
    timeLimitSeconds = 60 * (Servermem->cfg->maxtid[CURRENT_USER->status]) + extratime;
    SendStringCat("\n%s\r\n", CATSTR(MSG_KOM_TIME_LEFT), (timeLimitSeconds - (now - logintime)) / 60);
  }
}

int skapmot(void) {
  struct ShortUser *shortUser;
  int mad, setPermission, changed, ch, i, fidoDomainId, highestId, needsWrite;
  struct FidoDomain *fidoDomain;
  BPTR lock;
  struct User *user;
  struct Mote tmpConf,*searchConf,*newConf;

  memset(&tmpConf, 0, sizeof(struct Mote));
  if(argument[0] == '\0') {
    SendString("\r\n\n%s: ", CATSTR(MSG_FORUM_CREATE_NAME));
    if(GetString(40,NULL)) {
      return 1;
    }
    strcpy(tmpConf.namn, inmat);
  } else {
    strcpy(tmpConf.namn, argument);
  }
  if(parsemot(tmpConf.namn) != -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUM_CREATE_ALREADY_EXISTS));
    return 0;
  }
  tmpConf.skapat_tid = time(NULL);;
  tmpConf.skapat_av = inloggad;
  for(;;) {
    SendString("\r\n%s: ", CATSTR(MSG_FORUM_CREATE_ADMIN));
    if(GetString(5,NULL)) {
      return 1;
    }
    if(inmat[0]) {
      if((mad = parsenamn(inmat)) == -1) {
        SendString("\r\n%s", CATSTR(MSG_COMMON_NO_SUCH_USER));
      } else {
        tmpConf.mad = mad;
        break;
      }
    }
  }
  SendString("\n\r%s: ", CATSTR(MSG_FORUM_CREATE_SORT));
  tmpConf.sortpri = GetNumber(0, LONG_MAX, NULL);

  if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_CLOSED),
                      CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      CATSTR(MSG_FORUM_CREATE_CLOSED_YES), CATSTR(MSG_FORUM_CREATE_CLOSED_NO),
                      &tmpConf.status, SLUTET)) {
    return 1;
  }
  if(tmpConf.status & SLUTET) {
    SendString("\r\n%s\r\n", CATSTR(MSG_FORUM_CREATE_GROUPS));
    if(editgrupp((char *)&tmpConf.grupper)) {
      return 1;
    }
  }
  if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_WRITE),
                      CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      CATSTR(MSG_FORUM_CREATE_PROTECTED), CATSTR(MSG_FORUM_CREATE_UNPROTECTED),
                      &tmpConf.status, SKRIVSKYDD)) {
    return 1;
  }
  if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_COMMENT),
                      CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      CATSTR(MSG_FORUM_CREATE_PROTECTED), CATSTR(MSG_FORUM_CREATE_UNPROTECTED),
                      &tmpConf.status, KOMSKYDD)) {
    return 1;
  }
  if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_SECRET),
                      CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      CATSTR(MSG_FORUM_CREATE_SECRET_YES), CATSTR(MSG_FORUM_CREATE_SECRET_NO),
                      &tmpConf.status, HEMLIGT)) {
    return 1;
  }
  if(!(tmpConf.status & SLUTET)) {
    if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_AUTO),
                        NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                        &tmpConf.status, AUTOMEDLEM)) {
      return 1;
    }
    if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_PERMWRITE),
                        NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                        &tmpConf.status, SKRIVSTYRT)) {
      return 1;
    }
    if(tmpConf.status & SKRIVSTYRT) {
      SendString("\r\n%s\r\n", CATSTR(MSG_FORUM_CREATE_GROUPS));
      if(editgrupp((char *)&tmpConf.grupper)) {
        return 1;
      }
    }
  }
  if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_AREXX),
                      NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      &tmpConf.status, SUPERHEMLIGT)) {
    return 1;
  }

  SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_TYPE_HEAD));
  SendString("1: %s\n\r", CATSTR(MSG_FORUM_CREATE_TYPE_LOCAL));
  SendString("2: %s\n\n\r", CATSTR(MSG_FORUM_CREATE_TYPE_FIDO));
  SendString("Val: ");
  for(;;) {
    ch = GetChar();
    if(ch == GETCHAR_LOGOUT) {
      return 1;
    }
    if(ch == '1' || ch == '2') {
      break;
    }
  }
  if(ch == '1') {
    SendString("%s\n\n\r", CATSTR(MSG_FORUM_CREATE_TYPE_LOCAL));
    tmpConf.type = MOTE_ORGINAL;
  } else {
    SendString("%s\n\n\r", CATSTR(MSG_FORUM_CREATE_TYPE_FIDO));
    tmpConf.type = MOTE_FIDO;
    if(EditString(CATSTR(MSG_FORUM_CREATE_FIDO_DIR), tmpConf.dir, 79, TRUE)) {
      return 1;
    }
    if(!(lock = Lock(tmpConf.dir, SHARED_LOCK))) {
      if(!(lock = CreateDir(tmpConf.dir)))
        SendString("\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_FIDO_DIR_ERROR));
    }
    if(lock) {
      UnLock(lock);
    }
    if(EditString(CATSTR(MSG_FORUM_CREATE_FIDO_TAG), tmpConf.tagnamn, 49, TRUE)) {
      return 1;
    }
    strcpy(tmpConf.origin, Servermem->cfg->fidoConfig.defaultorigin);
    if(MaybeEditString(CATSTR(MSG_FORUM_CREATE_FIDO_ORIGIN), tmpConf.origin, 69)) {
      return 1;
    }
    
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_FIDO_CHR));
    SendString("1: %s\n\r", CATSTR(MSG_CHRS_ISO88591));
    SendString("2: %s\n\r", CATSTR(MSG_CHRS_SIS7));
    SendString("3: %s\n\r", CATSTR(MSG_CHRS_CP437));
    SendString("4: %s\n\n\r", CATSTR(MSG_CHRS_MAC));
    SendString("%s ", CATSTR(MSG_COMMON_CHOICE));
    for(;;) {
      ch = GetChar();
      if(ch == GETCHAR_LOGOUT) {
        return 1;
      }
      if(ch == '1' || ch == '2' || ch == '3' || ch == '4') {
        break;
      }
    }
    switch(ch) {
    case '1':
      SendString("%s\n\n\r", CATSTR(MSG_CHRS_ISO88591));
      tmpConf.charset = CHRS_LATIN1;
      break;
    case '2':
      SendString("%s\n\n\r", CATSTR(MSG_CHRS_SIS7));
      tmpConf.charset = CHRS_SIS7;
      break;
    case '3':
      SendString("%s\n\n\r", CATSTR(MSG_CHRS_CP437));
      tmpConf.charset = CHRS_CP437;
      break;
    case '4':
      SendString("%s\n\n\r", CATSTR(MSG_CHRS_MAC));
      tmpConf.charset = CHRS_MAC;
      break;
    }
    SendString("%s\n\r", CATSTR(MSG_FORUM_CREATE_DOMAIN));
    highestId = 0;
    for(i = 0; i < 10; i++) {
      if(!Servermem->cfg->fidoConfig.fd[i].domain[0]) {
        break;
      }
      highestId = max(highestId, Servermem->cfg->fidoConfig.fd[i].nummer);
      SendString("%3d: %s (%d:%d/%d.%d)\n\r",
                 Servermem->cfg->fidoConfig.fd[i].nummer,
                 Servermem->cfg->fidoConfig.fd[i].domain,
                 Servermem->cfg->fidoConfig.fd[i].zone,
                 Servermem->cfg->fidoConfig.fd[i].net,
                 Servermem->cfg->fidoConfig.fd[i].node,
                 Servermem->cfg->fidoConfig.fd[i].point);
    }
    if(i == 0) {
      SendString("\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_NO_DOMAIN));
      return 0;
    }
    for(;;) {
      SendString("\r\n%s: ", CATSTR(MSG_FORUM_CREATE_DOMAIN_CHOICE));
      if(GetString(5, NULL)) {
        return 1;
      }
      fidoDomainId = atoi(inmat);
      if((fidoDomain = getfidodomain(fidoDomainId, 0))) {
        break;
      } else {
        SendString("\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_DOMAIN_NF));
      }
    }
    tmpConf.domain = fidoDomain->nummer;
    SendString("%s\n\n\r", fidoDomain->domain);
  }
  for(i = 0; i < MAXMOTE; i++) {
    if(getmotpek(i) == NULL) {
      break;
    }
  }
  if(i >= MAXMOTE) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_NO_ROOM));
    return 0;
  }
  tmpConf.nummer = i;
  if(!(newConf = (struct Mote *)AllocMem(sizeof(struct ExtMote),
                                         MEMF_CLEAR | MEMF_PUBLIC))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.", sizeof(struct ExtMote));
    DisplayInternalError();
    return 0;
  }
  memcpy(newConf, &tmpConf, sizeof(struct Mote));
  InitSemaphore(&((struct ExtMote *)newConf)->fidoCommentsSemaphore);

  ITER_EL(searchConf, Servermem->mot_list, mot_node, struct Mote *) {
    if(searchConf->sortpri > newConf->sortpri) {
      break;
    }
  }
  searchConf = (struct Mote *)searchConf->mot_node.mln_Pred;
  Insert((struct List *)&Servermem->mot_list, (struct Node *)newConf,
         (struct Node *)searchConf);
  writemeet(newConf);

  if((newConf->status & AUTOMEDLEM) && !(newConf->status & SKRIVSTYRT)) {
    return 0;
  }
  if(newConf->status & SUPERHEMLIGT) {
    return 0;
  }

  setPermission = (newConf->status & (SLUTET | SKRIVSTYRT)) ? FALSE : TRUE;

  SendString("\r\n%s\r\n", CATSTR(MSG_FORUM_CREATE_CHANGING));
  ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
    if(!(shortUser->nummer % 10)) {
      SendString("\r%d", shortUser->nummer);
    }
    if(!(user = GetUserDataForUpdate(shortUser->nummer, &needsWrite))) {
      return 0;
    }
    changed = FALSE;
    if(setPermission != BAMTEST(user->motratt, newConf->nummer)) {
      if(setPermission) {
        BAMSET(user->motratt, newConf->nummer);
      } else {
        BAMCLEAR(user->motratt, newConf->nummer);
      }
      changed = TRUE;
    }
    if(!(newConf->status & AUTOMEDLEM) && BAMTEST(user->motmed, newConf->nummer)) {
      BAMCLEAR(user->motmed, newConf->nummer);
      changed = TRUE;
    }
    if(changed && needsWrite && !WriteUser(shortUser->nummer, user, FALSE)) {
      return 0;
    }
    
  }

    BAMSET(CURRENT_USER->motratt, newConf->nummer);
  BAMSET(CURRENT_USER->motmed, newConf->nummer);
  if(newConf->type == MOTE_FIDO) {
    ReScanFidoConf(newConf, 0);
  }
  return 0;
}

void listmot(char *pattern) {
  struct Mote *conf;
  int cnt, usingPattern = 0, patret;
  char parsedPattern[101];

  SendString("\r\n\n");
 
  if(pattern[0]) {
    patret = ParsePatternNoCase(pattern, parsedPattern, 98);
    if(patret != 1) {
      SendString("%s\r\n", CATSTR(MSG_FORUM_LIST_PATTERN));
      return;
    }
    usingPattern = 1;
  }

  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(!MaySeeConf(conf->nummer,inloggad,CURRENT_USER)) {
      continue;
    }
    if(usingPattern && !MatchPatternNoCase(parsedPattern, conf->namn)) {
      continue;
    }
    cnt = 0;
    if(IsMemberConf(conf->nummer, inloggad, CURRENT_USER)) {
      if(SendString("%3d %s ", countunread(conf->nummer), conf->namn)) { return; }
    } else {
      if(SendString("   *%s ",conf->namn)) { return; }
    }
    if(conf->status & SLUTET) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_CLOSED))) { return; }
    }
    if(conf->status & SKRIVSKYDD) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_WRITEPROT))) { return; }
    }
    if(conf->status & KOMSKYDD) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_COMMPROT))) { return; }
    }
    if(conf->status & HEMLIGT) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_SECRET))) { return; }
    }
    if(conf->status & AUTOMEDLEM) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_AUTO))) { return; }
    }
    if(conf->status & SKRIVSTYRT) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_WRITECTRL))) { return; }
    }
    if((conf->status & SUPERHEMLIGT) && CURRENT_USER->status >= Servermem->cfg->st.medmoten) {
      if(SendString(" (%s)", CATSTR(MSG_FORUM_LIST_AREXX))) { return; }
    }
    if(SendString("\r\n")) { return; }
  }
  
  if(Servermem->info.hightext > -1) {
    SendStringCat("\r\n\n%s\r\n\n", CATSTR(MSG_FORUM_LIST_SUMMARY),
                  Servermem->info.lowtext, Servermem->info.hightext);
  } else {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUM_LIST_NOTEXTS));
  }
}

int parsemot(char *skri) {
        struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        int going2=TRUE,found=-1;
        char *faci,*skri2;
        if(skri[0]==0 || skri[0]==' ') return(-3);
        for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
                if(!MaySeeConf(motpek->nummer, inloggad, CURRENT_USER)) continue;
                faci=motpek->namn;
                skri2=skri;
                going2=TRUE;
                if(matchar(skri2,faci)) {
                        while(going2) {
                                skri2=hittaefter(skri2);
                                faci=hittaefter(faci);
                                if(skri2[0]==0) return((int)motpek->nummer);
                                else if(faci[0]==0) going2=FALSE;
                                else if(!matchar(skri2,faci)) {
                                        faci=hittaefter(faci);
                                        if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
                                }
                        }
                }
        }
        return(found);
}

void medlem(char *args) {
  struct UnreadTexts *unreadTexts = CUR_USER_UNREAD;
  struct Mote *conf;
  BPTR lock;
  int confId;
  char filename[40];

  if(args[0] == '-' && (args[1] == 'a' || args[1] == 'A')) {
    ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
      if(MayBeMemberConf(conf->nummer, inloggad, CURRENT_USER)
         && !IsMemberConf(conf->nummer, inloggad, CURRENT_USER)) {
        BAMSET(CURRENT_USER->motmed, conf->nummer);
        if(conf->type == MOTE_ORGINAL) {
          unreadTexts->lowestPossibleUnreadText[conf->nummer] = 0;
        }
        else if(conf->type == MOTE_FIDO) {
          unreadTexts->lowestPossibleUnreadText[conf->nummer] = conf->lowtext;
        }
      }
    }
    return;
  }

  confId = parsemot(args);
  if(confId == -3) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_MEMBER_SYNTAX));
    return;
  }
  if(confId == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
    return;
  }
  if(!MayBeMemberConf(confId, inloggad, CURRENT_USER)) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_MEMBER_NO_PERM));
    return;
  }
  BAMSET(CURRENT_USER->motmed, confId);
  SendStringCat("\r\n\n%s\r\n\n", CATSTR(MSG_MEMBER_NOW_MEMBER), getmotnamn(confId));
  conf = getmotpek(confId);
  if(conf->type == MOTE_ORGINAL) {
    unreadTexts->lowestPossibleUnreadText[confId] = 0;
  }
  else if(conf->type == MOTE_FIDO) {
    unreadTexts->lowestPossibleUnreadText[confId] = conf->lowtext;
  }
  sprintf(filename,"NiKom:Lappar/%d.motlapp", confId);
  if((lock = Lock(filename,ACCESS_READ))) {
    UnLock(lock);
    sendfile(filename);
  }
}

void uttrad(char *args) {
  struct Mote *conf;
  int confId;
  
  if(args[0] == '-' && (args[1] == 'a' || args[1] == 'A')) {
    ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
      if(IsMemberConf(conf->nummer, inloggad, CURRENT_USER))
        BAMCLEAR(CURRENT_USER->motmed, conf->nummer);
    }
    return;
  }

  confId = parsemot(args);
  if(confId == -3) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_LEAVE_SYNTAX));
    return;
  }
  if(confId == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
    return;
  }
  conf = getmotpek(confId);
  if(conf->status & AUTOMEDLEM) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_LEAVE_CAN_NOT));
    return;
  }
  if(!IsMemberConf(confId, inloggad, CURRENT_USER)) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_LEAVE_NOT_MEMBER));
    return;
  }
  if(confId == mote2) {
    mote2 = -1;
  }
  BAMCLEAR(CURRENT_USER->motmed, confId);
  SendStringCat("\r\n\n%s\r\n\n", CATSTR(MSG_LEAVE_HAVE_LEFT), getmotnamn(confId));
}

int countunread(int conf) {
  struct Mote *motpek;
  if(conf == -1) {
    return(countmail(inloggad,CURRENT_USER->brevpek));
  }
  motpek=getmotpek(conf);
  if(motpek->type==MOTE_ORGINAL) {
    return CountUnreadTexts(conf, CUR_USER_UNREAD);
  }
  if(motpek->type==MOTE_FIDO) {
    return countfidomote(motpek);
  }
  return 0;
}

void trimLowestPossibleUnreadTextsForFido(void) {
  struct UnreadTexts *unreadTexts = CUR_USER_UNREAD;
  struct Mote *motpek;
  ITER_EL(motpek, Servermem->mot_list, mot_node, struct Mote *) {
    if(motpek->type==MOTE_FIDO) {
      if(unreadTexts->lowestPossibleUnreadText[motpek->nummer] < motpek->lowtext) {
        unreadTexts->lowestPossibleUnreadText[motpek->nummer] = motpek->lowtext;
      }
    }
  }
}

void connection(void) {
  char tellstr[100];
  ClearUserMessages(g_userDataSlot);
  NewList((struct List *)&aliaslist);
  trimLowestPossibleUnreadTextsForFido();
  time(&logintime);
  extratime=0;
  memset(&Statstr,0,sizeof(struct Inloggning));
  Statstr.anv=inloggad;
  mote2=-1;
  LoadCatalogForUser(CURRENT_USER);
  Servermem->nodeInfo[nodnr].action = GORNGTANNAT;
  strcpy(vilkabuf,"loggar in");
  Servermem->nodeInfo[nodnr].currentActivity = vilkabuf;
  senast_text_typ=0;
  if(Servermem->cfg->logmask & LOG_INLOGG) {
    LogEvent(USAGE_LOG, INFO, "%s loggar in på nod %d",
             getusername(inloggad), nodnr);
  }
  sprintf(tellstr,"loggade just in på nod %d",nodnr);
  SendUserMessage(inloggad, -1, tellstr, NIK_MESSAGETYPE_LOGNOTIFY);
  area2=CURRENT_USER->defarea;
  if(area2<0 || area2>Servermem->info.areor || !Servermem->areor[area2].namn || !arearatt(area2, inloggad, CURRENT_USER)) area2=-1;
  initgrupp();
  rxlinecount = TRUE;
  radcnt=0;
  Servermem->waitingNotifications[g_userDataSlot] = CountNotifications(inloggad);
  if(Servermem->cfg->ar.postinlogg) sendautorexx(Servermem->cfg->ar.postinlogg);
  DisplayVersionInfo();
  var(mote2);
  KomLoop();
}
