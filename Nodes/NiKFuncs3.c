#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Terminal.h"
#include "KOM.h"
#include "Logging.h"
#include "StringUtils.h"
#include "Stack.h"
#include "Languages.h"
#include "DateUtils.h"

#define EKO		1
#define EJEKO	0

extern struct System *Servermem;
extern int nodnr,inloggad,mote2,senast_text_typ,senast_text_nr,senast_text_mote,nu_skrivs,rad,connectbps;
extern char outbuffer[],inmat[],*argument;
extern struct Header sparhead,readhead;
extern struct MsgPort *permitport,*serverport,*NiKomPort;
extern struct Inloggning Statstr;
extern long logintime;
extern struct MinList edit_list;
extern struct ReadLetter brevspar;

void endast(void) {
  int amount, mailId;
  int confId;
  struct Mote *conf = NULL;

  if(!IzDigit(argument[0])) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_ONLY_SYNTAX));
    return;
  }
  amount = atoi(argument);
  argument = hittaefter(argument);
  if(argument[0]) {
    if(matchar(argument, CATSTR(MSG_MAIL_MAILBOX))) {
      confId = -1;
    } else {
      confId = parsemot(argument);
      if(confId == -3) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_ONLY_SYNTAX));
        return;
      }
      if(confId == -1) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
        return;
      }
    }
  }
  else {
    confId = mote2;
  }
  
  if(confId == -1) {
    Servermem->inne[nodnr].brevpek = getnextletter(inloggad) - amount;
    mailId = getfirstletter(inloggad);
    if(mailId > Servermem->inne[nodnr].brevpek) {
      Servermem->inne[nodnr].brevpek = mailId;
    }
  } else {
    conf = getmotpek(confId);
    if(conf->type == MOTE_ORGINAL) {
      org_endast(confId,amount);
    } else if(conf->type == MOTE_FIDO) {
      fido_endast(conf,amount);
    }
  }
  if(confId == mote2) {
    StackClear(g_unreadRepliesStack);
    var(mote2);
  }
}

int personlig(void) {
  int nummer, editret, confId;
  struct Mote *conf;
  if(argument[0]) {
    if(mote2 == -1) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_PERSONAL_MAIL));
      return 0;
    }
    nummer = atoi(argument);
    conf = getmotpek(mote2);
    if(conf->type == MOTE_ORGINAL) {
      if(nummer<Servermem->info.lowtext || nummer>Servermem->info.hightext) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
        return 0;
      }
      confId = GetConferenceForText(nummer);
      if(!MayBeMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMENT_NO_PERMISSIONS));
        return 0;
      }
      if(readtexthead(nummer, &readhead)) {
        return 0;
      }
      senast_text_mote = confId;
    } else if(conf->type == MOTE_FIDO) {
      if(nummer<conf->lowtext || nummer>conf->texter) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
        return 0;
      }
      senast_text_mote = conf->nummer;
    }
    senast_text_typ=TEXT;
    senast_text_nr = nummer;
  } else {
    if(senast_text_typ==BREV) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_PERSONAL_MAIL));
      return 0;
    }
    if(!(conf = getmotpek(senast_text_mote))) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't find conference %d in memory in personlig().", senast_text_mote);
      DisplayInternalError();
      return 0;
    }
  }
  if(conf->type == MOTE_FIDO) {
    nu_skrivs = BREV_FIDO;
    return(fido_brev(NULL, NULL, conf));
  }
  nu_skrivs = BREV;
  initpersheader();
  if((editret = edittext(NULL)) == 1) {
    return 1;
  }
  if(!editret) {
    sparabrev();
  }
  return 0;
}

void radtext(void) {
  int textId;
  struct Header radhead;
  struct Mote *conf;
  textId = atoi(argument);
  conf = getmotpek(GetConferenceForText(textId));
  if(conf->type != MOTE_ORGINAL) {
    SendString("\n\n\r%s\n\n\r", CATSTR(MSG_DELTEXT_ONLY_LOCAL));
    return;
  }
  if(textId < Servermem->info.lowtext || textId > Servermem->info.hightext) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return;
  }
  if(readtexthead(textId,&radhead)) {
    return;
  }
  if(radhead.person!=inloggad && !MayAdminConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_DELTEXT_ONLY_SELF));
    return;
  }
  SetConferenceForText(textId, -1, TRUE);
  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_DELTEXT_DELETED), textId);
}

