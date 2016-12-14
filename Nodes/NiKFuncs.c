#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "StringUtils.h"
#include "Terminal.h"
#include "Cmd_Users.h"
#include "NiKVersion.h"
#include "BasicIO.h"
#include "KOM.h"
#include "Languages.h"

#define EKO             1
#define EJEKO   0
#define KOM             1
#define EJKOM   0

extern struct System *Servermem;
extern int nodnr,inloggad,radcnt,rxlinecount, nodestate;
extern char outbuffer[],inmat[],backspace[],commandhistory[][1024];
extern struct ReadLetter brevread;
extern struct MinList edit_list;

long logintime, extratime;
int mote2,rad,senast_text_typ,nu_skrivs,area2,
        senast_brev_nr,senast_brev_anv,senast_text_nr,senast_text_mote;
int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf;
char *monthNames[2][12] =
  {{ "January", "February", "March", "April", "May", "June",
     "July", "August", "September", "October", "November", "December" },
   { "januari", "februari", "mars", "april", "maj", "juni",
     "juli", "augusti", "september", "oktober", "november", "december" }};
char *weekdayNames[2][7] =
  {{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
   { "Söndag", "Måndag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lördag" }};
char *argument,usernamebuf[50],argbuf[1081],vilkabuf[50];
struct Header sparhead,readhead;
struct Inloggning Statstr;
struct MinList aliaslist;

void atersekom(void) {
        struct Mote *motpek;
        if(senast_text_typ==BREV) {
                if(!brevread.reply[0]) puttekn("\r\n\nTexten är inte någon kommentar!\r\n\n",-1);
                else visabrev(atoi(hittaefter(brevread.reply)),atoi(brevread.reply));
        }
        else if(senast_text_typ==TEXT) {
                motpek=getmotpek(senast_text_mote);
                if(motpek->type==MOTE_ORGINAL) {
                        if(readhead.kom_till_nr==-1) puttekn("\r\n\nTexten är inte någon kommentar!\r\n\n",-1);
                        else if(readhead.kom_till_nr<Servermem->info.lowtext) puttekn("\r\n\nTexten finns inte!\r\n\n",-1);
                        else org_visatext(readhead.kom_till_nr, FALSE);
                }
                else if(motpek->type==MOTE_FIDO) puttekn("\n\n\rÅterse Kommenterade kan inte användas på Fido-texter\n\r",-1);
        }
        else puttekn("\r\n\nDu har inte läst någon text ännu!\r\n\n",-1);
}

void igen(void) {
        struct Mote *motpek;
        if(senast_text_typ==BREV) visabrev(senast_brev_nr,senast_brev_anv);
        else if(senast_text_typ==TEXT) {
                motpek=getmotpek(senast_text_mote);
                if(motpek->type==MOTE_ORGINAL) org_visatext(senast_text_nr, FALSE);
                else if(motpek->type==MOTE_FIDO) fido_visatext(senast_text_nr,motpek);
        }
        else puttekn("\r\n\nDu har inte läst någon text ännu!\r\n\n",-1);
}

int skriv(void) {
        struct Mote *motpek;
        if(mote2==-1) {
                sprintf(outbuffer,"\r\n\nAnvänd kommandot 'Brev' i %s.\r\n",Servermem->cfg.brevnamn);
                puttekn(outbuffer,-1);
                return(0);
        }
        motpek=getmotpek(mote2);
        if(!motpek) {
                puttekn("\n\n\rHmm.. Mötet du befinner dig i finns inte.\n\r",-1);
                return(0);
        }
        if(!MayWriteConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
                puttekn("\r\n\nDu får inte skriva i det här mötet!\r\n\n",-1);
                return(0);
        }
        if(motpek->type==MOTE_ORGINAL) return(org_skriv());
        else if(motpek->type==MOTE_FIDO) return(fido_skriv(EJKOM,0));
        return(0);
}

void var(int mot) {
        if(mot==-1) varmail();
        else varmote(mot);
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
          weekdayNames[Servermem->inne[nodnr].language][ts->tm_wday], ts->tm_mday,
          monthNames[Servermem->inne[nodnr].language][ts->tm_mon],
          1900 + ts->tm_year, ts->tm_hour, ts->tm_min);
  if(Servermem->cfg.maxtid[Servermem->inne[nodnr].status]) {
    timeLimitSeconds = 60 * (Servermem->cfg.maxtid[Servermem->inne[nodnr].status]) + extratime;
    SendStringCat("\n%s\r\n", CATSTR(MSG_KOM_TIME_LEFT), (timeLimitSeconds - (now - logintime)) / 60);
  }
}

