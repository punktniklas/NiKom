#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#ifdef __GNUC__
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __GNUC__
/* In gcc access() is defined in unistd.h, while SAS/C has the
   prototype in stdio.h */
# include <unistd.h>
#endif
#include <time.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Terminal.h"
#include "Logging.h"
#include "ServerMemUtils.h"
#include "StringUtils.h"
#include "BasicIO.h"
#include "Languages.h"

#define EKO		1

extern struct System *Servermem;
extern int nodnr,inloggad,mote2,senast_text_typ,radcnt;
extern char inmat[],*argument,usernamebuf[];
extern struct Inloggning Statstr;
extern struct Header readhead;
extern struct MinList aliaslist, edit_list;

char gruppnamebuf[41];

int movetext(void) {
  int textId, newConfId, oldConfId;
  struct Mote *conf;
  struct Header movehead;

  if(!IzDigit(argument[0])) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_MOVE_TEXT_SYNTAX));
    return 0;
  }
  textId = atoi(argument);
  if(textId < Servermem->info.lowtext || textId > Servermem->info.hightext) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return 0;
  }
  argument = hittaefter(argument);
  if(!argument[0]) {
    SendString("\r\n\n%s: ", CATSTR(MSG_MOVE_TEXT_WHICH_FORUM));
    if(getstring(EKO,45,NULL)) {
      return 1;
    }
    if(!inmat[0]) {
      return 0;
    }
    argument = inmat;
  }
  if((newConfId = parsemot(argument)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
    return 0;
  }
  oldConfId = GetConferenceForText(textId);
  if(newConfId == oldConfId) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_MOVE_TEXT_ALREADY_THERE));
    return 0;
  }
  if(readtexthead(textId, &movehead)) {
    return 0;
  }
  conf = getmotpek(oldConfId);
  if(!conf) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not find confId %d", oldConfId);
    DisplayInternalError();
    return 0;
  }
  if(movehead.person != inloggad
     && !MayAdminConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_MOVE_TEXT_NO_PERM_TEXT));
    return 0;
  }
  if(!MayWriteConf(newConfId, inloggad, &Servermem->inne[nodnr])
     || !MayReplyConf(newConfId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_MOVE_TEXT_NO_PERM_FORUM));
    return 0;
  }
  conf = getmotpek(newConfId);
  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_MOVE_TEXT_MOVED), conf->namn);
  movehead.mote = newConfId;
  if(writetexthead(textId,&movehead)) {
    return 0;
  }
  SetConferenceForText(textId, newConfId, TRUE);
  return 0;
}

char *getusername(int userId) {
  struct ShortUser *user;
  int found = FALSE;

  ITER_EL(user, Servermem->user_list, user_node, struct ShortUser *) {
    if(user->nummer == userId) {
      if(user->namn[0]) {
        sprintf(usernamebuf, "%s #%d", user->namn, userId);
      } else {
        sprintf(usernamebuf, "<%s>", CATSTR(MSG_USER_GET_DELETED));
      }
      found = TRUE;
      break;
    }
  }
  if(!found) {
    sprintf(usernamebuf, "<%s>", CATSTR(MSG_USER_GET_INVALID));
  }
  return usernamebuf;
}

int namematch(char *pattern, char *str) {
  char *pattern2, *str2;
  if(!matchar(pattern, str)) {
    return FALSE;
  }
  pattern2 = hittaefter(pattern);
  str2 = hittaefter(str);
  if(!pattern2[0]) {
    return TRUE;
  }
  if(!matchar(pattern2, str2)) {
    str2 = hittaefter(str2);
    if(!matchar(pattern2,str2)) {
      return FALSE;
    }
  }
  return TRUE;
}

