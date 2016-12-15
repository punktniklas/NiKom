#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "Terminal.h"
#include "ServerMemUtils.h"
#include "Logging.h"
#include "BasicIO.h"
#include "Languages.h"

#include "NiKomLib.h"
#include "NiKomStr.h"
#include "NiKomFuncs.h"

#include "Cmd_Users.h"

extern struct System *Servermem;
extern int nodnr, inloggad;
extern char outbuffer[],inmat[], *argument;


void Cmd_Status(void) {
  struct User readuserstr;
  struct Mote *conf;
  int userId, nod, cnt = 0, sumUnread = 0, showAllConf = FALSE;
  struct tm *ts;
  struct UserGroup *listpek;
  char filnamn[100];
  struct UnreadTexts unreadTextsBuf, *unreadTexts;

  if(argument[0] == '-' && (argument[1] == 'a' || argument[1] == 'A')) {
    showAllConf = TRUE;
    argument = hittaefter(argument);
  }
  if(argument[0] == 0) {
    memcpy(&readuserstr, &Servermem->inne[nodnr], sizeof(struct User));
    unreadTexts = &Servermem->unreadTexts[nodnr];
    userId = inloggad;
  } else {
    if((userId = parsenamn(argument)) == -1) {
      SendString("\r\n\nFinns ingen som heter så eller har det numret\r\n\n");
      return;
    }
    for(nod = 0; nod < MAXNOD; nod++) {
      if(userId == Servermem->inloggad[nod]) {
        break;
      }
    }
    if(nod < MAXNOD) {
      memcpy(&readuserstr, &Servermem->inne[nod], sizeof(struct User));
      unreadTexts = &Servermem->unreadTexts[nod];
    } else {
      if(readuser(userId,&readuserstr)) {
        return;
      }
      if(!ReadUnreadTexts(&unreadTextsBuf, userId)) {
        return;
      }
      unreadTexts = &unreadTextsBuf;
    }
  }
  if(SendString("\r\n\nStatus för %s #%d\r\n\n", readuserstr.namn,userId)) { return; }
  if(SendString("Status               : %d\r\n", readuserstr.status)) { return; }
  if(!((readuserstr.flaggor & SKYDDAD)
       && Servermem->inne[nodnr].status < Servermem->cfg.st.sestatus
       && inloggad != userId)) {
    if(SendString("Gatuadress           : %s\r\n", readuserstr.gata)) { return; }
    if(SendString("Postadress           : %s\r\n", readuserstr.postadress)) { return; }
    if(SendString("Land                 : %s\r\n", readuserstr.land)) { return; }
    if(SendString("Telefon              : %s\r\n", readuserstr.telefon)) { return; }
  }
  if(SendString("Annan info           : %s\r\n", readuserstr.annan_info)) { return; }
  if(SendString("Antal rader          : %d\r\n", readuserstr.rader)) { return; }
  ts = localtime(&readuserstr.forst_in);
  if(SendString("Först inloggad       : %4d%02d%02d  %02d:%02d\r\n",
          ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
          ts->tm_min)) { return; }
  ts = localtime(&readuserstr.senast_in);
  if(SendString("Senast inloggad      : %4d%02d%02d  %02d:%02d\r\n",
          ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
          ts->tm_min)) { return; }
  if(SendString("Total tid inloggad   : %d:%02d\r\n", readuserstr.tot_tid / 3600,
          (readuserstr.tot_tid % 3600) / 60)) { return; }
  if(SendString("Antal inloggningar   : %d\r\n", readuserstr.loggin)) { return; }
  if(SendString("Antal lästa texter   : %d\r\n", readuserstr.read)) { return; }
  if(SendString("Antal skrivna texter : %d\r\n", readuserstr.skrivit)) { return; }
  if(SendString("Antal downloads      : %d\r\n", readuserstr.download)) { return; }
  if(SendString("Antal uploads        : %d\r\n", readuserstr.upload)) { return; }

  if(readuserstr.downloadbytes < 90000)	{
    if(SendString("Antal downloaded B   : %d\r\n", readuserstr.downloadbytes)) { return; }
  } else {
    if(SendString("Antal downloaded KB  : %d\r\n", readuserstr.downloadbytes / 1024)) { return; }
  }

  if(readuserstr.uploadbytes < 90000) {
    if(SendString("Antal uploaded B     : %d\r\n\n", readuserstr.uploadbytes)) { return; }
  } else {
    if(SendString("Antal uploaded KB    : %d\r\n\n", readuserstr.uploadbytes / 1024)) { return; }
  }

  if(readuserstr.grupper) {
    SendString("Grupper:\r\n");
    ITER_EL(listpek, Servermem->grupp_list, grupp_node, struct UserGroup *) {
      if(!BAMTEST((char *)&readuserstr.grupper, listpek->nummer)) {
        continue;
      }
      if((listpek->flaggor & HEMLIGT)
         && !BAMTEST((char *)&Servermem->inne[nodnr].grupper, listpek->nummer)
         && Servermem->inne[nodnr].status < Servermem->cfg.st.medmoten) {
        continue;
      }
      if(SendString(" %s\r\n", listpek->namn)) { return; }
    }
    SendString("\n");
  }
  if(cnt = countmail(userId, readuserstr.brevpek)) {
    if(SendString("%4d %s\r\n", cnt, Servermem->cfg.brevnamn)) { return; }
    sumUnread += cnt;
  }
  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(!MaySeeConf(conf->nummer, inloggad, &Servermem->inne[nodnr])
       || !IsMemberConf(conf->nummer, userId, &readuserstr)) {
      continue;
    }
    cnt = 0;
    switch(conf->type) {
    case MOTE_ORGINAL :
      cnt = CountUnreadTexts(conf->nummer, unreadTexts);
      break;
    case MOTE_FIDO:
      cnt = conf->texter - unreadTexts[nodnr].lowestPossibleUnreadText[conf->nummer] + 1;
      break;
    default :
      break;
    }
    if(cnt > 0 || showAllConf) {
      if(SendString("%4d %s\r\n", cnt, conf->namn)) { return; }
      sumUnread += cnt;
    }
  }
  if(sumUnread == 0) {
    if(userId == inloggad) {
      SendString("\r\nDu har inga olästa texter.");
    } else {
      if(SendString("\r\n%s har inga olästa texter.",readuserstr.namn)) { return; }
    }
  } else {
    if(SendString("\r\nSammanlagt %d olästa.",sumUnread)) { return; }
  }
  SendString("\r\n\n");
  sprintf(filnamn, "NiKom:Users/%d/%d/Lapp", userId / 100, userId);
  if(!access(filnamn,0)) {
    sendfile(filnamn);
  }
}

