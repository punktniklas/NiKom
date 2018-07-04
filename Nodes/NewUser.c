#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "Logging.h"
#include "CharacterSets.h"
#include "InfoFiles.h"
#include "Languages.h"
#include "UserDataUtils.h"

#include "NewUser.h"

void initConfPermissions(struct User *user);
int createUserDirectory(struct User *user, int newUserId);

extern struct System *Servermem;
extern int nodnr;
extern char inmat[];
extern struct UnreadTexts g_newUserUnreadTexts;

int RegisterNewUser(struct User *user) {
  long tid;
  int newUserId, isCorrect, tmp;
  struct ShortUser *shortUser;

  memset(user, 0, sizeof(struct User));

  AskUserForLanguage(user);
  if(Servermem->cfg->defaultcharset == 0) {
    if(AskUserForCharacterSet(user, TRUE, FALSE)) {
      return -1;
    }
  } else {
    user->chrset = Servermem->cfg->defaultcharset;
  }

  SendString("\r\n\n");
  user->rader = Servermem->cfg->defaultrader;
  SendInfoFile("NewUser.txt", 0);

  for(;;) {
    do {
      SendString("\r\n\n%s: ", CATSTR(MSG_USER_NAME));
      if(GetString(40, NULL)) { return -1; }
    } while(inmat[0] == '\0');
    if(parsenamn(inmat) == -1) {
      break;
    }
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_USER_ALREADY_EXISTS));
  }
  strncpy(user->namn, inmat, 41);
  SendString("\r\n%s: ", CATSTR(MSG_USER_STREET));
  if(GetString(40, NULL)) { return -1; }
  strncpy(user->gata, inmat, 41);
  SendString("\r\n%s: ", CATSTR(MSG_USER_CITY));
  if(GetString(40, NULL)) { return -1; }
  strncpy(user->postadress, inmat, 41);
  SendString("\r\n%s: ", CATSTR(MSG_USER_COUNTRY));
  if(GetString(40, NULL)) { return -1; }
  strncpy(user->land, inmat, 41);
  SendString("\r\n%s: ", CATSTR(MSG_USER_PHONE));
  if(GetString(20, NULL)) { return -1; }
  strncpy(user->telefon, inmat, 21);
  SendString("\r\n%s: ", CATSTR(MSG_USER_MISCINFO));
  if(GetString(60, NULL)) { return -1; }
  strncpy(user->annan_info, inmat, 61);
  do {
    if(MaybeEditPassword(CATSTR(MSG_USER_PASSWORD), CATSTR(MSG_USER_PASSWORD_CONFIRM),
                         user->losen, 15)) {
      return -1;
    }
  } while(user->losen[0] == '\0');
  strcpy(user->prompt, "-->");
  if(MaybeEditString(CATSTR(MSG_USER_PROMPT), user->prompt, 5)) { return -1; }

  user->tot_tid = 0L;
  time(&tid);
  user->forst_in = tid;
  user->senast_in = 0L;
  user->read = 0L;
  user->skrivit = 0L;
  user->flaggor = Servermem->cfg->defaultflags;
  user->upload = 0;
  user->download = 0;
  user->loggin = 0;
  user->grupper = 0L;
  user->defarea = 0L;
  user->shell = 0;
  user->status = Servermem->cfg->defaultstatus;
  user->brevpek = 0;

  initConfPermissions(user);
  InitUnreadTexts(&g_newUserUnreadTexts);

  SendString("\r\n\n%s : %s\r\n", CATSTR(MSG_USER_NAME), user->namn);
  SendString("%-20s : %s\r\n", CATSTR(MSG_USER_STREET), user->gata);
  SendString("%-20s : %s\r\n", CATSTR(MSG_USER_CITY), user->postadress);
  SendString("%-20s : %s\r\n", CATSTR(MSG_USER_COUNTRY), user->land);
  SendString("%-20s : %s\r\n", CATSTR(MSG_USER_PHONE), user->telefon);
  SendString("%-20s : %s\r\n", CATSTR(MSG_USER_MISCINFO), user->annan_info);
  SendString("%-20s : %s\r\n", CATSTR(MSG_USER_PROMPT), user->prompt);

  if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMON_IS_CORRECT), NULL, NULL,
                CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n\n",
                TRUE, &isCorrect)) {
    return -1;
  }
  while(!isCorrect) {
    for(;;) {
      SendString("\r\n%s : (%s) ", CATSTR(MSG_USER_NAME), user->namn);
      if(GetString(40,NULL)) {
        return -1;
      }
      if(inmat[0] == '\0') {
        break;
      }
      if((tmp = parsenamn(inmat)) != -1) {
        SendString("\r\n\n%s\r\n", CATSTR(MSG_USER_ALREADY_EXISTS));
      } else {
        strncpy(user->namn, inmat, 41);
        break;
      }
    }

    if(MaybeEditString(CATSTR(MSG_USER_STREET), user->gata, 40)) { return -1; }
    if(MaybeEditString(CATSTR(MSG_USER_CITY), user->postadress, 40)) { return -1; }
    if(MaybeEditString(CATSTR(MSG_USER_COUNTRY), user->land, 40)) { return -1; }
    if(MaybeEditString(CATSTR(MSG_USER_PHONE), user->telefon, 20)) { return -1; }
    if(MaybeEditString(CATSTR(MSG_USER_MISCINFO), user->annan_info, 60)) { return -1; }
    if(MaybeEditPassword(CATSTR(MSG_USER_PASSWORD), CATSTR(MSG_USER_PASSWORD_CONFIRM), user->losen, 15)) {
      return -1;
    }
    if(MaybeEditString(CATSTR(MSG_USER_PROMPT), user->prompt, 5)) { return -1; }

    if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMON_IS_CORRECT), NULL, NULL,
                  CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n\n",
                  TRUE, &isCorrect)) {
      return -1;
    }
  }

  newUserId =((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer + 1;
  if(!(shortUser = (struct ShortUser *)AllocMem(sizeof(struct ShortUser),
                                                MEMF_CLEAR | MEMF_PUBLIC))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.\n",
             sizeof(struct ShortUser));
    DisplayInternalError();
    return -2;
  }

  strcpy(shortUser->namn, user->namn);
  shortUser->nummer = newUserId;
  shortUser->status = user->status;
  AddTail((struct List *)&Servermem->user_list, (struct Node *)shortUser);
  if(!createUserDirectory(user, newUserId)) {
    return -2;
  }
  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_USER_YOU_GET_ID), newUserId);
  return newUserId;
}