int skapagrupp(void) {
  struct UserGroup *newUserGroup, *userGroup;
  int groupadmin, groupId;
  char buff[9];
  
  if(Servermem->inne[nodnr].status < Servermem->cfg.st.grupper) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_CREATE_GROUP_NO_PERM));
    return 0;
  }

  groupId = 0;
  ITER_EL(userGroup, Servermem->grupp_list, grupp_node, struct UserGroup *) {
    if(userGroup->nummer > groupId) {
      break;
    }
    groupId++;
  }
  if(groupId >= 32) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_CREATE_GROUP_MAX));
    return 0;
  }
  if(!(newUserGroup = AllocMem(sizeof(struct UserGroup),
                               MEMF_CLEAR | MEMF_PUBLIC))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.",
             sizeof(struct UserGroup));
    DisplayInternalError();
    return 0;
  }
  newUserGroup->nummer = groupId;

  if(!argument[0]) {
    SendString("\r\n\n%s: ", CATSTR(MSG_CREATE_GROUP_NAME));
    if(GetString(40, NULL)) {
      FreeMem(newUserGroup, sizeof(struct UserGroup));
      return 1;
    }
    if(!inmat[0]) {
      FreeMem(newUserGroup, sizeof(struct UserGroup));
      return 0;
    }
    strcpy(newUserGroup->namn, inmat);
  } else {
    strcpy(newUserGroup->namn, argument);
  }
  if(parsegrupp(newUserGroup->namn) != -1) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_CREATE_GROUP_ALREADY));
    FreeMem(newUserGroup, sizeof(struct UserGroup));
    return 0;
  }
  SendString("\r\n\n%s\r\n", CATSTR(MSG_CREATE_GROUP_STATUS_1));
  SendString("0  = %s\r\n", CATSTR(MSG_CREATE_GROUP_STATUS_2));
  SendString("-1 = %s\r\n", CATSTR(MSG_CREATE_GROUP_STATUS_3));
  newUserGroup->autostatus = GetNumber(-1, 100, NULL);

  if(EditBitFlagChar("\r\n", CATSTR(MSG_CREATE_GROUP_SECRET), NULL, NULL,
                     CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                     &newUserGroup->flaggor, HEMLIGT)) {
    return 1;
  }

  for(;;) {
    SendString("\r\n%s: ", CATSTR(MSG_CREATE_GROUP_ADMIN));
    sprintf(buff,"%d",inloggad);
    if(GetString(40 ,buff)) {
      return 1;
    }
    if(inmat[0]) {
      if((groupadmin = parsenamn(inmat)) == -1) {
        SendString("\r\n%s", CATSTR(MSG_COMMON_NO_SUCH_USER));
      } else {
        newUserGroup->groupadmin = groupadmin;
        break;
      }
    }
  }

  if(writegrupp(groupId, newUserGroup)) {
    FreeMem(newUserGroup, sizeof(struct UserGroup));
    return 0;
  }
  
  Insert((struct List *)&Servermem->grupp_list, (struct Node *)newUserGroup,
         (struct Node *)userGroup->grupp_node.mln_Pred);
  BAMSET((char *)&Servermem->inne[nodnr].grupper, groupId);
  return 0;
}