int skapmot(void) {
  struct ShortUser *shortUser;
  int mad, setPermission, changed, ch, i, fidoDomainId, highestId;
  struct FidoDomain *fidoDomain;
  BPTR lock;
  struct User user;
  struct Mote tmpConf,*searchConf,*newConf;

  memset(&tmpConf, 0, sizeof(struct Mote));
  if(argument[0] == '\0') {
    SendString("\r\n\nNamn på mötet: ");
    if(GetString(40,NULL)) {
      return 1;
    }
    strcpy(tmpConf.namn, inmat);
  } else {
    strcpy(tmpConf.namn, argument);
  }
  if(parsemot(tmpConf.namn) != -1) {
    SendString("\r\n\nFinns redan ett sådant möte!\r\n");
    return 0;
  }
  tmpConf.skapat_tid = time(NULL);;
  tmpConf.skapat_av = inloggad;
  for(;;) {
    SendString("\r\nMötesAdministratör (MAD) : ");
    if(GetString(5,NULL)) {
      return 1;
    }
    if(inmat[0]) {
      if((mad = parsenamn(inmat)) == -1) {
        SendString("\r\nFinns ingen sådan användare!");
      } else {
        tmpConf.mad = mad;
        break;
      }
    }
  }
  SendString("\n\rSorteringsvärde: ");
  tmpConf.sortpri = GetNumber(0, LONG_MAX, NULL);

  if(EditBitFlagShort("\r\nSka mötet vara slutet?", 'j', 'n', "Slutet", "Öppet",
                     &tmpConf.status, SLUTET)) {
    return 1;
  }
  if(tmpConf.status & SLUTET) {
    SendString("\r\nVilka grupper ska ha tillgång till mötet? (? för lista)\r\n");
    if(editgrupp((char *)&tmpConf.grupper)) {
      return 1;
    }
  }
  if(EditBitFlagShort("\r\nSka mötet vara skrivskyddat?", 'j', 'n',
                     "Skyddat", "Oskyddat", &tmpConf.status, SKRIVSKYDD)) {
    return 1;
  }
  if(EditBitFlagShort("\r\nSka mötet vara kommentarsskyddat?", 'j', 'n',
                     "Skyddat", "Oskyddat", &tmpConf.status, KOMSKYDD)) {
    return 1;
  }
  if(EditBitFlagShort("\r\nSka mötet vara hemligt?", 'j', 'n',
                     "Hemligt", "Ej hemligt", &tmpConf.status, HEMLIGT)) {
    return 1;
  }
  if(!(tmpConf.status & SLUTET)) {
    if(EditBitFlagShort("\r\nSka alla användare bli medlemmar automagiskt?", 'j', 'n',
                       "Ja", "Nej", &tmpConf.status, AUTOMEDLEM)) {
      return 1;
    }
    if(EditBitFlagShort("\r\nSka rättigheterna styra skrivmöjlighet?", 'j', 'n',
                       "Ja", "Nej", &tmpConf.status, SKRIVSTYRT)) {
      return 1;
    }
    if(tmpConf.status & SKRIVSTYRT) {
      SendString("\r\nVilka grupper ska ha tillgång till mötet? (? för lista)\r\n");
      if(editgrupp((char *)&tmpConf.grupper)) {
        return 1;
      }
    }
  }
  if(EditBitFlagShort("\r\nSka mötet enbart kommas åt från ARexx?", 'j', 'n',
                     "Ja", "Nej", &tmpConf.status, SUPERHEMLIGT)) {
    return 1;
  }

  SendString("\n\n\rVilken typ av möte ska det vara?\n\r");
  SendString("1: Lokalt möte\n\r");
  SendString("2: Fido-möte\n\n\r");
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
    SendString("Lokalt möte\n\n\r");
    tmpConf.type = MOTE_ORGINAL;
  } else {
    SendString("Fido-möte\n\n\r");
    tmpConf.type = MOTE_FIDO;
    if(EditString("Katalog for .msg-filerna", tmpConf.dir, 79, TRUE)) {
      return 1;
    }
    if(!(lock = Lock(tmpConf.dir, SHARED_LOCK))) {
      if(!(lock = CreateDir(tmpConf.dir)))
        SendString("\n\rKunde inte skapa katalogen\n\r");
    }
    if(lock) {
      UnLock(lock);
    }
    if(EditString("FidoNet tag-namn", tmpConf.tagnamn, 49, TRUE)) {
      return 1;
    }
    strcpy(tmpConf.origin, Servermem->fidodata.defaultorigin);
    if(MaybeEditString("Origin-rad", tmpConf.origin, 69)) {
      return 1;
    }
    
    SendString("\n\n\rVilken teckenuppsättning ska användas för utgående texter?\n\r");
    SendString("1: ISO Latin 1 (ISO 8859-1)\n\r");
    SendString("2: SIS-7 (SF7, 'Måsvingar')\n\r");
    SendString("3: IBM CodePage\n\r");
    SendString("4: Mac\n\n\r");
    SendString("Val: ");
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
      SendString("ISO Latin 1\n\n\r");
      tmpConf.charset = CHRS_LATIN1;
      break;
    case '2':
      SendString("SIS-7\n\n\r");
      tmpConf.charset = CHRS_SIS7;
      break;
    case '3':
      SendString("IBM CodePage\n\n\r");
      tmpConf.charset = CHRS_CP437;
      break;
    case '4':
      SendString("Mac\n\n\r");
      tmpConf.charset = CHRS_MAC;
      break;
    }
    SendString("Vilken domän är mötet i?\n\r");
    highestId = 0;
    for(i = 0; i < 10; i++) {
      if(!Servermem->fidodata.fd[i].domain[0]) {
        break;
      }
      highestId = max(highestId, Servermem->fidodata.fd[i].nummer);
      SendString("%3d: %s (%d:%d/%d.%d)\n\r",
                 Servermem->fidodata.fd[i].nummer,
                 Servermem->fidodata.fd[i].domain,
                 Servermem->fidodata.fd[i].zone,
                 Servermem->fidodata.fd[i].net,
                 Servermem->fidodata.fd[i].node,
                 Servermem->fidodata.fd[i].point);
    }
    if(i == 0) {
      SendString("\n\rDu måste definiera en domän i NiKomFido.cfg först!\n\r");
      return 0;
    }
    for(;;) {
      SendString("\r\nDomän: ");
      if(GetString(5, NULL)) {
        return 1;
      }
      fidoDomainId = atoi(inmat);
      if(fidoDomain = getfidodomain(fidoDomainId, 0)) {
        break;
      } else {
        SendString("\n\rFinns ingen sådan domän.\n\r");
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
    SendString("\n\n\rDet finns inte plats för fler möten.\n\r");
    return 0;
  }
  tmpConf.nummer = i;
  if(!(newConf = (struct Mote *)AllocMem(sizeof(struct Mote),
                                         MEMF_CLEAR | MEMF_PUBLIC))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.", sizeof(struct Mote));
    DisplayInternalError();
    return 0;
  }
  memcpy(newConf, &tmpConf, sizeof(struct Mote));
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
  for(i = 0; i < MAXNOD; i++) {
    BAMCLEAR(Servermem->inne[i].motmed, newConf->nummer);
    if(setPermission) {
      BAMSET(Servermem->inne[i].motratt, newConf->nummer);
    } else {
      BAMCLEAR(Servermem->inne[i].motratt, newConf->nummer);
    }
  }

  SendString("\r\nÄndrar i användardata..\r\n");
  ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
    if(!(shortUser->nummer % 10)) {
      SendString("\r%d", shortUser->nummer);
    }
    if(readuser(shortUser->nummer, &user)) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not read user %d to set "
               "membership/permissions for new conference.", shortUser->nummer);
      DisplayInternalError();
      return 0;
    }
    changed = FALSE;
    if(setPermission != BAMTEST(user.motratt, newConf->nummer)) {
      if(setPermission) {
        BAMSET(user.motratt, newConf->nummer);
      } else {
        BAMCLEAR(user.motratt, newConf->nummer);
      }
      changed = TRUE;
    }
    if(!(newConf->status & AUTOMEDLEM) && BAMTEST(user.motmed, newConf->nummer)) {
      BAMCLEAR(user.motmed, newConf->nummer);
      changed = TRUE;
    }
    if(changed && writeuser(shortUser->nummer, &user)) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not write user %d to set "
               "membership/permissions for new conference.", shortUser->nummer);
      DisplayInternalError();
      return 0;
    }
    
  }
  for(i = 0; i < MAXNOD; i++) {
    BAMCLEAR(Servermem->inne[i].motmed, newConf->nummer);
    if(setPermission) {
      BAMSET(Servermem->inne[i].motratt, newConf->nummer);
    } else {
      BAMCLEAR(Servermem->inne[i].motratt, newConf->nummer);
    }
  }
  BAMSET(Servermem->inne[nodnr].motratt, newConf->nummer);
  BAMSET(Servermem->inne[nodnr].motmed, newConf->nummer);
  if(newConf->type == MOTE_FIDO) {
    ReScanFidoConf(newConf, 0);
  }
  return 0;
}