void initConfPermissions(struct User *user) {
  struct Mote *conf;
  memset(user->motmed, 0, MAXMOTE/8);
  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(conf->status & (SKRIVSTYRT | SLUTET)) {
      BAMCLEAR(user->motratt, conf->nummer);
    } else {
      BAMSET(user->motratt, conf->nummer);
    }
  }
}

int createUserDirectory(struct User *user, int newUserId) {
  BPTR lock, fh;
  char dirPath[100], filename[40];

  sprintf(dirPath,"NiKom:Users/%d", newUserId / 100);
  if(!(lock = Lock(dirPath, ACCESS_READ))) {
    if(!(lock=CreateDir(dirPath))) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not create directory %s", dirPath);
      DisplayInternalError();
      return 0;
    }
  }
  UnLock(lock);
  sprintf(dirPath,"NiKom:Users/%d/%d",newUserId/100,newUserId);
  if(!(lock=Lock(dirPath,ACCESS_READ))) {
    if(!(lock=CreateDir(dirPath))) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not create directory %s", dirPath);
      DisplayInternalError();
      return 0;
    }
  }
  UnLock(lock);
  if(!WriteUser(newUserId, user, TRUE)) {
    return 0;
  }
  
  if(!WriteUnreadTexts(&g_newUserUnreadTexts, newUserId)) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not create UnreadTexts for user %d",
             newUserId);
    DisplayInternalError();
    return 0;
  }
  
  sprintf(filename,"Nikom:Users/%d/%d/.firstletter",newUserId/100,newUserId);
  if(!(fh=Open(filename,MODE_NEWFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not create %s", filename);
    DisplayInternalError();
    return 0;
  }
  Write(fh,"0",1);
  Close(fh);
  sprintf(filename,"Nikom:Users/%d/%d/.nextletter",newUserId/100,newUserId);
  if(!(fh=Open(filename,MODE_NEWFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not create %s", filename);
    DisplayInternalError();
    return 0;
  }
  Write(fh,"0",1);
  Close(fh);
  return 1;
}