int writegrupp(int groupId, struct UserGroup *group) {
  BPTR fh;
  if(!(fh = Open("NiKom:DatoCfg/Grupper.dat", MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open Grupper.dat");
    DisplayInternalError();
    return 1;
  }
  if(Seek(fh, groupId * sizeof(struct UserGroup), OFFSET_BEGINNING) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not seek to position %d in Grupper.dat",
             groupId * sizeof(struct UserGroup));
    DisplayInternalError();
    Close(fh);
    return 1;
  }
  if(Write(fh, (void *)group, sizeof(struct UserGroup)) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not write to Grupper.dat");
    DisplayInternalError();
    Close(fh);
    return 1;
  }
  Close(fh);
  return 0;
}

void listagrupper(void) {
  struct UserGroup *group;
  int isMember;

  SendString("\r\n\n");
  ITER_EL(group, Servermem->grupp_list, grupp_node, struct UserGroup *) {
    isMember = gruppmed(group, Servermem->inne[nodnr].status, Servermem->inne[nodnr].grupper);
    if((group->flaggor & HEMLIGT)
       && !isMember
       && Servermem->inloggad[nodnr] != group->groupadmin) {
      continue;
    }
    if(SendString("%c %s\r\n", isMember ? '*' : ' ', group->namn)) {
      break;
    }
  }
}

static void changeGroupMembership(int isAdd) {
  int i, userId, groupId;
  struct User anv;
  struct ShortUser *user;
  struct UserGroup *group;
  
  if(!argument[0]) {
    SendString("\r\n\n%s ", isAdd ? CATSTR(MSG_CHANGE_GROUPMEM_WHOADD) : CATSTR(MSG_CHANGE_GROUPMEM_WHOREM));
    if(getstring(EKO, 40, NULL)) {
      return;
    }
    if(!inmat[0]) {
      return;
    }
    argument = inmat;
  }
  if((userId = parsenamn(argument)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
    return;
  }
  SendString("\r\n\n%s ", CATSTR(MSG_CHANGE_GROUPMEM_WHICH));
  if(getstring(EKO, 40, NULL)) {
    return;
  }
  if(!inmat[0]) {
    return;
  }
  if((groupId = parsegrupp(inmat)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_GROUP));
    return;
  }
  group = FindUserGroup(groupId);
  if(inloggad != group->groupadmin && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_CHANGE_GROUPMEM_NOPERM));
    return;
  }
  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->inloggad[i] == userId) { break; }
  }
  if(i == MAXNOD) {
    if(readuser(userId, &anv)) {
      return;
    }
    if(isAdd) {
      BAMSET((char *)&anv.grupper, groupId);
    } else {
      BAMCLEAR((char *)&anv.grupper, groupId);
    }
    if(writeuser(userId,&anv)) {
      return;
    }
  } else {
    if(isAdd) {
      BAMSET((char *)&Servermem->inne[i].grupper, groupId);
    } else {
      BAMCLEAR((char *)&Servermem->inne[i].grupper, groupId);
    }
  }

  SendStringCat("\r\n\n%s\r\n",
                isAdd ? CATSTR(MSG_CHANGE_GROUPMEM_ADDED) : CATSTR(MSG_CHANGE_GROUPMEM_REMOVED),
                getusername(userId), getgruppname(groupId));
  ITER_EL(user, Servermem->user_list, user_node, struct ShortUser *) {
    if(user->nummer == userId) { break; }
  }
  if(user->user_node.mln_Succ) {
    if(isAdd) {
      BAMSET((char *)&user->grupper, groupId);
    } else {
      BAMCLEAR((char *)&user->grupper, groupId);
    }
  } else {
    LogEvent(SYSTEM_LOG, ERROR, "Could not find ShortUser entry for user %d", userId);
    DisplayInternalError();
  }
}

void adderagruppmedlem(void) {
  changeGroupMembership(TRUE);
}

void subtraheragruppmedlem(void) {
  changeGroupMembership(FALSE);
}

int parsegrupp(char *pattern) {
  char *str, *pattern2;
  struct UserGroup *group;

  if(pattern[0] == '\0' || pattern[0] == ' ') {
    return -3;
  }
  ITER_EL(group, Servermem->grupp_list, grupp_node, struct UserGroup *) {
    str = group->namn;
    pattern2 = pattern;
    if(matchar(pattern2, str)) {
      for(;;) {
        pattern2 = hittaefter(pattern2);
        str = hittaefter(str);
        if(pattern2[0] == '\0') {
          return group->nummer;
        }
        else if(str[0] == '\0') {
          break;
        } else if(!matchar(pattern2, str)) {
          str = hittaefter(str);
          if(str[0] == '\0' || !matchar(pattern2, str)) {
            break;
          }
        }
      }
    }
  }
  return -1;
}