int radmot(void) {
  int confId, textNumber, isCorrect;
  char lappfile[40];
  struct Mote *conf;
  if((confId = parsemot(argument)) == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
    return -5;
  }
  if(confId == -3) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_DELFORUM_SYNTAX));
    return -5;
  }
  conf = getmotpek(confId);
  if(conf == NULL) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't find conference %d in memory", confId);
    DisplayInternalError();
    return -5;
  }
  if(!MayAdminConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_DELFORUM_NOPERM));
    return -5;
  }

  SendStringCat("\r\n\n%s ", CATSTR(MSG_DELFORUM_CONFIRM), conf->namn);
  if(GetYesOrNo(NULL, NULL, NULL, NULL,
                CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                "\r\n", FALSE, &isCorrect)) {
    return -8;
  }
  if(!isCorrect) {
    return -5;
  }

  Remove((struct Node *)conf);
  conf->namn[0] = 0;
  writemeet(conf);
  FreeMem(conf, sizeof(struct Mote));
  sprintf(lappfile, "NiKom:Lappar/%d.motlapp", confId);
  DeleteFile(lappfile);
  
  textNumber = 0;
  while((textNumber = FindNextTextInConference(textNumber, confId)) != -1) {
    SetConferenceForText(textNumber, -1, FALSE);
    textNumber++;
  }

  if(!WriteConferenceTexts()) {
    LogEvent(SYSTEM_LOG, ERROR, "Error writing TextMot.dat");
    DisplayInternalError();
  }
  return(mote2 == confId ? -9 : -5);
}

