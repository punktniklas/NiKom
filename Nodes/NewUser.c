#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "Logging.h"
#include "CharacterSets.h"

void initConfPermissions(void);
int createUserDirectory(int newUserId);

extern struct System *Servermem;
extern int nodnr, inloggad;
extern char inmat[];

int RegisterNewUser(void) {
  long tid;
  int newUserId, isCorrect, tmp;
  struct ShortUser *shortUser;
  struct User *user = &Servermem->inne[nodnr];

  memset(user, 0, sizeof(struct User));
  if(AskUserForCharacterSet(TRUE)) {
    return 1;
  }
  SendString("\r\n\n");
  user->rader = Servermem->cfg.defaultrader;
  sendfile("NiKom:Texter/NyAnv.txt");

  for(;;) {
    do {
      SendString("\r\n\nNamn: ");
      if(GetString(40, NULL)) { return 1; }
    } while(inmat[0] == '\0');
    if(parsenamn(inmat) == -1) {
      break;
    }
    SendString("\r\n\nDet finns redan en användare med det namnet.\r\n\n",-1);
  }
  strncpy(user->namn, inmat, 41);
  SendString("\r\nGatuadress: ");
  if(GetString(40, NULL)) { return 1; }
  strncpy(user->gata, inmat, 41);
  SendString("\r\nPostadress: ");
  if(GetString(40, NULL)) { return 1; }
  strncpy(user->postadress, inmat, 41);
  SendString("\r\nLand: ");
  if(GetString(40, NULL)) { return 1; }
  strncpy(user->land, inmat, 41);
  SendString("\r\nTelefon: ");
  if(GetString(20, NULL)) { return 1; }
  strncpy(user->telefon, inmat, 21);
  SendString("\r\nAnnan info: ");
  if(GetString(60, NULL)) { return 1; }
  strncpy(user->annan_info, inmat, 61);
  do {
    if(MaybeEditPassword("Lösenord", "Bekräfta lösenord",
                         user->losen, 15)) {
      return 1;
    }
  } while(user->losen[0] == '\0');
  strcpy(user->prompt, "-->");
  if(MaybeEditString("Prompt", user->prompt, 5)) { return 1; }

  user->tot_tid = 0L;
  time(&tid);
  user->forst_in = tid;
  user->senast_in = 0L;
  user->read = 0L;
  user->skrivit = 0L;
  user->flaggor = Servermem->cfg.defaultflags;
  user->upload = 0;
  user->download = 0;
  user->loggin = 0;
  user->grupper = 0L;
  user->defarea = 0L;
  user->shell = 0;
  user->status = Servermem->cfg.defaultstatus;
  user->protokoll = Servermem->cfg.defaultprotokoll;
  user->brevpek = 0;

  initConfPermissions();
  InitUnreadTexts(&Servermem->unreadTexts[nodnr]);

  SendString("\r\n\nNamn :        %s\r\n", user->namn);
  SendString("Gatuadress :  %s\r\n",user->gata);
  SendString("Postadress :  %s\r\n",user->postadress);
  SendString("Land :        %s\r\n",user->land);
  SendString("Telefon :     %s\r\n",user->telefon);
  SendString("Annan info :  %s\r\n",user->annan_info);
  SendString("Prompt :      %s\r\n",user->prompt);

  if(GetYesOrNo("\r\n\nStämmer detta?", 'j', 'n', "Ja\r\n\n", "Nej\r\n\n",
                TRUE, &isCorrect)) {
    return 1;
  }
  while(!isCorrect) {
    for(;;) {
      SendString("\r\nNamn : (%s) ", user->namn);
      if(GetString(40,NULL)) {
        return 1;
      }
      if(inmat[0] == '\0') {
        break;
      }
      if((tmp = parsenamn(inmat)) != -1) {
        SendString("\r\n\nDet finns redan en användare med det namnet.\r\n");
      } else {
        strncpy(user->namn, inmat, 41);
        break;
      }
    }

    if(MaybeEditString("Gatuadress", user->gata, 40)) { return 1; }
    if(MaybeEditString("Postadress", user->postadress, 40)) { return 1; }
    if(MaybeEditString("Land", user->land, 40)) { return 1; }
    if(MaybeEditString("Telefon", user->telefon, 20)) { return 1; }
    if(MaybeEditString("Annan info", user->annan_info, 60)) { return 1; }
    if(MaybeEditPassword("Lösenord", "Bekräfta lösenord", user->losen, 15)) {
      return 1;
    }
    if(MaybeEditString("Prompt", user->prompt, 5)) { return 1; }

    if(GetYesOrNo("\r\n\nStämmer allt nu?", 'j', 'n', "Ja\r\n\n", "Nej\r\n\n",
                  TRUE, &isCorrect)) {
      return 1;
    }
  }

  newUserId =((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer + 1;
  if(!(shortUser = (struct ShortUser *)AllocMem(sizeof(struct ShortUser),
                                                MEMF_CLEAR | MEMF_PUBLIC))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.\n",
             sizeof(struct ShortUser));
    DisplayInternalError();
    return 2;
  }

  strcpy(shortUser->namn, user->namn);
  shortUser->nummer = newUserId;
  shortUser->status = user->status;
  AddTail((struct List *)&Servermem->user_list, (struct Node *)shortUser);
  if(!createUserDirectory(newUserId)) {
    return 2;
  }
  inloggad = newUserId;
  SendString("\r\n\nDu får användarnumret %d\r\n",inloggad);
  if(Servermem->cfg.ar.nyanv) sendrexx(Servermem->cfg.ar.nyanv);
  return 0;
}

void initConfPermissions(void) {
  struct Mote *conf;
  memset(Servermem->inne[nodnr].motmed, 0, MAXMOTE/8);
  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(conf->status & (SKRIVSTYRT | SLUTET)) {
      BAMCLEAR(Servermem->inne[nodnr].motratt, conf->nummer);
    } else {
      BAMSET(Servermem->inne[nodnr].motratt, conf->nummer);
    }
  }
}

int createUserDirectory(int newUserId) {
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
  sprintf(filename,"NiKom:Users/%d/%d/Data",newUserId/100,newUserId);
  if(!(fh=Open(filename,MODE_NEWFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not create file %s", filename);
    DisplayInternalError();
    return 0;
  }
  if(Write(fh,(void *)&Servermem->inne[nodnr],sizeof(struct User))==-1) {
    LogEvent(SYSTEM_LOG, ERROR, "Error writing to file %s", filename);
    DisplayInternalError();
    Close(fh);
    return 0;
  }
  Close(fh);
  
  if(!WriteUnreadTexts(&Servermem->unreadTexts[nodnr], newUserId)) {
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