int andragrupp(void) {
  struct UserGroup *userGroup, tmpUserGroup;
  int groupId, groupadmin, isCorrect;
  char buff[9];
  
  if(!argument[0]) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_CHANGE_GROUP_SYNTAX));
    return 0;
  }
  if((groupId = parsegrupp(argument)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_GROUP));
    return 0;
  }

  if((userGroup = FindUserGroup(groupId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't find group %d in memory.", groupId);
    DisplayInternalError();
    return 0;
  }
  if(inloggad != userGroup->groupadmin
     && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_CHANGE_GROUP_NOPERM));
    return 0;
  }
  memcpy(&tmpUserGroup, userGroup, sizeof(struct UserGroup));

  MaybeEditString(CATSTR(MSG_CREATE_GROUP_NAME), tmpUserGroup.namn, 40);

  SendString("\r\n%s\r\n", CATSTR(MSG_CREATE_GROUP_STATUS_1));
  SendString("0  = %s\r\n", CATSTR(MSG_CREATE_GROUP_STATUS_2));
  SendString("-1 = %s\r\n", CATSTR(MSG_CREATE_GROUP_STATUS_3));
  MaybeEditNumberChar("Status", &tmpUserGroup.autostatus, 3, -1, 100);

  if(EditBitFlagChar("\r\n", CATSTR(MSG_CREATE_GROUP_SECRET), NULL, NULL,
                     CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                     &tmpUserGroup.flaggor, HEMLIGT)) {
    return 1;
  }

  for(;;) {
    SendString("\r\n%s: ", CATSTR(MSG_CREATE_GROUP_ADMIN));
    sprintf(buff, "%d", userGroup->groupadmin);
    if(GetString(40, buff)) {
      return 1;
    }
    if(inmat[0]) {
      if((groupadmin = parsenamn(inmat)) == -1) {
        SendString("\r\n%s", CATSTR(MSG_COMMON_NO_SUCH_USER));
      } else {
        tmpUserGroup.groupadmin = groupadmin;
        break;
      }
    }
  }

  if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMON_IS_CORRECT), NULL, NULL,
                CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n",
                TRUE, &isCorrect)) {
    return 1;
  }
  if(!isCorrect) {
    return 0;
  }

  memcpy(userGroup, &tmpUserGroup, sizeof(struct UserGroup));
  writegrupp(groupId, userGroup);
  return 0;
}

void raderagrupp(void) {
  struct UserGroup *userGroup;
  int groupId, isCorrect;

  if(!argument[0]) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_DELETE_GROUP_SYNTAX));
    return;
  }
  if((groupId = parsegrupp(argument)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_GROUP));
    return;
  }
  if((userGroup = FindUserGroup(groupId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't find group %d in memory.", groupId);
    DisplayInternalError();
    return;
  }
  if(inloggad != userGroup->groupadmin
     && Servermem->inne[nodnr].status < Servermem->cfg.st.grupper) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_DELETE_GROUP_NOPERM));
    return;
  }
  SendStringCat("\r\n\n%s", CATSTR(MSG_DELETE_GROUP_CONFIRM), userGroup->namn);
  if(GetYesOrNo(NULL, NULL, NULL, NULL,
                CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                "\r\n", FALSE, &isCorrect)) {
    return;
  }
  if(!isCorrect) {
    return;
  }
  Remove((struct Node *)userGroup);
  userGroup->namn[0] = 0;
  writegrupp(userGroup->nummer, userGroup);
  FreeMem(userGroup, sizeof(struct UserGroup));
}

void initgrupp(void) {
  struct UserGroup *group;
  ITER_EL(group, Servermem->grupp_list, grupp_node, struct UserGroup *) {
    if(group->autostatus != -1 && Servermem->inne[nodnr].status >= group->autostatus) {
      BAMSET((char *)&Servermem->inne[nodnr].grupper, group->nummer);
    }
  }
}

char *getgruppname(int groupId) {
  struct UserGroup *group;
  int found=FALSE;

  ITER_EL(group, Servermem->grupp_list, grupp_node, struct UserGroup *) {
    if(group->nummer == groupId) {
      if(group->namn[0]) {
        strcpy(gruppnamebuf, group->namn);
      } else {
        sprintf(gruppnamebuf, "<%s>", CATSTR(MSG_GET_GROUP_DELETED));
      }
      found = TRUE;
      break;
    }
  }
  if(!found) {
    sprintf(gruppnamebuf,"<%s>", CATSTR(MSG_GET_GROUP_INVALID));
  }
  return gruppnamebuf;
}