void listmot(char *foo)
{
        struct Mote *listpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        int cnt, pattern = 0, patret;
        char buffer[61], motpattern[101];
        puttekn("\r\n\n",-1);

        if(foo[0])
        {
			strncpy(buffer,foo,50);
			patret = ParsePatternNoCase(buffer, motpattern, 98);

			if(patret != 1)
			{
				puttekn("\r\n\nDu måste ange ett argument som innehåller wildcards!\r\n", -1);
				return;
			}

			pattern = 1;
		}

        for(;listpek->mot_node.mln_Succ;listpek=(struct Mote *)listpek->mot_node.mln_Succ) {
                if(!MaySeeConf(listpek->nummer,inloggad,&Servermem->inne[nodnr])) continue;

                if(pattern && !MatchPatternNoCase(motpattern, listpek->namn)) continue;
                cnt=0;
                if(IsMemberConf(listpek->nummer, inloggad, &Servermem->inne[nodnr])) sprintf(outbuffer,"%3d %s ",countunread(listpek->nummer),listpek->namn);
                else sprintf(outbuffer,"   *%s ",listpek->namn);
                if(puttekn(outbuffer,-1)) return;
                if(listpek->status & SLUTET) if(puttekn(" (Slutet)",-1)) return;
                if(listpek->status & SKRIVSKYDD) if(puttekn(" (Skrivskyddat)",-1)) return;
                if(listpek->status & KOMSKYDD) if(puttekn(" (Kom.skyddat)",-1)) return;
                if(listpek->status & HEMLIGT) if(puttekn(" (Hemligt)",-1)) return;
                if(listpek->status & AUTOMEDLEM) if(puttekn(" (Auto)",-1)) return;
                if(listpek->status & SKRIVSTYRT) if(puttekn(" (Skrivstyrt)",-1)) return;
                if((listpek->status & SUPERHEMLIGT) && Servermem->inne[nodnr].status >= Servermem->cfg.st.medmoten) if(puttekn(" (ARexx-styrt)",-1)) return;
                if(puttekn("\r\n",-1)) return;
        }

        if(Servermem->info.hightext > -1)
	        sprintf(outbuffer,"\r\n\nGlobala texter: Lägsta textnummer: %d   Högsta textnummer: %d\r\n\n",Servermem->info.lowtext, Servermem->info.hightext);
		else
			strcpy(outbuffer, "\r\n\nDet finns inga texter.\r\n\n");
        puttekn(outbuffer,-1);
}

