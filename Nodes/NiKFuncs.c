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
char *month[] = { "januari","februari","mars","april","maj","juni",
                "juli","augusti","september","oktober","november","december" },
                *veckodag[] = { "S�ndag","M�ndag","Tisdag","Onsdag","Torsdag","Fredag",
                "L�rdag" },*argument,usernamebuf[50],argbuf[1081],vilkabuf[50];
struct Header sparhead,readhead;
struct Inloggning Statstr;
struct MinList aliaslist;

void atersekom(void) {
        struct Mote *motpek;
        if(senast_text_typ==BREV) {
                if(!brevread.reply[0]) puttekn("\r\n\nTexten �r inte n�gon kommentar!\r\n\n",-1);
                else visabrev(atoi(hittaefter(brevread.reply)),atoi(brevread.reply));
        }
        else if(senast_text_typ==TEXT) {
                motpek=getmotpek(senast_text_mote);
                if(motpek->type==MOTE_ORGINAL) {
                        if(readhead.kom_till_nr==-1) puttekn("\r\n\nTexten �r inte n�gon kommentar!\r\n\n",-1);
                        else if(readhead.kom_till_nr<Servermem->info.lowtext) puttekn("\r\n\nTexten finns inte!\r\n\n",-1);
                        else org_visatext(readhead.kom_till_nr, FALSE);
                }
                else if(motpek->type==MOTE_FIDO) puttekn("\n\n\r�terse Kommenterade kan inte anv�ndas p� Fido-texter\n\r",-1);
        }
        else puttekn("\r\n\nDu har inte l�st n�gon text �nnu!\r\n\n",-1);
}

void igen(void) {
        struct Mote *motpek;
        if(senast_text_typ==BREV) visabrev(senast_brev_nr,senast_brev_anv);
        else if(senast_text_typ==TEXT) {
                motpek=getmotpek(senast_text_mote);
                if(motpek->type==MOTE_ORGINAL) org_visatext(senast_text_nr, FALSE);
                else if(motpek->type==MOTE_FIDO) fido_visatext(senast_text_nr,motpek);
        }
        else puttekn("\r\n\nDu har inte l�st n�gon text �nnu!\r\n\n",-1);
}