int editgrupp(char *bitmap) {
  int groupId;
  struct UserGroup *group;

  do {
    if(getstring(EKO, 40, NULL)) {
      return 1;
    }
    if(inmat[0] == '?') {
      listagrupper();
      SendString("%s\r\n", CATSTR(MSG_EDIT_GROUPS_AVAILABLE));
    } else if(inmat[0] == '!') {
      SendString("\r\n%s:\r\n", CATSTR(MSG_EDIT_GROUPS_CHOSEN));
      ITER_EL(group, Servermem->grupp_list, grupp_node, struct UserGroup *) {
        if(BAMTEST(bitmap, group->nummer)) {
          SendString("%s\r\n", group->namn);
        }
      }
    } else if(inmat[0] != '\0') {
      if((groupId = parsegrupp(inmat)) == -1) {
        SendString("\r\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_GROUP));
      } else if(groupId >= 0) {
        SendString("\r%s\r\n", getgruppname(groupId));
        if(BAMTEST(bitmap, groupId)) {
          BAMCLEAR(bitmap, groupId);
        } else {
          BAMSET(bitmap, groupId);
        }
      }
    }
  } while(inmat[0] != '\0');
  return 0;
}

void motesstatus(void) {
  int confId;
  struct Mote *conf;
  struct tm *ts;
  struct UserGroup *group;
  char filename[50];

  if(!argument[0]) {
    if(mote2 != -1) {
      confId = mote2;
    } else {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_FORUM_STATUS_SYNTAX));
      return;
    }
  } else {
    confId = parsemot(argument);
    if(confId == -1) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_GO_NO_SUCH_FORUM));
      return;
    }
  }
  conf = getmotpek(confId);
  SendString("\r\n\n%-18s: %s",CATSTR(MSG_FORUM_CREATE_NAME), conf->namn);
  SendString("\r\n%-18s: %s",CATSTR(MSG_FORUM_STATUS_CREATED_BY), getusername(conf->skapat_av));
  ts = localtime(&conf->skapat_tid);
  SendString("\r\n%-18s: %4d%02d%02d  %02d:%02d", CATSTR(MSG_FORUM_STATUS_CREATED_TIME),
             ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
             ts->tm_min);
  SendString("\r\n%-18s: %d", CATSTR(MSG_FORUM_STATUS_ID), conf->nummer);
  SendString("\r\n%-18s: %ld", CATSTR(MSG_FORUM_CREATE_SORT), conf->sortpri);
  SendString("\r\n%-18s: %s", CATSTR(MSG_FORUM_STATUS_MAD), getusername(conf->mad));
  SendString("\r\n%-18s: ", CATSTR(MSG_FORUM_STATUS_TYPE));
  if(conf->type == MOTE_ORGINAL) {
    SendString("%s\r\n", CATSTR(MSG_FORUM_CREATE_TYPE_LOCAL));
  } else if(conf->type == MOTE_FIDO) {
    SendString(CATSTR(MSG_FORUM_CREATE_TYPE_FIDO));
    SendString("\r\n%-18s: %s", CATSTR(MSG_FORUM_STATUS_TAG), conf->tagnamn);
    SendString("\r\n%-18s: %s", CATSTR(MSG_FORUM_STATUS_DIR), conf->dir);
    SendString("\r\n%-18s: %s\r\n", CATSTR(MSG_FORUM_CREATE_FIDO_ORIGIN), conf->origin);
  } else {
    SendString("\r\n");
  }

  if(conf->status & KOMSKYDD) SendString("%s  ", CATSTR(MSG_FORUM_LIST_COMMPROT));
  if(conf->status & SKRIVSKYDD) SendString("%s  ", CATSTR(MSG_FORUM_LIST_WRITEPROT));
  if(conf->status & SLUTET) SendString("%s  ", CATSTR(MSG_FORUM_LIST_CLOSED));
  if(conf->status & HEMLIGT) SendString("%s  ", CATSTR(MSG_FORUM_LIST_SECRET));
  if(conf->status & AUTOMEDLEM) SendString("%s  ", CATSTR(MSG_FORUM_LIST_AUTO));
  if(conf->status & SKRIVSTYRT) SendString("%s  ", CATSTR(MSG_FORUM_LIST_WRITECTRL));
  if(conf->status & SUPERHEMLIGT) SendString("%s  ", CATSTR(MSG_FORUM_LIST_AREXX));
  SendString("\r\n\n");
  if((conf->status & (SLUTET | SKRIVSTYRT)) && conf->grupper) {
    SendString("%s:\r\n", CATSTR(MSG_FORUM_STATUS_GROUPS));
    ITER_EL(group, Servermem->grupp_list, grupp_node, struct UserGroup *) {
      if(!BAMTEST((char *)&conf->grupper, group->nummer)) {
        continue;
      }
      if((group->flaggor & HEMLIGT)
         && !gruppmed(group, Servermem->inne[nodnr].status, Servermem->inne[nodnr].grupper)) {
        continue;
      }
      SendString("%s\r\n", group->namn);
    }
  }
  sprintf(filename, "NiKom:Lappar/%d.motlapp", confId);
  if(access(filename,0) == 0) {
    sendfile(filename);
  }
}