int Cmd_ChangeUser(void) {
  int userId, tmp, i, isCorrect;
  struct User user;
  struct ShortUser *shortUser;
  if(argument[0]) {
    if(Servermem->inne[nodnr].status < Servermem->cfg.st.anv) {
      SendString("\r\n\nDu kan inte ändra andra användare än dig själv!\r\n\n");
      return 0;
    }
    if((userId = parsenamn(argument)) == -1) {
      SendString("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n");
      return 0;
    } else if(userId == -3) {
      LogEvent(SYSTEM_LOG, ERROR,
               "-3 returned from parsenamn() in Cmd_ChangeUser(). Argument = '%s'",
               argument);
      DisplayInternalError();
      return 0;
    }
    for(i = 0; i < MAXNOD; i++) {
      if(userId==Servermem->inloggad[i]) {
        break;
      }
    }
    if( i < MAXNOD) {
      memcpy(&user,&Servermem->inne[i],sizeof(struct User));
    }
    else {
      if(readuser(userId,&user)) {
        DisplayInternalError();
        return 0;
      }
      SendString("\r\n\nÄndrar användare %s\r\n", getusername(userId));
    }
  } else {
    memcpy(&user, &Servermem->inne[nodnr], sizeof(struct User));
    userId = inloggad;
    SendString("\r\n\nÄndrar egen användarinformation\r\n");
  }

  for(;;) {
    SendString("\r\nNamn : (%s) ", user.namn);
    if(GetString(40,NULL)) {
      return 1;
    }
    if(inmat[0] == '\0') {
      break;
    }
    if((tmp = parsenamn(inmat)) != -1 && tmp != userId) {
      SendString("\r\n\nDet finns redan en användare med det namnet.\r\n");
    } else {
      strncpy(user.namn, inmat, 41);
      break;
    }
  }

  if(Servermem->inne[nodnr].status >= Servermem->cfg.st.chgstatus) {
    if(MaybeEditNumberChar("Status", &user.status, 3, 0, 100)) { return 1; }
  }
  if(MaybeEditString("Gatuadress", user.gata, 40)) { return 1; }
  if(MaybeEditString("Postadress", user.postadress, 40)) { return 1; }
  if(MaybeEditString("Land", user.land, 40)) { return 1; }
  if(MaybeEditString("Telefon", user.telefon, 20)) { return 1; }
  if(MaybeEditString("Annan info", user.annan_info, 60)) { return 1; }
  if(MaybeEditPassword("Lösenord", "Bekräfta lösenord", user.losen, 15)) { return 1; }
  if(MaybeEditString("Prompt", user.prompt, 5)) { return 1; }
  if(MaybeEditNumberChar("Antal rader", &user.rader, 5, 0, 127)) { return 1; }

  if(Servermem->inne[nodnr].status>=Servermem->cfg.st.anv) {
    if(MaybeEditNumber("Antal lästa", (int *)&user.read, 8, 0, INT_MAX)) { return 1; }
    if(MaybeEditNumber("Antal skrivna", (int *)&user.skrivit, 8, 0, INT_MAX)) { return 1; }
    if(MaybeEditNumber("Antal downloads", (int *)&user.download, 8, 0, INT_MAX)) { return 1; }
    if(MaybeEditNumber("Antal uploads", (int *)&user.upload, 8, 0, INT_MAX)) { return 1; }
    if(MaybeEditNumber("Antal dlbytes", (int *)&user.downloadbytes, 8, 0, INT_MAX)) { return 1; }
    if(MaybeEditNumber("Antal ulbytes", (int *)&user.uploadbytes, 8, 0, INT_MAX)) { return 1; }
    if(MaybeEditNumber("Antal inloggningar", (int *)&user.loggin, 8, 0, INT_MAX)) { return 1; }
  }
  if(GetYesOrNo("\r\n\n", "Är allt korrekt?", NULL, NULL, "Ja", "Nej", "\r\n\n",
                TRUE, &isCorrect)) {
    return 1;
  }
  if(!isCorrect) {
    return 0;
  }

  if((shortUser = FindShortUser(userId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "While changing user %d, can't find ShortUser entry.", userId);
    DisplayInternalError();
    return 0;
  }
  strncpy(shortUser->namn, user.namn, 41);
  shortUser->status = user.status;

  for(i=0; i < MAXNOD; i++) {
    if(userId == Servermem->inloggad[i]) {
      strncpy(Servermem->inne[i].namn, user.namn, 41);
      Servermem->inne[i].status = user.status;
      strncpy(Servermem->inne[i].gata, user.gata, 41);
      strncpy(Servermem->inne[i].postadress, user.postadress, 41);
      strncpy(Servermem->inne[i].land, user.land, 41);
      strncpy(Servermem->inne[i].telefon, user.telefon, 21);
      strncpy(Servermem->inne[i].annan_info, user.annan_info, 61);
      strncpy(Servermem->inne[i].losen, user.losen, 16);
      strncpy(Servermem->inne[i].prompt, user.prompt, 6);
      Servermem->inne[i].rader = user.rader;
      Servermem->inne[i].read = user.read;
      Servermem->inne[i].skrivit = user.skrivit;
      Servermem->inne[i].download = user.download;
      Servermem->inne[i].upload = user.upload;
      Servermem->inne[i].uploadbytes = user.uploadbytes;
      Servermem->inne[i].downloadbytes = user.downloadbytes;
      Servermem->inne[i].loggin = user.loggin;
      break;
    }
  }
  writeuser(userId, &user);
  return(0);
}

void Cmd_DeleteUser(void) {
  struct ShortUser *shortUser;
  int userId, isCorrect;
  if(!argument[0]) {
    SendString("\r\n\nSkriv: Radera Användare <användare>\r\n\n");
    return;
  }
  if((userId = parsenamn(argument)) == -1) {
    SendString("\r\n\nFinns ingen som heter så eller har det numret\r\n\n");
    return;
  }
  SendString("\r\n\nÄr du säker på att du vill radera %s?",
             getusername(userId));
  if(GetYesOrNo(NULL, NULL, NULL, NULL, "Ja", "Nej", "\r\n\n", FALSE, &isCorrect)) {
    return;
  }
  if(!isCorrect) {
    return;
  }

  if((shortUser = FindShortUser(userId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Error finding ShortUser entry for user %d in Cmd_DeleteUser()",
             userId);
    DisplayInternalError();
    return;
  }

  Remove((struct Node *)shortUser);
  sprintf(inmat, "%d", userId);
  argument = inmat;
  sendrexx(15);
}

int Cmd_ListUsers(void) {
  struct ShortUser *listpek;
  int backwards=FALSE,status=-1,going=TRUE,more=0,less=100,cnt=2;
  char pattern[40]="",*argpek=argument,*tpek=NULL;
  puttekn("\r\n\n",-1);
  while(argpek[0] && going) {
    if(argpek[0]=='-') {
      if(argpek[1]=='b' || argpek[1]=='B') backwards=TRUE;
      else if(argpek[1]='s' || argpek[1]=='S') {
        while(argpek[cnt] && argpek[cnt]!=' ') {
          if(argpek[cnt]=='-') { tpek=&argpek[cnt+1]; break; }
          cnt++;
        }
        if(tpek) {
          if(argpek[2]!='-') more=atoi(&argpek[2]);
          if(tpek[0] && tpek[0]!=' ') less=atoi(tpek);
        } else status=atoi(&argpek[2]);
      }
    } else {
      strncpy(pattern,argpek,39);
      going=FALSE;
    }
    argpek=hittaefter(argpek);
  }
  if(backwards) {
    for(listpek=(struct ShortUser *)Servermem->user_list.mlh_TailPred;listpek->user_node.mln_Pred;listpek=(struct ShortUser *)listpek->user_node.mln_Pred) {
      if(status!=-1 && status!=listpek->status) continue;
      if(listpek->status>less || listpek->status<more) continue;
      if(!namematch(pattern,listpek->namn)) continue;
      sprintf(outbuffer,"%s #%d\r\n",listpek->namn,listpek->nummer);
      if(puttekn(outbuffer,-1)) break;
    }
    return(0);
  }
  for(listpek=(struct ShortUser *)Servermem->user_list.mlh_Head;listpek->user_node.mln_Succ;listpek=(struct ShortUser *)listpek->user_node.mln_Succ) {
    if(status!=-1 && status!=listpek->status) continue;
    if(listpek->status>less || listpek->status<more) continue;
    if(!namematch(pattern,listpek->namn)) continue;
    sprintf(outbuffer,"%s #%d\r\n",listpek->namn,listpek->nummer);
    if(puttekn(outbuffer,-1)) break;
  }
  return(0);
}

static char *languages[] = {
  "English",
  "Svenska"
};

void Cmd_ChangeLanguage(void) {
  int lang, i;

  SendString("\n\n\rDessa språk finns.\n\r");
  SendString("* markerar nuvarande val.\n\n\r");

  SendString("  Nr Språk\n\r");
  SendString("-----------------------------------------\n\r");
  for(i = 0; i < 2; i++) {
    SendString("%c %2d: %s\n\r", Servermem->inne[nodnr].language == i ? '*' : ' ', i, languages[i]);
  }
  
  SendString("\n\rVal: ");
  lang = GetNumber(0, 1, NULL);
  if(ImmediateLogout()) {
    return;
  }
  SendString("%s\n\r", languages[lang]);
  Servermem->inne[nodnr].language = lang;
  LoadCatalogForUser();
}
