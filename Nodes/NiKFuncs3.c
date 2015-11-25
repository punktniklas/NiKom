#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <error.h>
#include <math.h>
#include <limits.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Terminal.h"
#include "Logging.h"

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

int endast(void) {
	int antal,foo=TRUE;
	int motnr, nyttmote = 1;
	struct Mote *motpek = NULL;
	if(!isdigit(argument[0])) {
		puttekn("\r\n\nSkriv: Endast <antal texter> [mötesnamn]\r\n",-1);
		return(-5);
	}
	antal=atoi(argument);
	argument=hittaefter(argument);
	if(argument[0])
	{
		if(matchar(argument,Servermem->cfg.brevnamn))
			motnr = -1;
		else
		{
			motnr=parsemot(argument);
			if(motnr==-3)
			{
				puttekn("\r\n\nSkriv: Endast <antal texter> [mötesnamn]\r\n\n",-1);
				return(-5);
			}
			if(motnr==-1)
			{
				puttekn("\r\n\nFinns inget sådant möte!\r\n\n", -1);
				return(-5);
			}
		}
		nyttmote = 0;
	}
	else
		motnr = mote2;

	if(motnr==-1) {
		Servermem->inne[nodnr].brevpek=getnextletter(inloggad)-antal;
		foo=getfirstletter(inloggad);
		if(foo>Servermem->inne[nodnr].brevpek) Servermem->inne[nodnr].brevpek=foo;
	} else {
		motpek = getmotpek(motnr);
		if(motpek->type==MOTE_ORGINAL) org_endast(motnr,antal);
		else if(motpek->type==MOTE_FIDO) fido_endast(motpek,antal);
	}
	if(nyttmote)
		return(-11);
	else
		return(-5);
}

int personlig(void) {
  int nummer, editret, confId;
  struct Mote *motpek;
  if(argument[0]) {
    if(mote2==-1) {
      puttekn("\r\n\nBrev kommenterar du som vanligt med 'kommentera'.\r\n\n",-1);
      return(0);
    }
    nummer=atoi(argument);
    motpek=getmotpek(mote2);
    if(motpek->type == MOTE_ORGINAL) {
      if(nummer<Servermem->info.lowtext || nummer>Servermem->info.hightext) {
        puttekn("\r\n\nTexten finns inte!\r\n\n",-1);
        return(0);
      }
      confId = GetConferenceForText(nummer);
      if(!MayBeMemberConf(confId, inloggad, &Servermem->inne[nodnr])) {
        puttekn("\r\n\nDu har inte rätt att kommentera den texten!\r\n\n",-1);
        return(0);
      }
      if(readtexthead(nummer,&readhead)) return(0);
      senast_text_mote = confId;
    } else if(motpek->type == MOTE_FIDO) {
      if(nummer<motpek->lowtext || nummer>motpek->texter) {
        puttekn("\r\n\nTexten finns inte!\r\n\n",-1);
        return(0);
      }
      senast_text_mote = motpek->nummer;
    }
    senast_text_typ=TEXT;
    senast_text_nr = nummer;
  } else {
    if(senast_text_typ==BREV) {
      puttekn("\r\n\nBrev kommenterar du som vanligt med 'kommentera'.\r\n\n",-1);
      return(0);
    }
    if(!(motpek=getmotpek(senast_text_mote))) {
      puttekn("\n\n\rHmm.. Mötet där texten finns existerar inte..\n\r",-1);
      return(0);
    }
  }
  if(motpek->type == MOTE_FIDO) {
    nu_skrivs = BREV_FIDO;
    return(fido_brev(NULL,NULL,motpek));
  }
  nu_skrivs=BREV;
  initpersheader();
  if((editret=edittext(NULL))==1) return(1);
  if(!editret) sparabrev();
  return(0);
}