struct LangCommand *chooseLangCommand(struct Kommando *cmd) {
  return cmd->langCmd[Servermem->inne[nodnr].language].name[0]
    ? &cmd->langCmd[Servermem->inne[nodnr].language] : &cmd->langCmd[0];
}

int parse(char *str) {
  int argType, timeSinceFirstLogin;
  char *arg2 = NULL, *word2;
  struct Kommando *cmd, *foundCmd = NULL;
  struct LangCommand *langCmd;

  timeSinceFirstLogin = time(NULL) - Servermem->inne[nodnr].forst_in;
  if(str[0] == 0) {
    return -3;
  }
  if(str[0] >= '0' && str[0] <= '9') {
    argument = str;
    return 212;
  }

  arg2 = FindNextWord(str);
  if(IzDigit(arg2[0])) {
    argType = KOMARGNUM;
  } else if(!arg2[0]) {
    argType = KOMARGINGET;
  } else {
    argType = KOMARGCHAR;
  }

  ITER_EL(cmd, Servermem->kom_list, kom_node, struct Kommando *) {
    if(cmd->secret) {
      if(cmd->status > Servermem->inne[nodnr].status) continue;
      if(cmd->minlogg > Servermem->inne[nodnr].loggin) continue;
      if(cmd->mindays * 86400 > timeSinceFirstLogin) continue;
      if(cmd->grupper && !(cmd->grupper & Servermem->inne[nodnr].grupper)) continue;
    }
    langCmd = chooseLangCommand(cmd);
    if(langCmd->name[0] && matchar(str, langCmd->name)) {
      word2 = FindNextWord(langCmd->name);
      if((langCmd->words == 2 && matchar(arg2, word2) && arg2[0]) || langCmd->words == 1) {
        if(langCmd->words == 1) {
          if(cmd->argument == KOMARGNUM && argType == KOMARGCHAR) continue;
          if(cmd->argument == KOMARGINGET && argType != KOMARGINGET) continue;
        }
        if(foundCmd == NULL) {
          foundCmd = cmd;
        }
        else if(foundCmd == (struct Kommando *)1L) {
          SendString("%s\n\r", chooseLangCommand(cmd)->name);
        } else {
          SendString("\r\n\n%s\r\n\n", CATSTR(MSG_KOM_AMBIGOUS_COMMAND));
          SendString("%s\n\r", chooseLangCommand(foundCmd)->name);
          SendString("%s\n\r", chooseLangCommand(cmd)->name);
          foundCmd = (struct Kommando *)1L;
        }
      }
    }
  }
  if(foundCmd != NULL && foundCmd != (struct Kommando *)1L) {
    argument = FindNextWord(str);
    if(chooseLangCommand(foundCmd)->words == 2) {
      argument = FindNextWord(argument);
    }
    memset(argbuf, 0, 1080);
    strncpy(argbuf, argument, 1080);
    argbuf[strlen(argument)] = 0;
    argument = argbuf;
  }
  if(foundCmd == NULL) {
    return -1;
  }
  else if(foundCmd == (struct Kommando *)1L) {
    return -2;
  } else {
    if(foundCmd->status > Servermem->inne[nodnr].status || foundCmd->minlogg > Servermem->inne[nodnr].loggin) {
      return -4;
    }
    if(foundCmd->mindays * 86400 > timeSinceFirstLogin) {
      return -4;
    }
    if(foundCmd->grupper && !(foundCmd->grupper & Servermem->inne[nodnr].grupper)) {
      return -4;
    }
  }
  if(foundCmd->losen[0]) {
    SendString("\r\n\n%s: ", CATSTR(MSG_KOM_COMMAND_PASSWORD));
    if(Servermem->inne[nodnr].flaggor & STAREKOFLAG) {
      getstring(STAREKO,20,NULL);
    } else {
      getstring(EJEKO,20,NULL);
    }
    if(strcmp(foundCmd->losen, inmat)) {
      return -5;
    }
  }
  return foundCmd->nummer;
}