int skriv(void) {
        struct Mote *motpek;
        if(mote2==-1) {
                sprintf(outbuffer,"\r\n\nAnv�nd kommandot 'Brev' i %s.\r\n",Servermem->cfg.brevnamn);
                puttekn(outbuffer,-1);
                return(0);
        }
        motpek=getmotpek(mote2);
        if(!motpek) {
                puttekn("\n\n\rHmm.. M�tet du befinner dig i finns inte.\n\r",-1);
                return(0);
        }
        if(!MayWriteConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
                puttekn("\r\n\nDu f�r inte skriva i det h�r m�tet!\r\n\n",-1);
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
  SendString("\r\n\nDu befinner dig i %s.\r\n", conf->namn);
  if(conf->type == MOTE_FIDO) {
    if(conf->lowtext > conf->texter) {
      SendString("Det finns inga texter.\n\r");
    } else {
      SendString("Det finns texter numrerade fr�n %d till %d.\n\r",
                 conf->lowtext, conf->texter);
    }
  }
  cnt = countunread(mote);
  if(cnt == 0) {
    SendString("Du har inga ol�sta texter\r\n");
  } else if(cnt == 1) {
    SendString("Du har 1 ol�st text\r\n");
  } else {
    SendString("Du har %d ol�sta texter\r\n", cnt);
  }
}

void tiden(void)
{
        long tid,tidgrans;
        struct tm *ts;
        time(&tid);
        ts=localtime(&tid);
        sprintf(outbuffer,"\r\n\nTiden just nu: %s %d %s %d  %d:%02d\r\n",
                veckodag[ts->tm_wday],ts->tm_mday,month[ts->tm_mon],1900+ts->tm_year,
                ts->tm_hour,ts->tm_min);
        puttekn(outbuffer,-1);
        if(Servermem->cfg.maxtid[Servermem->inne[nodnr].status]) {
                tidgrans=60*(Servermem->cfg.maxtid[Servermem->inne[nodnr].status])+extratime;
                sprintf(outbuffer,"\nDin tid tar slut om %d minuter.\r\n",(tidgrans-(tid-logintime))/60);
                puttekn(outbuffer,-1);
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
    SendString("\r\n\nNamn p� m�tet: ");
    if(GetString(40,NULL)) {
      return 1;
    }
    strcpy(tmpConf.namn, inmat);
  } else {
    strcpy(tmpConf.namn, argument);
  }
  if(parsemot(tmpConf.namn) != -1) {
    SendString("\r\n\nFinns redan ett s�dant m�te!\r\n");
    return 0;
  }
  tmpConf.skapat_tid = time(NULL);;
  tmpConf.skapat_av = inloggad;
  for(;;) {
    SendString("\r\nM�tesAdministrat�r (MAD) : ");
    if(GetString(5,NULL)) {
      return 1;
    }
    if(inmat[0]) {
      if((mad = parsenamn(inmat)) == -1) {
        SendString("\r\nFinns ingen s�dan anv�ndare!");
      } else {
        tmpConf.mad = mad;
        break;
      }
    }
  }
  SendString("\n\rSorteringsv�rde: ");
  tmpConf.sortpri = GetNumber(0, LONG_MAX, NULL);

  if(EditBitFlagShort("\r\nSka m�tet vara slutet?", 'j', 'n', "Slutet", "�ppet",
                     &tmpConf.status, SLUTET)) {
    return 1;
  }
  if(tmpConf.status & SLUTET) {
    SendString("\r\nVilka grupper ska ha tillg�ng till m�tet? (? f�r lista)\r\n");
    if(editgrupp((char *)&tmpConf.grupper)) {
      return 1;
    }
  }
  if(EditBitFlagShort("\r\nSka m�tet vara skrivskyddat?", 'j', 'n',
                     "Skyddat", "Oskyddat", &tmpConf.status, SKRIVSKYDD)) {
    return 1;
  }
  if(EditBitFlagShort("\r\nSka m�tet vara kommentarsskyddat?", 'j', 'n',
                     "Skyddat", "Oskyddat", &tmpConf.status, KOMSKYDD)) {
    return 1;
  }
  if(EditBitFlagShort("\r\nSka m�tet vara hemligt?", 'j', 'n',
                     "Hemligt", "Ej hemligt", &tmpConf.status, HEMLIGT)) {
    return 1;
  }
  if(!(tmpConf.status & SLUTET)) {
    if(EditBitFlagShort("\r\nSka alla anv�ndare bli medlemmar automagiskt?", 'j', 'n',
                       "Ja", "Nej", &tmpConf.status, AUTOMEDLEM)) {
      return 1;
    }
    if(EditBitFlagShort("\r\nSka r�ttigheterna styra skrivm�jlighet?", 'j', 'n',
                       "Ja", "Nej", &tmpConf.status, SKRIVSTYRT)) {
      return 1;
    }
    if(tmpConf.status & SKRIVSTYRT) {
      SendString("\r\nVilka grupper ska ha tillg�ng till m�tet? (? f�r lista)\r\n");
      if(editgrupp((char *)&tmpConf.grupper)) {
        return 1;
      }
    }
  }
  if(EditBitFlagShort("\r\nSka m�tet enbart kommas �t fr�n ARexx?", 'j', 'n',
                     "Ja", "Nej", &tmpConf.status, SUPERHEMLIGT)) {
    return 1;
  }

  SendString("\n\n\rVilken typ av m�te ska det vara?\n\r");
  SendString("1: Lokalt m�te\n\r");
  SendString("2: Fido-m�te\n\n\r");
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
    SendString("Lokalt m�te\n\n\r");
    tmpConf.type = MOTE_ORGINAL;
  } else {
    SendString("Fido-m�te\n\n\r");
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
    
    SendString("\n\n\rVilken teckenupps�ttning ska anv�ndas f�r utg�ende texter?\n\r");
    SendString("1: ISO Latin 1 (ISO 8859-1)\n\r");
    SendString("2: SIS-7 (SF7, 'M�svingar')\n\r");
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
    SendString("Vilken dom�n �r m�tet i?\n\r");
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
      SendString("\n\rDu m�ste definiera en dom�n i NiKomFido.cfg f�rst!\n\r");
      return 0;
    }
    for(;;) {
      SendString("\r\nDom�n: ");
      if(GetString(5, NULL)) {
        return 1;
      }
      fidoDomainId = atoi(inmat);
      if(fidoDomain = getfidodomain(fidoDomainId, 0)) {
        break;
      } else {
        SendString("\n\rFinns ingen s�dan dom�n.\n\r");
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
    SendString("\n\n\rDet finns inte plats f�r fler m�ten.\n\r");
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

  SendString("\r\n�ndrar i anv�ndardata..\r\n");
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
				puttekn("\r\n\nDu m�ste ange ett argument som inneh�ller wildcards!\r\n", -1);
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
	        sprintf(outbuffer,"\r\n\nGlobala texter: L�gsta textnummer: %d   H�gsta textnummer: %d\r\n\n",Servermem->info.lowtext, Servermem->info.hightext);
		else
			strcpy(outbuffer, "\r\n\nDet finns inga texter.\r\n\n");
        puttekn(outbuffer,-1);
}

int parse(char *skri) {
        int nummer=0,argtyp, inloggtid;
        char *arg2 = NULL,*ord2;
        struct Kommando *kompek,*forst=NULL;

		inloggtid = time(NULL)-Servermem->inne[nodnr].forst_in;
        if(skri[0]==0) return(-3);
        if(skri[0]>='0' && skri[0]<='9') {
                argument=skri;
                return(212);
        }

		arg2=hittaefter(skri);
        if(IzDigit(arg2[0])) argtyp=KOMARGNUM;
        else if(!arg2[0]) argtyp=KOMARGINGET;
        else argtyp=KOMARGCHAR;

        for(kompek=(struct Kommando *)Servermem->kom_list.mlh_Head;kompek->kom_node.mln_Succ;kompek=(struct Kommando *)kompek->kom_node.mln_Succ)
        {
                if(kompek->secret)
                {
                        if(kompek->status > Servermem->inne[nodnr].status) continue;
                        if(kompek->minlogg > Servermem->inne[nodnr].loggin) continue;
                        if(kompek->mindays*86400 > inloggtid) continue;
                        if(kompek->grupper && !(kompek->grupper & Servermem->inne[nodnr].grupper)) continue;
                }
                if(matchar(skri,kompek->namn))
                {
                        ord2=hittaefter(kompek->namn);
                        if((kompek->antal_ord==2 && matchar(arg2,ord2) && arg2[0]) || kompek->antal_ord==1)
                        {
                                if(kompek->antal_ord==1)
                                {
                                        if(kompek->argument==KOMARGNUM && argtyp==KOMARGCHAR) continue;
                                        if(kompek->argument==KOMARGINGET && argtyp!=KOMARGINGET) continue;
                                }
                                if(forst==NULL)
                                {
                                        forst=kompek;
                                        nummer=kompek->nummer;
                                }
                                else if(forst==(struct Kommando *)1L)
                                {
                                        puttekn(kompek->namn,-1);
                                        puttekn("\n\r",-1);
                                }
                                else
                                {
                                        puttekn("\r\n\nFLERTYDIGT KOMMANDO\r\n\n",-1);
                                        puttekn(forst->namn,-1);
                                        puttekn("\n\r",-1);
                                        puttekn(kompek->namn,-1);
                                        puttekn("\n\r",-1);
                                        forst=(struct Kommando *)1L;
                                }
                        }
                }
        }
        if(forst!=NULL && forst!=(struct Kommando *)1L)
        {
			argument=hittaefter(skri);
			if(forst->antal_ord==2) argument=hittaefter(argument);
            memset(argbuf,0,1080);
            strncpy(argbuf,argument,1080);
            argbuf[strlen(argument)]=0;
            argument=argbuf;
        }
        if(forst==NULL) return(-1);
        else if(forst==(struct Kommando *)1L) return(-2);
        else
        {
			if(forst->status > Servermem->inne[nodnr].status || forst->minlogg > Servermem->inne[nodnr].loggin) return(-4);
			if(forst->mindays*86400 > inloggtid) return(-4);
			if(forst->grupper && !(forst->grupper & Servermem->inne[nodnr].grupper)) return(-4);
        }
        if(forst->losen[0])
        {
                puttekn("\r\n\nL�sen: ",-1);
                if(Servermem->inne[nodnr].flaggor & STAREKOFLAG)
	                getstring(STAREKO,20,NULL);
				else
					getstring(EJEKO,20,NULL);
                if(strcmp(forst->losen,inmat)) return(-5);
        }
        return(nummer);
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
    SendString("\r\n\nSkriv: Medlem <m�tesnamn>\r\neller: Medlem -a f�r att bli med i alla m�ten.\r\n\n");
    return;
  }
  if(confId == -1) {
    SendString("\r\n\nFinns inget s�dant m�te!\r\n\n");
    return;
  }
  if(!MayBeMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu har inte r�tt att vara med i det m�tet!\r\n\n");
    return;
  }
  BAMSET(Servermem->inne[nodnr].motmed, confId);
  SendString("\r\n\nDu �r nu med i m�tet %s.\r\n\n", getmotnamn(confId));
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
    SendString("\r\n\nSkriv: Uttr�d <m�tesnamn>\r\n\n");
    return;
  }
  if(confId == -1) {
    SendString("\r\n\nFinns inget s�dant m�te!\r\n\n");
    return;
  }
  conf = getmotpek(confId);
  if(conf->status & AUTOMEDLEM) {
    SendString("\n\n\rDu kan inte uttr�da ur det m�tet!\n\r");
    return;
  }
  if(!IsMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu �r inte medlem i det m�tet!\r\n\n");
    return;
  }
  if(confId == mote2) {
    mote2 = -1;
  }
  BAMCLEAR(Servermem->inne[nodnr].motmed, confId);
  SendString("\r\n\nDu har nu uttr�tt ur m�tet %s.\r\n\n", getmotnamn(confId));
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
  Servermem->action[nodnr]=GORNGTANNAT;
  strcpy(vilkabuf,"loggar in");
  Servermem->vilkastr[nodnr]=vilkabuf;
  senast_text_typ=0;
  if(Servermem->cfg.logmask & LOG_INLOGG) {
    LogEvent(USAGE_LOG, INFO, "%s loggar in p� nod %d",
             getusername(inloggad), nodnr);
  }
  sprintf(tellstr,"loggade just in p� nod %d",nodnr);
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