void radtext(void) {
	int nummer;
	struct Header radhead;
	struct Mote *motpek;
	nummer = atoi(argument);
	motpek=getmotpek(GetConferenceForText(nummer));
	if(motpek->type != MOTE_ORGINAL) {
		puttekn("\n\n\rDu kan bara radera texter i interna möten.\n\n\r",-1);
		return;
	}
	if(nummer < Servermem->info.lowtext || nummer > Servermem->info.hightext) {
		puttekn("\r\n\nTexten finns inte!\r\n\n",-1);
		return;
	}
	if(readtexthead(nummer,&radhead)) return;
	if(radhead.person!=inloggad && !MayAdminConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
		puttekn("\r\n\nDu kan bara radera texter av dig själv!\r\n\n",-1);
		return;
	}
        SetConferenceForText(nummer, -1, TRUE);
}

int radmot(void) {
  int confId, textNumber, isCorrect;
  char lappfile[40];
  struct Mote *conf;
  if((confId = parsemot(argument)) == -1) {
    SendString("\r\n\nFinns inget möte som heter så!\r\n\n");
    return -5;
  }
  if(confId == -3) {
    SendString("\r\n\nSkriv: Radera Möte <mötesnamn>\r\n\n");
    return -5;
  }
  conf = getmotpek(confId);
  if(conf == NULL) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't find conference %d in memory", confId);
    DisplayInternalError();
    return -5;
  }
  if(!MayAdminConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\n\n\rDu har ingen rätt att radera det mötet!\n\r");
    return -5;
  }

  SendString("\r\n\nÄr du säker på att du vill radera mötet %s? ", conf->namn);
  if(GetYesOrNo(NULL, 'j', 'n', "Ja\r\n", "Nej\r\n", FALSE, &isCorrect)) {
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
    SendString("\r\n\nFel vid skrivandet av Textmot.dat\r\n\n");
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
    SendString("\r\n\nFinns inget möte som heter så!\r\n\n");
    return 0;
  } else if(confId == -3) {
    SendString("\r\n\nSkriv: Ändra Möte <mötesnamn>\r\n\n");
    return 0;
  }
  conf = getmotpek(confId);
  if(!MayAdminConf(confId, inloggad, &Servermem->inne[nodnr])) {
    SendString("\r\n\nDu har ingen rätt att ändra på det mötet!\r\n\n");
    return 0;
  }
  memcpy(&tmpConf, conf, sizeof(struct Mote));
  for(;;) {
    SendString("\r\nNamn : (%s) ", tmpConf.namn);
    if(GetString(40, NULL)) {
      return 1;
    }
    if(inmat[0] == '\0') {
      break;
    }
    if((tmpConfId = parsemot(inmat)) != -1 && tmpConfId != confId) {
      SendString("\r\n\nMötet finns redan!\r\n");
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
      SendString("\r\nFinns ingen sådan användare!");
    } else {
      tmpConf.mad=mad;
    }
  }
  if(MaybeEditNumber("Sorteringsvärde", (int *)&tmpConf.sortpri, 8, 0, LONG_MAX)) {
    return 1;
  }
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
  } else {
    tmpConf.status &= ~(AUTOMEDLEM | SKRIVSTYRT);
  }
  if(EditBitFlagShort("\r\nSka mötet bara vara åtkomligt från ARexx?", 'j', 'n',
                 "Ja", "Nej", &tmpConf.status, SUPERHEMLIGT)) {
    return 1;
  }

  if(tmpConf.type == MOTE_FIDO) {
    if(MaybeEditString("Katalog", tmpConf.dir, 79)) { return 1; }
    if(MaybeEditString("Tag-namn", tmpConf.tagnamn, 49)) { return 1; }
    if(MaybeEditString("Origin", tmpConf.origin, 69)) { return 1; }
    
    SendString("\n\n\r%c1: ISO Latin 1 (ISO 8859-1)\n\r",
               tmpConf.charset == CHRS_LATIN1 ? '*' : ' ');
    SendString("%c2: SIS-7 (SF7, 'Måsvingar')\n\r",
               tmpConf.charset == CHRS_SIS7 ? '*' : ' ');
    SendString("%c3: IBM CodePage\n\r", tmpConf.charset == CHRS_CP437 ? '*' : ' ');
    SendString("%c4: Mac\n\n\r", tmpConf.charset == CHRS_MAC ? '*' : ' ');
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
      if(!Servermem->fidodata.fd[i].domain[0]) {
        break;
      }
      SendString("%c%3d: %s (%d:%d/%d.%d)\n\r",
                 tmpConf.domain == Servermem->fidodata.fd[i].nummer ? '*' : ' ',
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
      if(GetString(10,NULL)) {
        return(1);
      }
      if(inmat[0] == '\0') {
        break;
      }
      if(domain = getfidodomain(atoi(inmat), 0)) {
        tmpConf.domain = domain->nummer;
        break;
      } else {
        SendString("\n\rFinns ingen sådan domän.\n\r");
      }
    }
  }
  
  if(GetYesOrNo("\r\n\nÄr allt korrekt?", 'j', 'n', "Ja\r\n\n", "Nej\r\n\n",
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
    SendString("Users.dat kommer nu att gås igenom för att nollställa medlemskap\n\r");
    SendString("och rättigheter som om mötet var nyskapat.\n\n\r");
    SendString("Är ändringarna fortfarande korrekta? ");
    if(GetYesOrNo("Är ändringarna fortfarande korrekta?", 'j', 'n',
                  "Ja\r\n", "Nej\r\n", TRUE, &isCorrect)) {
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
  SendString("\r\nÄndrar i Users.dat..\r\n");
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
	puttekn("\n\n\rRadera brev är tillfälligt ur funktion!\n\r",-1);
}

void vilka(void) {
	int x,verbose=FALSE;
	long timenow, idle;
	struct Mote *motpek;
	char namn[50],bps[15];
	if(argument[0]) {
		if(argument[0] == '-' && (argument[1] == 'v' || argument[1] == 'V')) verbose = TRUE;
	}
	puttekn("\r\n\n",-1);
	timenow = time(NULL);
	for(x=0;x<MAXNOD;x++) {
		if(!Servermem->nodtyp[x]) continue;
		if(Servermem->inloggad[x] == -1) strcpy(namn,"<Ingen inloggad>");
		else if(Servermem->inloggad[x] == -2) strcpy(namn,"<Uppringd>");
		else strcpy(namn,getusername(Servermem->inloggad[x]));
		if(Servermem->nodtyp[x]==NODCON) sprintf(outbuffer,"Nod #%-2d CON %-20s %s\n\r",x,Servermem->nodid[x],namn);
		else if(Servermem->nodtyp[x]==NODSER) sprintf(outbuffer,"Nod #%-2d SER %-20s %s\n\r",x,Servermem->nodid[x],namn);
		else sprintf(outbuffer,"Nod #%-2d FOO %-20s %s\n\r",x,Servermem->nodid[x],namn);
		puttekn(outbuffer,-1);
		if(!verbose) continue;
		idle = timenow - Servermem->idletime[x];
		if(Servermem->inloggad[x] == -1 || Servermem->inloggad[x] == -2)
		{
/*			eka('\n');
			continue; */

			if(Servermem->inloggad[x] == -2)
				sprintf(bps,"Bps: %-7d%15d:%02d %c\r\n\n",Servermem->connectbps[x],idle/3600,(idle%3600)/60,Servermem->say[x] ? '*' : ' ');
			else
				sprintf(bps,"Bps: -      %15d:%02d %c\r\n\n",idle/3600,(idle%3600)/60,Servermem->say[x] ? '*' : ' ');

			puttekn(bps,-1);
			continue;
		}
		if(Servermem->nodtyp[x]==NODCON)
			sprintf(bps,"Bps: -      %15d:%02d %c",idle/3600,(idle%3600)/60,Servermem->say[x] ? '*' : ' ');
		else sprintf(bps,"Bps: %-7d%15d:%02d %c",Servermem->connectbps[x],idle/3600,(idle%3600)/60,Servermem->say[x] ? '*' : ' ');
		switch(Servermem->action[x]) {
			case INGET :
				sprintf(outbuffer,"%-32s Har inga olästa texter\r\n\n",bps);
				break;
			case SKRIVER :
				if(Servermem->varmote[x]!=-1) {
					motpek=getmotpek(Servermem->varmote[x]);
					if(!motpek) continue;
					if(!MaySeeConf(motpek->nummer, inloggad, &Servermem->inne[nodnr]))
						sprintf(outbuffer,"%-32s Skriver en text\r\n\n",bps);
					else sprintf(outbuffer,"%-32s Skriver i %s\r\n\n",bps,motpek->namn);
				} else sprintf(outbuffer,"%-32s Skriver ett brev\r\n\n",bps);
				break;
			case LASER :
				if(Servermem->varmote[x]!=-1) {
					motpek=getmotpek(Servermem->varmote[x]);
					if(!motpek) continue;
					if(!MaySeeConf(motpek->nummer, inloggad, &Servermem->inne[nodnr]))
						sprintf(outbuffer,"%-32s Läser texter\r\n",bps);
					else sprintf(outbuffer,"%-32s Läser i %s\r\n",bps,motpek->namn);
				} else sprintf(outbuffer,"%-32s Läser i %s\r\n",bps,Servermem->cfg.brevnamn);
				break;
			case GORNGTANNAT :
				sprintf(outbuffer,"%-32s %s\r\n",bps,Servermem->vilkastr[x]);
				break;
			case UPLOAD :
				if(!Servermem->areor[Servermem->varmote[x]].namn[0] || !arearatt(Servermem->varmote[x], inloggad, &Servermem->inne[nodnr]))
					sprintf(outbuffer,"%-32s Laddar upp\r\n",bps);
				else {
					if(Servermem->vilkastr[x]) sprintf(outbuffer,"%-32s Laddar upp %s\r\n",bps,Servermem->vilkastr[x]);
					else sprintf(outbuffer,"%-32s Ska strax ladda upp en fil\r\n",bps);
				}
				break;
			case DOWNLOAD :
				if(!Servermem->areor[Servermem->varmote[x]].namn[0] || !arearatt(Servermem->varmote[x], inloggad, &Servermem->inne[nodnr]))
					sprintf(outbuffer,"%-32s Laddar ner\r\n",bps);
				else {
					if(Servermem->vilkastr[x]) sprintf(outbuffer,"%-32s Laddar ner %s\r\n",bps,Servermem->vilkastr[x]);
					else sprintf(outbuffer,"%-32s Ska strax ladda ner en fil\r\n",bps);
				}
				break;
			default :
				sprintf(outbuffer,"%-32s <Gör något odefinierat>\r\n",bps);
				break;
		}
		if(Servermem->CallerID[x])
		{
			strcat(outbuffer, Servermem->CallerID[x]);
			strcat(outbuffer, "\r\n");
		}

		strcat(outbuffer, "\n");
		puttekn(outbuffer,-1);
	}
}

void visainfo(void) {
   struct AnchorPath *anchor;
   char filnamn[100],pattern[100];
	if(!argument[0]) {
		sendfile("NiKom:Texter/Info.txt");
		return;
	}
	if(anchor = AllocMem(sizeof(struct AnchorPath),MEMF_CLEAR)) {
	   sprintf(pattern,"NiKom:Texter/%s#?.txt",argument);
	   if(MatchFirst(pattern,anchor)) {
	      puttekn("\r\n\nFinns ingen sådan fil!\r\n",-1);
			return;
		}
	   sprintf(filnamn,"NiKom:Texter/%s",anchor->ap_Info.fib_FileName);
	   MatchEnd(anchor);
	   FreeMem(anchor,sizeof(struct AnchorPath));
   	sendfile(filnamn);
   } else puttekn("\n\n\rKunde inte allokera en Anchorpath\n\r",-1);
}

int readtexthead(int nummer,struct Header *head) {
	BPTR fp;
	int fil,pos;
	char filnamn[40];
	fil=nummer/512;
	pos=nummer%512;
	sprintf(filnamn,"NiKom:Moten/Head%d.dat",fil);
	NiKForbid();
	if(!(fp=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Head.dat!\r\n\n",-1);
		NiKPermit();
		return(1);
	}
	if(Seek(fp,pos*sizeof(struct Header),OFFSET_BEGINNING)==-1L) {
		puttekn("\r\n\nKunde inte söka i Head.dat!\r\n\n",-1);
		Close(fp);
		NiKPermit();
		return(1);
	}
	if(!(Read(fp,(void *)head,sizeof(struct Header)))) {
		puttekn("\r\n\nKunde inte läsa Head.dat!\r\n\n",-1);
		Close(fp);
		NiKPermit();
		return(1);
	}
	Close(fp);
	NiKPermit();
	return(0);
}

int writetexthead(int nummer,struct Header *head) {
	BPTR fh;
	int fil,pos;
	char filnamn[40];
	fil=nummer/512;
	pos=nummer%512;
	sprintf(filnamn,"NiKom:Moten/Head%d.dat",fil);
	NiKForbid();
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Head.dat!\r\n\n",-1);
		NiKPermit();
		return(1);
	}
	if(Seek(fh,pos*sizeof(struct Header),OFFSET_BEGINNING)==-1) {
		puttekn("\r\n\nKunde inte söka i Head.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return(1);
	}
	if(Write(fh,(void *)head,sizeof(struct Header))==-1) {
		puttekn("\r\n\nKunde inte skriva Head.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return(1);
	}
	Close(fh);
	NiKPermit();
	return(0);
}

int getnextletter(int user) {
	BPTR fh;
	char nrstr[20],filnamn[50];
	sprintf(filnamn,"NiKom:Users/%d/%d/.nextletter",user/100,user);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\n\n\rKunde inte öppna .nextfile\n\r",-1);
		return(-1);
	}
	memset(nrstr,0,20);
	if(Read(fh,nrstr,19)==-1) {
		puttekn("\n\n\rKunde inte läsa .nextfile\n\r",-1);
		Close(fh);
		return(-1);
	}
	Close(fh);
	return(atoi(nrstr));
}

int getfirstletter(int user) {
	BPTR fh;
	char nrstr[20],filnamn[50];
	sprintf(filnamn,"NiKom:Users/%d/%d/.firstletter",user/100,user);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\n\n\rKunde inte öppna .firstfile\n\r",-1);
		return(-1);
	}
	memset(nrstr,0,20);
	if(Read(fh,nrstr,19)==-1) {
		puttekn("\n\n\rKunde inte läsa .firstfile\n\r",-1);
		Close(fh);
		return(-1);
	}
	Close(fh);
	return(atoi(nrstr));
}

int readuser(int nummer,struct User *user) {
	BPTR fh;
	char filnamn[40];
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",nummer/100,nummer);
	NiKForbid();
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Users.dat!\r\n\n",-1);
		NiKPermit();
		return(1);
	}
	if(Read(fh,(void *)user,sizeof(struct User))==-1) {
		puttekn("\r\n\nKunde inte läsa Users.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return(1);
	}
	Close(fh);
	NiKPermit();
	return(0);
}

int writeuser(int nummer,struct User *user) {
	BPTR fh;
	char filnamn[40];
	sprintf(filnamn,"NiKom:Users/%d/%d/Data",nummer/100,nummer);
	NiKForbid();
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Users.dat!\r\n\n",-1);
		NiKPermit();
		return(1);
	}
	if(Write(fh,(void *)user,sizeof(struct User))==-1) {
		puttekn("\r\n\nKunde inte skriva Users.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return(1);
	}
	Close(fh);
	NiKPermit();
	return(0);
}

void rensatexter(void) {
	int antal=0;
	if(!(antal=atoi(argument))) {
		puttekn("\r\n\nSkriv : Rensa texter <antal texter>\r\n\n",-1);
		return;
	}
	if(antal%512) {
		puttekn("\r\n\nAntalet måste vara en jämn multipel av 512!\r\n\n",-1);
		return;
	}
	sendservermess(RADERATEXTER,antal);
}

void gamlatexter(void) { }

void gamlabrev(void) { }

void getconfig(void) {
	sendservermess(READCFG,0);
}

void writeinfo(void) {
	sendservermess(WRITEINFO,0);
}

void displaysay(void) {
	struct SayString *pekare,*oldpek;
	int textlen;
	char tmpchar;
	pekare=Servermem->say[nodnr];
	Servermem->say[nodnr]=NULL;
	while(pekare) {
		if(pekare->fromuser == -1) strcpy(outbuffer,"\a\r\n\nSystemmeddelande");
		else sprintf(outbuffer,"\a\r\n\n%s",getusername(pekare->fromuser));
		textlen = strlen(pekare->text);
		puttekn(outbuffer,-1);
		if((strlen(outbuffer) + textlen) < 79) {
			sprintf(outbuffer,": %s\r\n",pekare->text);
			puttekn(outbuffer,-1);
		} else {
			puttekn(":\n\r",-1);
			if(textlen <= MAXSAYTKN-1) puttekn(pekare->text,-1);
			else {
				tmpchar = pekare->text[MAXSAYTKN-1];
				pekare->text[MAXSAYTKN-1] = 0;
				puttekn(pekare->text,-1);
				pekare->text[MAXSAYTKN-1] = tmpchar;
				puttekn(&pekare->text[MAXSAYTKN-1],-1);
			}
		}
		oldpek=pekare;
		pekare=oldpek->NextSay;
		FreeMem(oldpek,sizeof(struct SayString));
	}
	eka('\n');
}

int sag(void) {
	int vem,x;
	char *quick;
	struct SayString *pekare,*oldpekare=NULL,*allocpekare;
	quick = strchr(argument,',');
	if(quick) *quick++ = 0;
	if((vem=parsenamn(argument))==-3) {
		puttekn("\r\n\nSkriv : Säg <användare>[,<meddelande>]\r\n\n",-1);
		return(0);
	}
	if(vem==-1) {
		puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n",-1);
		return(0);
	}
	for(x=0;x<MAXNOD;x++) if(Servermem->inloggad[x]==vem) break;
	if(x==MAXNOD) {
		puttekn("\r\n\nPersonen är inte inloggad!\r\n",-1);
		return(0);
	}
	if(!quick) {
		puttekn("\r\n\nVad vill du säga?\r\n",-1);
		if(getstring(EKO,MAXSAYTKN-1,NULL)) return(1);
		if(!inmat[0]) return(0);
		if(Servermem->inloggad[x]==-1) {
			puttekn("\r\n\nTyvärr, personen har loggat ut.\r\n",-1);
			return(0);
		}
	}
	pekare=Servermem->say[x];
	if(pekare) sprintf(outbuffer,"\r\n%s har olästa meddelanden. Skickar meddelandet ändå.\r\n",getusername(vem));
	else sprintf(outbuffer,"\r\n%s meddelad\r\n",getusername(vem));
	puttekn(outbuffer,-1);
	Forbid();
	while(pekare) {
		oldpekare=pekare;
		pekare=oldpekare->NextSay;
	}
	if(!(allocpekare=(struct SayString *)AllocMem(sizeof(struct SayString),MEMF_PUBLIC | MEMF_CLEAR))) {
		Permit();
		puttekn("\r\n\nKunde inte allokera minne till meddelandet!\r\n\n",-1);
		return(0);
	}
	allocpekare->fromuser=inloggad;
	if(quick) strcpy(allocpekare->text,quick);
	else strcpy(allocpekare->text,inmat);
	if(Servermem->say[x]) oldpekare->NextSay=allocpekare;
	else Servermem->say[x]=allocpekare;
	Permit();
	return(0);
}

void writesenaste(void) {
	BPTR fh;
	long tiden;
	time(&tiden);
	Statstr.tid_inloggad=(tiden-logintime)/60;
	Statstr.utloggtid=tiden;
	NiKForbid();
	movmem(&Servermem->senaste,&Servermem->senaste[1],sizeof(struct Inloggning)*(MAXSENASTE-1));
	memcpy(Servermem->senaste,&Statstr,sizeof(struct Inloggning));
	if(!(fh=Open("NiKom:DatoCfg/Senaste.dat",MODE_NEWFILE))) {
		printf("Kunde inte öppna Senaste.dat\n");
		NiKPermit();
		return;
	}
	if(Write(fh,(void *)Servermem->senaste,sizeof(struct Inloggning)*MAXSENASTE)==-1)
		printf("Fel under skrivandet av Senaste.dat\n");
	Close(fh);
	NiKPermit();
}

void listasenaste(void) {
	int x,nummer,alla=TRUE,cnt=0,antal=MAXSENASTE;
	struct tm *ts;
	if(argument[0]=='-') {
		antal=atoi(&argument[1]);
		argument=hittaefter(argument);
	}
	if((nummer=parsenamn(argument))==-1) {
		puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n",-1);
		return;
	} else if(nummer==-3) alla=TRUE;
	else alla=FALSE;
	puttekn("\r\n\n     Namn                            Utloggad     Tid   Läst Skr  Ul  Dl\r\n\n",-1);
	for(x=0;x<MAXSENASTE;x++) {
		if(!Servermem->senaste[x].utloggtid || (!alla && nummer!=Servermem->senaste[x].anv)) continue;
		if(!userexists(Servermem->senaste[x].anv)) continue;
		cnt++;
		ts=localtime(&Servermem->senaste[x].utloggtid);
		sprintf(outbuffer,"%-35s %02d/%02d %02d:%02d %3dmin  %4d %3d  %2d  %2d\r\n",
			getusername(Servermem->senaste[x].anv),ts->tm_mday,ts->tm_mon+1,ts->tm_hour,ts->tm_min,
			Servermem->senaste[x].tid_inloggad,Servermem->senaste[x].read,
			Servermem->senaste[x].write,Servermem->senaste[x].ul,
			Servermem->senaste[x].dl);
		if(puttekn(outbuffer,-1)) return;
		if(cnt>=antal) return;
	}
}

int dumpatext(void) {
	int tnr,x;
	struct Header dumphead;
	FILE *fpr,*fpd;
	char *dumpfil,foostr[82],filnamn[40];
	if(!isdigit(argument[0])) {
		puttekn("\r\n\nSkriv: Dumpa Text <textnr> [filnamn]\r\n\n",-1);
		return(0);
	}
	tnr=atoi(argument);
	if(tnr<Servermem->info.lowtext || tnr>Servermem->info.hightext) {
		puttekn("\r\n\nTexten finns inte!\r\n",-1);
		return(0);
	}
	if(!MayReadConf(GetConferenceForText(tnr), inloggad, &Servermem->inne[nodnr])) {
		puttekn("\r\n\nDu har inte rätt att dumpa den texten!\r\n",-1);
		return(0);
	}
	dumpfil=hittaefter(argument);
	if(!dumpfil[0]) {
		puttekn("\r\n\nTill vilken fil? : ",-1);
		if(getstring(EKO,50,NULL)) return(1);
		if(!inmat[0]) return(0);
		dumpfil=inmat;
	}
	if(readtexthead(tnr,&dumphead)) return(0);
	sprintf(filnamn,"NiKom:Moten/Text%d.dat",dumphead.nummer/512);
	NiKForbid();
	if(!(fpr=fopen(filnamn,"r"))) {
		puttekn("\r\n\nKunde inte öppna Text.dat!\r\n\n",-1);
		NiKPermit();
		return(0);
	}
	if(fseek(fpr,dumphead.textoffset,0)) {
		puttekn("\r\n\nKunde inte söka i Text.dat!\r\n\n",-1);
		fclose(fpr);
		NiKPermit();
		return(0);
	}
	if(!(fpd=fopen(dumpfil,"w"))) {
		sprintf(outbuffer,"\r\n\nKunde inte öppna %s!\r\n",dumpfil);
		puttekn(outbuffer,-1);
		fclose(fpr);
		NiKPermit();
		return(0);
	}
	for(x=0;x<dumphead.rader;x++) {
		if(!(fgets(foostr,81,fpr))) {
			puttekn("\r\n\nFel vid läsandet av Text.dat!\r\n\n",-1);
			break;
		}
		if(fputs(foostr,fpd)) {
			sprintf(outbuffer,"\r\n\nFel vid skrivandet av %s!\r\n",dumpfil);
			puttekn(outbuffer,-1);
			break;
		}
	}
	fclose(fpr);
	fclose(fpd);
	NiKPermit();
	return(0);
}

void listaarende(void) {
  char *nextarg=argument,namn[50],kom[10];
  int forward = FALSE, textNumber = -1, baratexter = FALSE;
  struct Mote *motpek;
  struct Header lahead;
  struct tm *ts;
  if(mote2==-1) {
    sprintf(outbuffer,"\n\n\rAnvänd 'Lista Brev' i %s\n\r",Servermem->cfg.brevnamn);
    puttekn(outbuffer,-1);
    return;
  }
  while(nextarg[0]) {
    if(nextarg[0]=='-') {
      if(nextarg[1]=='t' || nextarg[1]=='T') baratexter=TRUE;
      else if(nextarg[1]=='f' || nextarg[1]=='F') {
        forward = TRUE;
      }
    } else if(isdigit(nextarg[0])) {
      textNumber = atoi(nextarg);
    }
    nextarg=hittaefter(nextarg);
  }
  motpek=getmotpek(mote2);
  if(motpek->type==MOTE_FIDO) {
    fidolistaarende(motpek, forward ? 1 : -1);
    return;
  }
  puttekn("\r\n\nNamn                              Text    Kom   Datum  Ärende",-1);
  puttekn("\r\n-------------------------------------------------------------------------\r\n",-1);

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
    if(lahead.kom_till_nr!=-1 && baratexter) {
      continue;
    }
    strcpy(namn,getusername(lahead.person));
    lahead.arende[22]=0;
    ts=localtime(&lahead.tid);
    if(lahead.kom_till_nr==-1) strcpy(kom,"   -");
    else sprintf(kom,"%d",lahead.kom_till_nr);
    sprintf(outbuffer,"%-34s%6d %6s %02d%02d%02d %s\r\n", namn,
            lahead.nummer, kom, ts->tm_year % 100, ts->tm_mon + 1,
            ts->tm_mday, lahead.arende);
    if(puttekn(outbuffer,-1)) return;
  }
}

void tellallnodes(char *str) {
	int x;
	struct SayString *pekare,*oldpekare=NULL,*allocpekare;
	for(x=0;x<MAXNOD;x++) {
		if((!Servermem->nodtyp[x]) || Servermem->inloggad[x]<0 || x==nodnr) continue;
		if(Servermem->inne[x].flaggor & NOLOGNOTIFY) continue;
		pekare=Servermem->say[x];
		Forbid();
		while(pekare) {
			oldpekare=pekare;
			pekare=oldpekare->NextSay;
		}
		if(!(allocpekare=(struct SayString *)AllocMem(sizeof(struct SayString),MEMF_PUBLIC | MEMF_CLEAR))) {
			Permit();
			printf("\r\n\nKunde inte allokera minne till meddelandet!\r\n\n");
			return;
		}
		allocpekare->fromuser=inloggad;
		strcpy(allocpekare->text,str);
		if(Servermem->say[x]) oldpekare->NextSay=allocpekare;
		else Servermem->say[x]=allocpekare;
		Permit();
	}
}

int skrivlapp(void) {
	int editret;
	struct EditLine *el;
	FILE *fp;
	char filnamn[100];
	sprintf(filnamn,"NiKom:Users/%d/%d/Lapp",inloggad/100,inloggad);
	puttekn("\r\n\nGår in i editorn.\r\n",-1);
	if((editret=edittext(filnamn))==1) return(1);
	else if(editret==2) return(0);
	if(!(fp=fopen(filnamn,"w"))) {
		puttekn("\r\nKunde inte spara lappen\r\n\n",-1);
		return(0);
	}
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) {
		if(fputs(el->text,fp)) {
			sprintf(outbuffer,"\r\n\nFel vid skrivandet av %s!\r\n",filnamn);
			puttekn(outbuffer,-1);
			fclose(fp);
			return(0);
		}
		fputc('\n',fp);
	}
	fclose(fp);
	freeeditlist();
}

void radlapp(void) {
	char filnamn[100];
	sprintf(filnamn,"NiKom:Users/%d/%d/Lapp",inloggad/100,inloggad);
	if(remove(filnamn)) puttekn("\r\n\nKunde inte radera lappen\r\n",-1);
	else puttekn("\r\n\nLappen raderad\r\n",-1);
}

int userexists(int nummer) {
	struct ShortUser *letpek;
	int found=FALSE;
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			found=TRUE;
			break;
		}
	}
	return(found);
}