void hoppaarende(void) {
  struct Header header;
  int skipped = 0, nextUnread;

  if(!argument[0]) {
    if(senast_text_typ != TEXT) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_SKIP_SUBJECT_SYNTAX));
      return;
    } else {
      argument = readhead.arende;
    }
  }
  if(mote2 == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SKIP_SUBJECT_MAIL));
    return;
  }
  if(strlen(argument) > 40) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SKIP_SUBJECT_TOO_LONG));
    return;
  }
  nextUnread = -1;
  while((nextUnread = FindNextUnreadText(nextUnread + 1, mote2,
                                         &Servermem->unreadTexts[nodnr])) != -1) {
    if(readtexthead(nextUnread, &header)) {
      return;
    }
    if(!strncmp(header.arende, argument, strlen(argument))) {
      ChangeUnreadTextStatus(nextUnread, 0, &Servermem->unreadTexts[nodnr]);
      skipped++;
    }
  }
  if(!skipped) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SKIP_SUBJECT_NONE));
  } else if(skipped==1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SKIP_SUBJECT_ONE));
  } else {
    SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_SKIP_SUBJECT_MANY), skipped);
  }
}

int arearatt(int area, int usrnr, struct User *usr) {
  if(usr->status >= Servermem->cfg.st.bytarea) {
    return 1;
  }
  return (Servermem->areor[area].mote==-1 || MayBeMemberConf(Servermem->areor[area].mote, usrnr, usr))
    && (!Servermem->areor[area].grupper || (Servermem->areor[area].grupper & usr->grupper)); 
}

int gruppmed(struct UserGroup *grupp, char status, long grupper) {
  if(status >= Servermem->cfg.st.medmoten) {
    return 1;
  }
  if(grupp->autostatus != -1 && status >= grupp->autostatus) {
    return 1;
  }
  if(BAMTEST((char *)&grupper,grupp->nummer)) {
    return 1;
  }
  return 0;
}

void listgruppmed(void) {
  struct ShortUser *user;
  int groupId;

  if(!argument[0]) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_LIST_GROUPMEM_SYNTAX));
    return;
  }
  if((groupId = parsegrupp(argument)) == -1) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_COMMON_NO_SUCH_GROUP));
    return;
  }
  SendString("\r\n\n");
  ITER_EL(user, Servermem->user_list, user_node, struct ShortUser *) {
    if(BAMTEST((char *)&user->grupper, groupId)) {
      if(SendString("%s #%ld\r\n", user->namn, user->nummer)) {
        return;
      }
    }
  }
}

struct Alias *parsealias(char *skri) {
  struct Alias *alias;
  int i;

  ITER_EL(alias, aliaslist, alias_node, struct Alias *) {
    for(i = 0; alias->namn[i] != ' ' && alias->namn[i] != 0; i++);
    if(!strnicmp(skri, alias->namn, i) && (skri[i] == ' ' || skri[i] == 0)) {
      return alias;
    }
  }
  return NULL;
}

void listalias(void) {
  struct Alias *alias;
  SendString("\r\n\n");
  ITER_EL(alias, aliaslist, alias_node, struct Alias *) {
    SendString("%-21s = %s\r\n", alias->namn, alias->blirtill);
  }
}

void remalias(void) {
  struct Alias *alias;
  if(!(alias = parsealias(argument))) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_ALIAS_NOSUCH));
    return;
  }
  Remove((struct Node *)alias);
  FreeMem(alias, sizeof(struct Alias));
}