int andmot(void) {
  int confId, tmpConfId, permissionsNarrowed = FALSE, mad,
    clearMembership, setPermission, userChanged, ch, i, isCorrect;
  struct ShortUser *shortUser;
  struct FidoDomain *domain;
  struct User skuser;
  struct Mote tmpConf, *conf;

  if((confId = parsemot(argument)) == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
    return 0;
  } else if(confId == -3) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUM_CHANGE_SYNTAX));
    return 0;
  }
  conf = getmotpek(confId);
  if(!MayAdminConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUM_CHANGE_NOPERM));
    return 0;
  }
  memcpy(&tmpConf, conf, sizeof(struct Mote));
  for(;;) {
    SendString("\r\n%s : (%s) ", CATSTR(MSG_FORUM_CREATE_NAME), tmpConf.namn);
    if(GetString(40, NULL)) {
      return 1;
    }
    if(inmat[0] == '\0') {
      break;
    }
    if((tmpConfId = parsemot(inmat)) != -1 && tmpConfId != confId) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUM_CREATE_ALREADY_EXISTS));
    } else {
      strncpy(tmpConf.namn, inmat, 41);
      break;
    }
  }
  SendString("\r\nMAD (%s) ", getusername(tmpConf.mad));
  if(GetString(40,NULL)) {
    return 1;
  }
  if(inmat[0]) {
    if((mad = parsenamn(inmat)) == -1) {
      SendString("\r\n%s", CATSTR(MSG_COMMON_NO_SUCH_USER));
    } else {
      tmpConf.mad=mad;
    }
  }
  if(MaybeEditNumber(CATSTR(MSG_FORUM_CREATE_SORT), (int *)&tmpConf.sortpri, 8, 0, LONG_MAX)) {
    return 1;
  }
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
  } else {
    tmpConf.status &= ~(AUTOMEDLEM | SKRIVSTYRT);
  }
  if(EditBitFlagShort("\r\n", CATSTR(MSG_FORUM_CREATE_AREXX),
                      NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                      &tmpConf.status, SUPERHEMLIGT)) {
    return 1;
  }

  if(tmpConf.type == MOTE_FIDO) {
    if(MaybeEditString(CATSTR(MSG_FORUM_CREATE_FIDO_DIR), tmpConf.dir, 79)) { return 1; }
    if(MaybeEditString(CATSTR(MSG_FORUM_CREATE_FIDO_TAG), tmpConf.tagnamn, 49)) { return 1; }
    if(MaybeEditString(CATSTR(MSG_FORUM_CREATE_FIDO_ORIGIN), tmpConf.origin, 69)) { return 1; }
    
    SendString("\n\n\r%c1: %s\n\r",
               tmpConf.charset == CHRS_LATIN1 ? '*' : ' ', CATSTR(MSG_CHRS_ISO88591));
    SendString("%c2: %s\n\r",
               tmpConf.charset == CHRS_SIS7 ? '*' : ' ', CATSTR(MSG_CHRS_SIS7));
    SendString("%c3: %s\n\r", tmpConf.charset == CHRS_CP437 ? '*' : ' ', CATSTR(MSG_CHRS_CP437));
    SendString("%c4: %s\n\n\r", tmpConf.charset == CHRS_MAC ? '*' : ' ', CATSTR(MSG_CHRS_MAC));
    SendString("Val: ");
    for(;;) {
      ch = GetChar();
      if(ch == GETCHAR_LOGOUT) {
        return 1;
      }
      if(ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == GETCHAR_RETURN) {
        break;
      }
    }
    switch(ch) {
    case '1' : tmpConf.charset = CHRS_LATIN1; break;
    case '2' : tmpConf.charset = CHRS_SIS7; break;
    case '3' : tmpConf.charset = CHRS_CP437; break;
    case '4' : tmpConf.charset = CHRS_MAC; break;
    default: break;
    }
    SendString("\n\n\r");

    for(i = 0; i < 10; i++) {
      if(!Servermem->cfg->fidoConfig.fd[i].domain[0]) {
        break;
      }
      SendString("%c%3d: %s (%d:%d/%d.%d)\n\r",
                 tmpConf.domain == Servermem->cfg->fidoConfig.fd[i].nummer ? '*' : ' ',
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
      if(GetString(10,NULL)) {
        return(1);
      }
      if(inmat[0] == '\0') {
        break;
      }
      if((domain = getfidodomain(atoi(inmat), 0))) {
        tmpConf.domain = domain->nummer;
        break;
      } else {
        SendString("\n\r%s\n\r", CATSTR(MSG_FORUM_CREATE_DOMAIN_NF));
      }
    }
  }
  
  if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMON_IS_CORRECT), NULL, NULL,
                CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n\n",
                TRUE, &isCorrect)) {
    return 1;
  }
  if(!isCorrect) {
    return 0;
  }

  permissionsNarrowed =
    (!(conf->status & SLUTET) && tmpConf.status & SLUTET)
    || (conf->status & AUTOMEDLEM && !(tmpConf.status & AUTOMEDLEM))
    || (!(conf->status & SKRIVSTYRT) && tmpConf.status & SKRIVSTYRT);

  if(permissionsNarrowed) {
    SendString("%s\n\r", CATSTR(MSG_FORUM_CHANGE_RESET_1));
    SendString("%s\n\n\r", CATSTR(MSG_FORUM_CHANGE_RESET_2));
    if(GetYesOrNo(NULL, CATSTR(MSG_FORUM_CHANGE_STILL_CORRECT), NULL, NULL,
                  CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n", TRUE,
                  &isCorrect)) {
      return 1;
    }
    if(!isCorrect) {
      return 0;
    }
  }
  memcpy(conf, &tmpConf, sizeof(struct Mote));
  writemeet(conf);
  if(!permissionsNarrowed) {
    return 0;
  }
  if((tmpConf.status & AUTOMEDLEM) && !(tmpConf.status & SKRIVSTYRT)) {
    return 0;
  }
  if(tmpConf.status & SUPERHEMLIGT) {
    return 0;
  }
  if(tmpConf.status & AUTOMEDLEM) {
    clearMembership=FALSE;
  } else {
    clearMembership=TRUE;
  }
  if(tmpConf.status & (SLUTET | SKRIVSTYRT)) {
    setPermission=FALSE;
  } else {
    setPermission=TRUE;
  }
  for(i = 0; i < MAXNOD; i++) {
    BAMCLEAR(Servermem->inne[i].motmed, tmpConf.nummer);
    if(setPermission) {
      BAMSET(Servermem->inne[i].motratt,tmpConf.nummer);
    } else {
      BAMCLEAR(Servermem->inne[i].motratt,tmpConf.nummer);
    }
  }
  SendString("\r\n%s\r\n", CATSTR(MSG_FORUM_CHANGE_RESETTING));
  ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
    if(!(shortUser->nummer % 10)) {
      SendString("\r%d", shortUser->nummer);
    }
    if(readuser(shortUser->nummer,&skuser)) {
      LogEvent(SYSTEM_LOG, ERROR, "Can't read user %d", shortUser->nummer);
      DisplayInternalError();
      return 0;
    }
    userChanged = FALSE;
    if(setPermission != BAMTEST(skuser.motratt, tmpConf.nummer)) {
      if(setPermission) {
        BAMSET(skuser.motratt, tmpConf.nummer);
      } else {
        BAMCLEAR(skuser.motratt, tmpConf.nummer);
      }
      userChanged = TRUE;
    }
    if(clearMembership && BAMTEST(skuser.motmed, tmpConf.nummer)) {
      BAMCLEAR(skuser.motmed, tmpConf.nummer);
      userChanged = TRUE;
    }
    if(userChanged && writeuser(shortUser->nummer,&skuser)) {
      LogEvent(SYSTEM_LOG, ERROR, "Can't write user %d", shortUser->nummer);
      DisplayInternalError();      
      return 0;
    }
    
  }
  for(i = 0; i < MAXNOD; i++) {
    BAMCLEAR(Servermem->inne[i].motmed, tmpConf.nummer);
    if(setPermission) {
      BAMSET(Servermem->inne[i].motratt,tmpConf.nummer);
    } else {
      BAMCLEAR(Servermem->inne[i].motratt,tmpConf.nummer);
    }
  }
  BAMSET(Servermem->inne[nodnr].motratt,tmpConf.nummer);
  BAMSET(Servermem->inne[nodnr].motmed,tmpConf.nummer);
  return 0;
}