int parsemot(char *skri) {
        struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        int going2=TRUE,found=-1;
        char *faci,*skri2;
        if(skri[0]==0 || skri[0]==' ') return(-3);
        for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
                if(!MaySeeConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) continue;
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
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  struct Mote *conf;
  BPTR lock;
  int confId;
  char filename[40];

  if(args[0] == '-' && (args[1] == 'a' || args[1] == 'A')) {
    ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
      if(MayBeMemberConf(conf->nummer, inloggad, &Servermem->inne[nodnr])
         && !IsMemberConf(conf->nummer, inloggad, &Servermem->inne[nodnr])) {
        BAMSET(Servermem->inne[nodnr].motmed, conf->nummer);
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
    SendString("\r\n\nSkriv: Medlem <mötesnamn>\r\neller: Medlem -a för att bli med i alla möten.\r\n\n");
    return;
  }
  if(confId == -1) {
    SendString("\r\n\nFinns inget sådant möte!\r\n\n");
    return;
  }
  if(!MayBeMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu har inte rätt att vara med i det mötet!\r\n\n");
    return;
  }
  BAMSET(Servermem->inne[nodnr].motmed, confId);
  SendString("\r\n\nDu är nu med i mötet %s.\r\n\n", getmotnamn(confId));
  conf = getmotpek(confId);
  if(conf->type == MOTE_ORGINAL) {
    unreadTexts->lowestPossibleUnreadText[confId] = 0;
  }
  else if(conf->type == MOTE_FIDO) {
    unreadTexts->lowestPossibleUnreadText[confId] = conf->lowtext;
  }
  sprintf(filename,"NiKom:Lappar/%d.motlapp", confId);
  if(lock = Lock(filename,ACCESS_READ)) {
    UnLock(lock);
    sendfile(filename);
  }
}

void uttrad(char *args) {
  struct Mote *conf;
  int confId;
  
  if(args[0] == '-' && (args[1] == 'a' || args[1] == 'A')) {
    ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
      if(IsMemberConf(conf->nummer, inloggad, &Servermem->inne[nodnr]))
        BAMCLEAR(Servermem->inne[nodnr].motmed, conf->nummer);
    }
    return;
  }

  confId = parsemot(args);
  if(confId == -3) {
    SendString("\r\n\nSkriv: Utträd <mötesnamn>\r\n\n");
    return;
  }
  if(confId == -1) {
    SendString("\r\n\nFinns inget sådant möte!\r\n\n");
    return;
  }
  conf = getmotpek(confId);
  if(conf->status & AUTOMEDLEM) {
    SendString("\n\n\rDu kan inte utträda ur det mötet!\n\r");
    return;
  }
  if(!IsMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu är inte medlem i det mötet!\r\n\n");
    return;
  }
  if(confId == mote2) {
    mote2 = -1;
  }
  BAMCLEAR(Servermem->inne[nodnr].motmed, confId);
  SendString("\r\n\nDu har nu utträtt ur mötet %s.\r\n\n", getmotnamn(confId));
}