void defalias(void) {
  struct Alias *alias;
  char *secondArg, *str;

  secondArg = hittaefter(argument);
  str = argument;
  while(str[0] != ' ' && str[0]) {
    str++;
  }
  str[0] = '\0';
  if((alias = parsealias(argument))) {
    strncpy(alias->blirtill, secondArg, 40);
  } else {
    if(!(alias = AllocMem(sizeof(struct Alias), MEMF_CLEAR))) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes", sizeof(struct Alias));
      DisplayInternalError();
      return;
    }
    strncpy(alias->namn, argument, 20);
    strncpy(alias->blirtill, secondArg, 40);
    AddTail((struct List *)&aliaslist, (struct Node *)alias);
  }
}

void alias(void) {
  char *secondArg = hittaefter(argument);
  if(!argument[0]) {
    listalias();
  } else if(!secondArg[0]) {
    remalias();
  } else {
    defalias();
  }
}

int readtextlines(long pos, int lines, int textId) {
  FILE *fp;
  struct EditLine *el;
  int i, fileNumber;
  char rlbuf[100], filename[40];

  fileNumber = textId / 512;
  NewList((struct List *)&edit_list);
  NiKForbid();
  sprintf(filename, "NiKom:Moten/Text%d.dat", fileNumber);
  if(!(fp = fopen(filename, "r"))) {
    NiKPermit();
    LogEvent(SYSTEM_LOG, ERROR, "Could not open %s for reading.", filename);
    DisplayInternalError();
    return 1;
  }
  if(fseek(fp,pos,0)) {
    fclose(fp);
    NiKPermit();
    LogEvent(SYSTEM_LOG, ERROR, "Could not find position %d in %s.", pos, filename);
    DisplayInternalError();
    return 1;
  }
  for(i = 0; i < lines; i++) {
    if(!fgets(rlbuf, MAXFULLEDITTKN+1, fp)) {
      fclose(fp);
      NiKPermit();
      LogEvent(SYSTEM_LOG, ERROR, "Could not read line from %s.", filename);
      DisplayInternalError();
      return 1;
    }
    if(!(el = (struct EditLine *)AllocMem(sizeof(struct EditLine),
                                          MEMF_CLEAR | MEMF_PUBLIC))) {
      fclose(fp);
      NiKPermit();
      LogEvent(SYSTEM_LOG, ERROR,
               "Out of memory while reading text %d from %s (pos %d).",
               textId, filename, pos);
      DisplayInternalError();
      return 1;
    }
    strcpy(el->text,rlbuf);
    AddTail((struct List *)&edit_list,(struct Node *)el);
  }
  fclose(fp);
  NiKPermit();
  return 0;
}

void freeeditlist(void) {
  struct EditLine *el;
  while((el = (struct EditLine *)RemHead((struct List *)&edit_list))) {
    FreeMem(el, sizeof(struct EditLine));
  }
  NewList((struct List *)&edit_list);
}

int rek_flyttagren(int rootTextId, int count, int targetConfId) {
  static struct Header header;
  int i = 0;
  long commentIn[MAXKOM];

  if(readtexthead(rootTextId, &header)) {
    return count;
  }
  memcpy(commentIn, header.kom_i, MAXKOM * sizeof(long));
  header.mote = targetConfId;
  if(writetexthead(rootTextId, &header)) {
    return count;
  }
  SetConferenceForText(rootTextId, targetConfId, TRUE);
  count++;
  while(commentIn[i] != -1 && i < MAXKOM) {
    count = rek_flyttagren(commentIn[i], count, targetConfId);
    i++;
  }
  return count;
}

void flyttagren(void) {
  char *confName = hittaefter(argument);
  int rootTextId, confId;

  if(!argument[0] || !confName[0]) {
    SendString("\n\r\r%s\n\r", CATSTR(MSG_MOVE_BRANCH_SYNTAX));
    return;
  }
  rootTextId = atoi(argument);
  if(rootTextId < Servermem->info.lowtext || rootTextId > Servermem->info.hightext) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUMS_NO_SUCH_TEXT));
    return;
  }
  confId = parsemot(confName);
  if(confId == -1) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_GO_NO_SUCH_FORUM));
    return;
  }
  SendStringCat("\n\n\r%s\n\r", CATSTR(MSG_MOVE_BRANCH_MOVED), rek_flyttagren(rootTextId,0,confId));
}