void radbrev(void) {
  SendString("\n\n\r%s\n\r", CATSTR(MSG_MAIL_DELETE));
}

void vilka(void) {
  int i, verbose = FALSE, allNodes = FALSE, userLoggedIn;
  long timenow;
  int idle;
  struct Mote *conf;
  char name[50], actionbuf[100], idlebuf[20];

  if(argument[0] == '-') {
    for(i = 1; argument[i] != '\0' && argument[i] != ' '; i++) {
      switch(argument[i]) {
      case 'a': case 'A':
        allNodes = TRUE;
        break;
      case 'v': case 'V':
        verbose = TRUE;
        break;
      }
    }
  }
  SendString("\r\n\n");
  timenow = time(NULL);
  for(i = 0; i < MAXNOD; i++) {
    if(!Servermem->nodtyp[i] || (!allNodes && Servermem->inloggad[i] == -1)) {
      continue;
    }
    userLoggedIn = Servermem->inloggad[i] >= 0;
    if(Servermem->inloggad[i] == -1) {
      sprintf(name, "<%s>", CATSTR(MSG_WHO_NOONE));
    } else if(Servermem->inloggad[i] == -2) {
      sprintf(name, "<%s>", CATSTR(MSG_WHO_CONNECTED));
    } else {
      strcpy(name, getusername(Servermem->inloggad[i]));
    }

    idle = timenow - Servermem->idletime[i];
    SendString("%s #%-2d %-40s %c %7s\n\r",
               CATSTR(MSG_WHO_NODE), i,
               name,
               Servermem->say[i] ? '*' : ' ',
               (idle < 60 && userLoggedIn) ? (const char *)CATSTR(MSG_WHO_ACTIVE) : FormatDuration(idle, idlebuf));

    if(!verbose) {
      continue;
    }
    if(!userLoggedIn) {
      SendString("\n");
      continue;
    }

    SendString("  %s %-20s ",
               Servermem->nodtyp[i] == NODCON ? "CON" : "SER",
               Servermem->nodid[i]);
    switch(Servermem->action[i]) {
    case INGET :
      strcpy(actionbuf, CATSTR(MSG_WHO_NO_UNREAD));
      break;
    case SKRIVER :
      if(Servermem->varmote[i] !=- 1) {
        conf = getmotpek(Servermem->varmote[i]);
        if(!conf) {
          continue;
        }
        if(!MaySeeConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {
          strcpy(actionbuf, CATSTR(MSG_WHO_WRITES_TEXT));
        } else {
          sprintf(actionbuf, "%s %s", CATSTR(MSG_WHO_WRITES_IN), conf->namn);
        }
      } else {
        strcpy(actionbuf, CATSTR(MSG_WHO_WRITES_MAIL));
      }
      break;
    case LASER :
      if(Servermem->varmote[i] != -1) {
        conf = getmotpek(Servermem->varmote[i]);
        if(!conf) {
          continue;
        }
        if(!MaySeeConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {
          strcpy(actionbuf,CATSTR(MSG_WHO_READS_TEXTS));
        } else {
          sprintf(actionbuf, "%s %s", CATSTR(MSG_WHO_READS_IN), conf->namn);
        }
      } else {
        strcpy(actionbuf, CATSTR(MSG_WHO_READS_MAIL));
      }
      break;
    case GORNGTANNAT :
      strcpy(actionbuf, Servermem->vilkastr[i]);
      break;
    case UPLOAD :
      if(!Servermem->areor[Servermem->varmote[i]].namn[0]
         || !arearatt(Servermem->varmote[i], inloggad, &Servermem->inne[nodnr])) {
        strcpy(actionbuf, CATSTR(MSG_WHO_UPLOADING));
      } else {
        if(Servermem->vilkastr[i]) {
          sprintf(actionbuf, "%s %s", CATSTR(MSG_WHO_UPLOADING), Servermem->vilkastr[i]);
        } else {
          strcpy(actionbuf, CATSTR(MSG_WHO_SOON_UPLOAD));
        }
      }
      break;
    case DOWNLOAD :
      if(!Servermem->areor[Servermem->varmote[i]].namn[0]
         || !arearatt(Servermem->varmote[i], inloggad, &Servermem->inne[nodnr])) {
        strcpy(actionbuf, CATSTR(MSG_WHO_DOWNLOADING));
      } else {
        if(Servermem->vilkastr[i]) {
          sprintf(actionbuf, "%s %s", CATSTR(MSG_WHO_DOWNLOADING), Servermem->vilkastr[i]);
        } else {
          strcpy(actionbuf, CATSTR(MSG_WHO_SOON_DOWNLOAD));
        }
      }
      break;
    default :
      strcpy(actionbuf, CATSTR(MSG_WHO_UNDEFINED));
      break;
    }
    SendString("%s\r\n\n", actionbuf);
  }
}

int readtexthead(int textId, struct Header *head) {
  BPTR fp;
  int fil, pos;
  char filename[40];
  fil = textId / 512;
  pos = textId % 512;
  sprintf(filename, "NiKom:Moten/Head%d.dat", fil);
  NiKForbid();
  if(!(fp = Open(filename,MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't open %s for text %d.", filename, textId);
    DisplayInternalError();
    NiKPermit();
    return 1;
  }
  if(Seek(fp, pos * sizeof(struct Header), OFFSET_BEGINNING) == -1L) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't seek to position for text %d in %s.", textId, filename);
    DisplayInternalError();
    Close(fp);
    NiKPermit();
    return 1;
  }
  if(!(Read(fp, (void *)head, sizeof(struct Header)))) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't read text %d in %s.", textId, filename);
    DisplayInternalError();
    Close(fp);
    NiKPermit();
    return 1;
  }
  Close(fp);
  NiKPermit();
  return 0;
}

int writetexthead(int textId, struct Header *head) {
  BPTR fh;
  int fil, pos;
  char filename[40];
  fil = textId / 512;
  pos = textId % 512;
  sprintf(filename, "NiKom:Moten/Head%d.dat", fil);
  NiKForbid();
  if(!(fh = Open(filename,MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't open %s for text %d.", filename, textId);
    DisplayInternalError();
    NiKPermit();
    return 1;
  }
  if(Seek(fh, pos * sizeof(struct Header), OFFSET_BEGINNING) == -1) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't seek to position for text %d in %s.", textId, filename);
    DisplayInternalError();
    Close(fh);
    NiKPermit();
    return 1;
  }
  if(Write(fh, (void *)head,sizeof(struct Header)) == -1) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Can't write text %d in %s.", textId, filename);
    DisplayInternalError();
    Close(fh);
    NiKPermit();
    return 1;
  }
  Close(fh);
  NiKPermit();
  return 0;
}

int getnextletter(int user) {
  BPTR fh;
  char nrstr[20], filename[50];
  sprintf(filename, "NiKom:Users/%d/%d/.nextletter", user / 100, user);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    DisplayInternalError();
    return -1;
  }
  memset(nrstr, 0, 20);
  if(Read(fh, nrstr, 19) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't read %s", filename);
    DisplayInternalError();
    Close(fh);
    return -1;
  }
  Close(fh);
  return atoi(nrstr);
}

int getfirstletter(int user) {
  BPTR fh;
  char nrstr[20], filename[50];
  sprintf(filename, "NiKom:Users/%d/%d/.firstletter", user / 100, user);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    DisplayInternalError();
    return -1;
  }
  memset(nrstr, 0, 20);
  if(Read(fh, nrstr, 19) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't read %s", filename);
    DisplayInternalError();
    Close(fh);
    return -1;
  }
  Close(fh);
  return atoi(nrstr);
}

