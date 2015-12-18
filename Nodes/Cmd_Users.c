#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "Terminal.h"
#include "ServerMemUtils.h"
#include "Logging.h"

#include "NiKomLib.h"
#include "NiKomStr.h"
#include "NiKomFuncs.h"

#include "Cmd_Users.h"

extern struct System *Servermem;
extern int nodnr, inloggad;
extern char outbuffer[],inmat[], *argument;


void Cmd_Status(void) {
  struct User readuserstr;
  struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
  int nummer,nod,cnt=0,olasta=FALSE,tot=0;
  struct tm *ts;
  struct UserGroup *listpek;
  char filnamn[100];
  struct UnreadTexts unreadTextsBuf, *unreadTexts;

  if(argument[0]==0) {
    memcpy(&readuserstr,&Servermem->inne[nodnr],sizeof(struct User));
    unreadTexts = &Servermem->unreadTexts[nodnr];
    nummer=inloggad;
  } else {
    if((nummer=parsenamn(argument))==-1) {
      puttekn("\r\n\nFinns ingen som heter så eller har det numret\r\n\n",-1);
      return;
    }
    for(nod=0;nod<MAXNOD;nod++) if(nummer==Servermem->inloggad[nod]) break;
    if(nod<MAXNOD) {
      memcpy(&readuserstr,&Servermem->inne[nod],sizeof(struct User));
      unreadTexts = &Servermem->unreadTexts[nod];
    } else {
      if(readuser(nummer,&readuserstr)) return;
      if(!ReadUnreadTexts(&unreadTextsBuf, nummer)) {
        return;
      }
      unreadTexts = &unreadTextsBuf;
    }
  }
  sprintf(outbuffer,"\r\n\nStatus för %s #%d\r\n\n",readuserstr.namn,nummer);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Status               : %d\r\n",readuserstr.status);
  if(puttekn(outbuffer,-1)) return;
  if(!((readuserstr.flaggor & SKYDDAD)
       && Servermem->inne[nodnr].status<Servermem->cfg.st.sestatus
       && inloggad!=nummer)) {
    sprintf(outbuffer,"Gatuadress           : %s\r\n",readuserstr.gata);
    if(puttekn(outbuffer,-1)) return;
    sprintf(outbuffer,"Postadress           : %s\r\n",readuserstr.postadress);
    if(puttekn(outbuffer,-1)) return;
    sprintf(outbuffer,"Land                 : %s\r\n",readuserstr.land);
    if(puttekn(outbuffer,-1)) return;
    sprintf(outbuffer,"Telefon              : %s\r\n",readuserstr.telefon);
    if(puttekn(outbuffer,-1)) return;
  }
  sprintf(outbuffer,"Annan info           : %s\r\n",readuserstr.annan_info);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Antal rader          : %d\r\n",readuserstr.rader);
  if(puttekn(outbuffer,-1)) return;
  ts=localtime(&readuserstr.forst_in);
  sprintf(outbuffer,"Först inloggad       : %4d%02d%02d  %02d:%02d\r\n",
          ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
          ts->tm_min);
  if(puttekn(outbuffer,-1)) return;
  ts=localtime(&readuserstr.senast_in);
  sprintf(outbuffer,"Senast inloggad      : %4d%02d%02d  %02d:%02d\r\n",
          ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
          ts->tm_min);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Total tid inloggad   : %d:%02d\r\n",readuserstr.tot_tid/3600,
          (readuserstr.tot_tid%3600)/60);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Antal inloggningar   : %d\r\n",readuserstr.loggin);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Antal lästa texter   : %d\r\n",readuserstr.read);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Antal skrivna texter : %d\r\n",readuserstr.skrivit);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Antal downloads      : %d\r\n",readuserstr.download);
  if(puttekn(outbuffer,-1)) return;
  sprintf(outbuffer,"Antal uploads        : %d\r\n",readuserstr.upload);
  if(puttekn(outbuffer,-1)) return;

  if(readuserstr.downloadbytes < 90000)	{
    sprintf(outbuffer,"Antal downloaded B   : %d\r\n",readuserstr.downloadbytes);
    if(puttekn(outbuffer,-1)) return;
  } else {
    sprintf(outbuffer,"Antal downloaded KB  : %d\r\n",readuserstr.downloadbytes/1024);
    if(puttekn(outbuffer,-1)) return;
  }

  if(readuserstr.uploadbytes < 90000) {
    sprintf(outbuffer,"Antal uploaded B     : %d\r\n\n",readuserstr.uploadbytes);
    if(puttekn(outbuffer,-1)) return;
  } else {
    sprintf(outbuffer,"Antal uploaded KB    : %d\r\n\n",readuserstr.uploadbytes/1024);
    if(puttekn(outbuffer,-1)) return;
  }

  if(readuserstr.grupper) {
    puttekn("Grupper:\r\n",-1);
    for(listpek=(struct UserGroup *)Servermem->grupp_list.mlh_Head;listpek->grupp_node.mln_Succ;listpek=(struct UserGroup *)listpek->grupp_node.mln_Succ) {
      if(!BAMTEST((char *)&readuserstr.grupper,listpek->nummer)) continue;
      if((listpek->flaggor & HEMLIGT)
         && !BAMTEST((char *)&Servermem->inne[nodnr].grupper,listpek->nummer)
         && Servermem->inne[nodnr].status<Servermem->cfg.st.medmoten) {
        continue;
      }
      sprintf(outbuffer," %s\r\n",listpek->namn);
      if(puttekn(outbuffer,-1)) return;
    }
    eka('\n');
  }
  if(cnt=countmail(nummer,readuserstr.brevpek)) {
    sprintf(outbuffer,"%4d %s\r\n",cnt,Servermem->cfg.brevnamn);
    if(puttekn(outbuffer,-1)) return;
    olasta=TRUE;
    tot+=cnt;
  }
  for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
    if(motpek->status & SUPERHEMLIGT) continue;
    if(IsMemberConf(motpek->nummer, nummer, &readuserstr)) {
      cnt=0;
      switch(motpek->type) {
      case MOTE_ORGINAL :
        cnt = CountUnreadTexts(motpek->nummer, unreadTexts);
        break;
      default :
        break;
      }
      if(cnt && MaySeeConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
        sprintf(outbuffer,"%4d %s\r\n",cnt,motpek->namn);
        if(puttekn(outbuffer,-1)) return;
        olasta=TRUE;
        tot+=cnt;
      }
    }
  }
  if(!olasta) {
    if(nummer==inloggad) puttekn("Du har inga olästa texter.",-1);
    else {
      sprintf(outbuffer,"%s har inga olästa texter.",readuserstr.namn);
      puttekn(outbuffer,-1);
    }
  } else {
    sprintf(outbuffer,"\r\nSammanlagt %d olästa.",tot);
    puttekn(outbuffer,-1);
  }
  puttekn("\r\n\n",-1);
  sprintf(filnamn,"NiKom:Users/%d/%d/Lapp",nummer/100,nummer);
  if(!access(filnamn,0)) sendfile(filnamn);
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
  if(GetYesOrNo("\r\n\nÄr allt korrekt?", 'j', 'n', "Ja\r\n\n", "Nej\r\n\n",
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
  if(GetYesOrNo(NULL, 'j', 'n', "Ja\r\n\n", "Nej\r\n\n", FALSE, &isCorrect)) {
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