int countunread(int conf) {
  struct Mote *motpek;
  if(conf == -1) {
    return(countmail(inloggad,Servermem->inne[nodnr].brevpek));
  }
  motpek=getmotpek(conf);
  if(motpek->type==MOTE_ORGINAL) {
    return CountUnreadTexts(conf, &Servermem->unreadTexts[nodnr]);
  }
  if(motpek->type==MOTE_FIDO) {
    return countfidomote(motpek);
  }
  return 0;
}

void trimLowestPossibleUnreadTextsForFido(void) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
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
  dellostsay();
  NewList((struct List *)&aliaslist);
  trimLowestPossibleUnreadTextsForFido();
  time(&logintime);
  extratime=0;
  memset(&Statstr,0,sizeof(struct Inloggning));
  Statstr.anv=inloggad;
  mote2=-1;
  LoadCatalogForUser();
  Servermem->action[nodnr]=GORNGTANNAT;
  strcpy(vilkabuf,"loggar in");
  Servermem->vilkastr[nodnr]=vilkabuf;
  senast_text_typ=0;
  if(Servermem->cfg.logmask & LOG_INLOGG) {
    LogEvent(USAGE_LOG, INFO, "%s loggar in på nod %d",
             getusername(inloggad), nodnr);
  }
  sprintf(tellstr,"loggade just in på nod %d",nodnr);
  tellallnodes(tellstr);
  area2=Servermem->inne[nodnr].defarea;
  if(area2<0 || area2>Servermem->info.areor || !Servermem->areor[area2].namn || !arearatt(area2, inloggad, &Servermem->inne[nodnr])) area2=-1;
  initgrupp();
  rxlinecount = TRUE;
  radcnt=0;
  if(Servermem->cfg.ar.postinlogg) sendautorexx(Servermem->cfg.ar.postinlogg);
  DisplayVersionInfo();
  var(mote2);
  KomLoop();
}