int readuser(int userId,struct User *user) {
  BPTR fh;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  NiKForbid();
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    DisplayInternalError();
    NiKPermit();
    return 1;
  }
  if(Read(fh, (void *)user,sizeof(struct User)) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't read %s", filename);
    DisplayInternalError();
    Close(fh);
    NiKPermit();
    return 1;
  }
  Close(fh);
  NiKPermit();
  return 0;
}

int writeuser(int userId,struct User *user) {
  BPTR fh;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  NiKForbid();
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    DisplayInternalError();
    NiKPermit();
    return 1;
  }
  if(Write(fh, (void *)user, sizeof(struct User)) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't write %s", filename);
    DisplayInternalError();
    Close(fh);
    NiKPermit();
    return 1;
  }
  Close(fh);
  NiKPermit();
  return 0;
}

void rensatexter(void) {
  int noOfTexts = 0;
  if(!(noOfTexts = atoi(argument))) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_PURGE_TEXTS_SYNTAX));
    return;
  }
  if(noOfTexts % 512) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_PURGE_TEXTS_512));
    return;
  }
  sendservermess(RADERATEXTER, noOfTexts);
}

void gamlatexter(void) { }

void gamlabrev(void) { }

void getconfig(void) {
  long success;
  success = sendservermess(READCFG, 0);
  SendString("\r\n\n%s\r\n",
             success ? CATSTR(MSG_READCFG_SUCCESS) : CATSTR(MSG_READCFG_FAIL));
}

void writeinfo(void) {
	sendservermess(WRITEINFO,0);
}

void displaysay(void) {
  struct SayString *sayStr, *prevSayStr;
  int textlen;
  char tmpchar, *fromStr;

  sayStr = Servermem->say[nodnr];
  Servermem->say[nodnr] = NULL;
  while(sayStr) {
    if(sayStr->fromuser == -1) {
      fromStr = CATSTR(MSG_SAY_DISPLAY_SYSTEM);
    } else {
      fromStr = getusername(sayStr->fromuser);
    }
    SendString("\a\r\n\n%s", fromStr);
    textlen = strlen(sayStr->text);
    if((strlen(fromStr) + textlen) < 75) {
      SendString(": %s\r\n", sayStr->text);
    } else {
      SendString(":\n\r");
      if(textlen <= MAXSAYTKN - 1) {
        SendString(sayStr->text);
      } else {
        tmpchar = sayStr->text[MAXSAYTKN - 1];
        sayStr->text[MAXSAYTKN - 1] = '\0';
        SendString(sayStr->text);
        sayStr->text[MAXSAYTKN-1] = tmpchar;
        SendString(&sayStr->text[MAXSAYTKN - 1]);
      }
    }
    prevSayStr = sayStr;
    sayStr = prevSayStr->NextSay;
    FreeMem(prevSayStr, sizeof(struct SayString));
  }
  SendString("\n");
}

int sag(void) {
  int userId, i;
  char *quick;
  struct SayString *sayStr, *prevSayStr = NULL, *newSayStr;

  quick = strchr(argument,',');
  if(quick) {
    *quick++ = 0;
  }
  if((userId = parsenamn(argument)) == -3) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_SAY_SYNTAX));
    return 0;
  }
  if(userId == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
    return 0;
  }
  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->inloggad[i]==userId) { break; }
  }
  if(i == MAXNOD) {
    SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_SAY_NOT_LOGGED_IN), getusername(userId));
    return 0;
  }
  if(!quick) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SAY_WHAT));
    if(getstring(EKO, MAXSAYTKN - 1, NULL)) {
      return 1;
    }
    if(!inmat[0]) {
      return 0;
    }
    if(Servermem->inloggad[i] == -1) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_SAY_USER_LOGGED_OUT));
      return 0;
    }
  }
  sayStr = Servermem->say[i];
  if(sayStr) {
    SendStringCat("\r\n%s\r\n", CATSTR(MSG_SAY_USER_HAS_UNREAD), getusername(userId));
  } else {
    SendStringCat("\r\n%s\r\n", CATSTR(MSG_SAY_SENT), getusername(userId));
  }
  Forbid();
  while(sayStr) {
    prevSayStr = sayStr;
    sayStr = prevSayStr->NextSay;
  }
  if(!(newSayStr = (struct SayString *)AllocMem(sizeof(struct SayString), MEMF_PUBLIC | MEMF_CLEAR))) {
    Permit();
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't allocate %d bytes.", sizeof(struct SayString));
    DisplayInternalError();
    return 0;
  }
  newSayStr->fromuser = inloggad;
  if(quick) {
    strcpy(newSayStr->text, quick);
  } else {
    strcpy(newSayStr->text, inmat);
  }
  if(Servermem->say[i]) {
    prevSayStr->NextSay = newSayStr;
  } else {
    Servermem->say[i] = newSayStr;
  }
  Permit();
  return 0;
}

void writesenaste(void) {
  BPTR fh;
  long now;

  time(&now);
  Statstr.tid_inloggad = (now - logintime) / 60;
  Statstr.utloggtid = now;
  NiKForbid();
  memmove(&Servermem->senaste[1], &Servermem->senaste, sizeof(struct Inloggning) * (MAXSENASTE - 1));
  memcpy(Servermem->senaste, &Statstr, sizeof(struct Inloggning));
  if(!(fh = Open("NiKom:DatoCfg/Senaste.dat", MODE_NEWFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't open Senaste.dat.");
    NiKPermit();
    return;
  }
  if(Write(fh, (void *)Servermem->senaste, sizeof(struct Inloggning) * MAXSENASTE) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't write Senaste.dat.");
  }
  Close(fh);
  NiKPermit();
}

void listasenaste(void) {
  int i, userId, allUsers = TRUE, cnt = 0, antal = MAXSENASTE;
  struct tm *ts;
  char durationBuf[10];

  if(argument[0] == '-') {
    antal = atoi(&argument[1]);
    argument = hittaefter(argument);
  }
  if((userId = parsenamn(argument)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
    return;
  } else if(userId == -3) {
    allUsers = TRUE;
  } else {
    allUsers = FALSE;
  }
  SendString("\r\n\n%s\r\n\n", CATSTR(MSG_LIST_LOGINS_HEADER));
  for(i = 0; i < MAXSENASTE; i++) {
    if(!Servermem->senaste[i].utloggtid || (!allUsers && userId != Servermem->senaste[i].anv)) {
      continue;
    }
    if(!userexists(Servermem->senaste[i].anv)) {
      continue;
    }
    cnt++;
    ts = localtime(&Servermem->senaste[i].utloggtid);
    if(SendString("%-35s %02d/%02d %02d:%02d %7s  %4d %3d  %2d  %2d\r\n",
                  getusername(Servermem->senaste[i].anv), ts->tm_mday, ts->tm_mon + 1, ts->tm_hour, ts->tm_min,
                  FormatDuration(Servermem->senaste[i].tid_inloggad * 60, durationBuf),
                  Servermem->senaste[i].read, Servermem->senaste[i].write,
                  Servermem->senaste[i].ul, Servermem->senaste[i].dl)) {
      return;
    }
    if(cnt >= antal) {
      return;
    }
  }
}

int dumpatext(void) {
  int tnr, i;
  struct Header dumphead;
  FILE *fpr,*fpd;
  char *dumpfil,foostr[82],filename[40];

  if(!IzDigit(argument[0])) {
    SendString("\r\n\n%s\r\n\n");
    return 0;
  }
  tnr = atoi(argument);
  if(tnr < Servermem->info.lowtext || tnr > Servermem->info.hightext) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return 0;
  }
  if(!MayReadConf(GetConferenceForText(tnr), inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_DUMP_TEXT_NOPERM));
    return 0;
  }
  dumpfil = hittaefter(argument);
  if(!dumpfil[0]) {
    SendString("\r\n\n%s : ", CATSTR(MSG_DUMP_TEXT_FILE));
    if(getstring(EKO, 50, NULL)) {
      return 1;
    }
    if(!inmat[0]) {
      return 0;
    }
    dumpfil = inmat;
  }
  if(readtexthead(tnr, &dumphead)) {
    return 0;
  }
  sprintf(filename, "NiKom:Moten/Text%ld.dat", dumphead.nummer / 512);
  NiKForbid();
  if(!(fpr = fopen(filename, "r"))) {
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't open %s.", filename);
    DisplayInternalError();
    NiKPermit();
    return 0;
  }
  if(fseek(fpr, dumphead.textoffset, 0)) {
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't seek to position %d in %s.", dumphead.textoffset, filename);
    DisplayInternalError();
    fclose(fpr);
    NiKPermit();
    return 0;
  }
  if(!(fpd = fopen(dumpfil, "w"))) {
    SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_DUMP_TEXT_ERROR_OPEN), dumpfil);
    fclose(fpr);
    NiKPermit();
    return 0;
  }
  for(i = 0; i < dumphead.rader; i++) {
    if(!(fgets(foostr, 81, fpr))) {
      LogEvent(SYSTEM_LOG, ERROR, "Error reading %s.", filename);
      DisplayInternalError();
      break;
    }
    if(fputs(foostr, fpd)) {
      LogEvent(SYSTEM_LOG, ERROR, "Error writing %s.", dumpfil);
      DisplayInternalError();
      break;
    }
  }
  fclose(fpr);
  fclose(fpd);
  NiKPermit();
  return 0;
}

void listaarende(void) {
  char *nextarg = argument, namn[50], kom[10];
  int forward = FALSE, textNumber = -1, onlyRoots = FALSE;
  struct Mote *conf;
  struct Header lahead;
  struct tm *ts;

  if(mote2 == -1) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_LIST_TEXTS_MAIL));
    return;
  }
  while(nextarg[0]) {
    if(nextarg[0] == '-') {
      if(nextarg[1] == 't' || nextarg[1] == 'T') {
        onlyRoots=TRUE;
      } else if(nextarg[1]=='f' || nextarg[1]=='F') {
        forward = TRUE;
      }
    } else if(IzDigit(nextarg[0])) {
      textNumber = atoi(nextarg);
    }
    nextarg = hittaefter(nextarg);
  }
  conf = getmotpek(mote2);
  if(conf->type == MOTE_FIDO) {
    fidolistaarende(conf, forward ? 1 : -1);
    return;
  }
  SendString("\r\n\n%s", CATSTR(MSG_LIST_TEXTS_HEADER));
  SendString("\r\n-------------------------------------------------------------------------\r\n");

  if(textNumber == -1) {
    textNumber = (forward ? -1 : INT_MAX);
  } else {
    textNumber += (forward ? -1 : 1);
  }
  while((textNumber = (forward
                       ? FindNextTextInConference(textNumber + 1, mote2)
                       : FindPrevTextInConference(textNumber - 1, mote2))) != -1) {
    if(readtexthead(textNumber, &lahead)) {
      return;
    }
    if(lahead.kom_till_nr != -1 && onlyRoots) {
      continue;
    }
    strcpy(namn, getusername(lahead.person));
    lahead.arende[22] = 0;
    ts=localtime(&lahead.tid);
    if(lahead.kom_till_nr == -1) {
      strcpy(kom,"   -");
    } else {
      sprintf(kom,"%ld",lahead.kom_till_nr);
    }
    if(SendString("%-34s%6ld %6s %02d%02d%02d %s\r\n", namn,
                  lahead.nummer, kom, ts->tm_year % 100, ts->tm_mon + 1,
                  ts->tm_mday, lahead.arende)) {
      return;
    }
  }
}

void tellallnodes(char *str) {
  int i;
  struct SayString *sayStr, *prevSayStr=NULL, *newSayStr;

  for(i = 0; i < MAXNOD; i++) {
    if((!Servermem->nodtyp[i]) || Servermem->inloggad[i] < 0 || i == nodnr) {
      continue;
    }
    if(Servermem->inne[i].flaggor & NOLOGNOTIFY) {
      continue;
    }
    sayStr = Servermem->say[i];
    Forbid();
    while(sayStr) {
      prevSayStr = sayStr;
      sayStr = prevSayStr->NextSay;
    }
    if(!(newSayStr = (struct SayString *)AllocMem(sizeof(struct SayString), MEMF_PUBLIC | MEMF_CLEAR))) {
      Permit();
      LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.", sizeof(struct SayString));
      DisplayInternalError();
      return;
    }
    newSayStr->fromuser = inloggad;
    strcpy(newSayStr->text, str);
    if(Servermem->say[i]) {
      prevSayStr->NextSay = newSayStr;
    } else {
      Servermem->say[i] = newSayStr;
    }
    Permit();
  }
}

int skrivlapp(void) {
  int editret;
  struct EditLine *el;
  FILE *fp;
  char filename[100];

  sprintf(filename, "NiKom:Users/%d/%d/Lapp", inloggad / 100, inloggad);
  SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_ENTERING_EDITOR));
  if((editret = edittext(filename)) == 1) {
    return 1;
  } else if(editret == 2) {
    return 0;
  }
  if(!(fp = fopen(filename, "w"))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open file %s", filename);
    DisplayInternalError();
    return 0;
  }
  ITER_EL(el, edit_list, line_node, struct EditLine *) {
    if(fputs(el->text, fp)) {
      LogEvent(SYSTEM_LOG, ERROR, "Error writing file %s", filename);
      DisplayInternalError();
      fclose(fp);
      return 0;
    }
    fputc('\n', fp);
  }
  fclose(fp);
  freeeditlist();
  return 1;
}

void radlapp(void) {
  char filename[100];
  sprintf(filename, "NiKom:Users/%d/%d/Lapp", inloggad / 100, inloggad);
  if(remove(filename)) {
    LogEvent(SYSTEM_LOG, ERROR, "Error deleting file %s", filename);
    DisplayInternalError();
  } else {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_DELETE_NOTE_DONE));
  }
}

int userexists(int userId) {
  struct ShortUser *user;
  int found=FALSE;

  ITER_EL(user, Servermem->user_list, user_node, struct ShortUser *) {
    if(user->nummer == userId) {
      found = TRUE;
      break;
    }
  }
  return found;
}
