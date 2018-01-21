#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <devices/timer.h>
#include "StringUtils.h"
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Terminal.h"
#include "Languages.h"
#include "UserDataUtils.h"

extern struct System *Servermem;
extern int nodnr,inloggad,senast_text_typ,rad,mote2,buftkn,senast_brev_nr,senast_brev_anv, g_userDataSlot;
extern char outbuffer[],inmat[],backspace[],*argument,vilkabuf[];
extern struct MsgPort *timerport,*conreadport;
extern struct Inloggning Statstr;
extern struct timerequest *timerreq;
extern char coninput;
extern struct MinList edit_list;

int parsenamn(char *skri) {
  int going = TRUE, going2 = TRUE, found = -1, nummer;
  char *faci, *skri2;
  struct ShortUser *letpek;
  if(skri[0] == 0 || skri[0] == ' ') {
    return -3;
  }
  if(IzDigit(skri[0]) || (skri[0]=='#' && IzDigit(skri[1]))) {
    if(skri[0] == '#') {
      skri++;
    }
    nummer = atoi(skri);
    return userexists(nummer) ? nummer : -1;
  }
  if(matchar(skri,"sysop")) {
    return 0;
  }
  letpek = (struct ShortUser *)Servermem->user_list.mlh_Head;
  while(letpek->user_node.mln_Succ && going) {
    faci = letpek->namn;
    skri2 = skri;
    going2 = TRUE;
    if(matchar(skri2, faci)) {
      while(going2) {
        skri2 = hittaefter(skri2);
        faci = hittaefter(faci);
        if(skri2[0] == 0) {
          found = letpek->nummer;
          going2 = going=FALSE;
        } else if(faci[0] == 0) {
          going2 = FALSE;
        } else if(!matchar(skri2, faci)) {
          faci = hittaefter(faci);
          if(faci[0] == 0 || !matchar(skri2, faci)) {
            going2 = FALSE;
          }
        }
      }
    }
    letpek = (struct ShortUser *)letpek->user_node.mln_Succ;
  }
  return found;
}

int matchar(skrivet,facit)
char *skrivet,*facit;
{
	int mat=TRUE,count=0;
	char tmpskrivet,tmpfacit;
	if(facit!=NULL) {
		while(mat && skrivet[count]!=' ' && skrivet[count]!=0) {
			if(skrivet[count]=='*') { count++; continue; }
			tmpskrivet=ToUpper(skrivet[count]);
			tmpfacit=ToUpper(facit[count]);
			if(tmpskrivet!=tmpfacit) mat=FALSE;
			count++;
		}
	}
	return(mat);
}

char *hittaefter(strang)
char *strang;
{
	do
	{
		while(*(strang) != 0 && *(strang) != ' ')
			strang++;

	} while(*(strang) != 0 && *(++strang) == ' ');

	return strang;
}

void listmed(void) {
  int listMembers;
  struct ShortUser *shortUser;
  struct Mote *conf;
  struct User *user;

  if(argument[0] == '-' && (argument[1] == 'g' || argument[1] == 'G')) {
    argument = hittaefter(argument);
    listgruppmed();
    return;
  }
  if(mote2 == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUM_LISTMEM_MAIL));
    return;
  }
  
  if(GetYesOrNo("\r\n\n", CATSTR(MSG_FORUM_LISTMEM_WHAT), NULL, NULL,
                CATSTR(MSG_FORUM_LISTMEM_MEM), CATSTR(MSG_FORUM_LISTMEM_NONMEM),
                "\r\n\n", TRUE, &listMembers)) {
    return;
  }

  conf = getmotpek(mote2);
  SendString("%s - %s\n\n\r",
             listMembers ? CATSTR(MSG_FORUM_LISTMEM_MEM) : CATSTR(MSG_FORUM_LISTMEM_NONMEM),
             conf->namn);
  ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
    if(!(user = GetUserData(shortUser->nummer))) {
      return;
    }
    if((listMembers && IsMemberConf(mote2, shortUser->nummer, user))
       || (!listMembers && !IsMemberConf(mote2, shortUser->nummer, user))) {
      if(SendString("%s #%d\r\n", user->namn, shortUser->nummer)) {
        return;
      }
    }
  }
}

void listratt(void) {
  int listPerm;
  struct ShortUser *shortUser;
  struct Mote *conf;
  struct User *user;

  if(mote2 == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_FORUM_LISTAUTH_MAIL));
    return;
  }
  conf = getmotpek(mote2);
  if(conf->status & AUTOMEDLEM) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUM_LISTAUTH_AUTO));
    return;
  }
  if(conf->status & SUPERHEMLIGT) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_FORUM_LISTAUTH_AREXX));
    return;
  }

  if(GetYesOrNo("\r\n\n", CATSTR(MSG_FORUM_LISTAUTH_WHAT), NULL, NULL,
                CATSTR(MSG_FORUM_LISTAUTH_AUTH), CATSTR(MSG_FORUM_LISTAUTH_NONAUTH),
                "\r\n\n", TRUE, &listPerm)) {
    return;
  }

  SendString("%s - %s\n\n\r",
             listPerm ? CATSTR(MSG_FORUM_LISTAUTH_AUTH) : CATSTR(MSG_FORUM_LISTAUTH_NONAUTH),
             conf->namn);
  ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
    if(!(user = GetUserData(shortUser->nummer))) {
      return;
    }
    if((listPerm && BAMTEST(user->motratt, mote2))
       || (!listPerm && !BAMTEST(user->motratt, mote2))) {
      if(SendString("%s #%d\r\n",user->namn, shortUser->nummer)) {
        return;
      }
    }
  }
}

void listnyheter(void) {
	int x,cnt=0,olasta=FALSE,tot=0;
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	struct Fil *sokpek;
	puttekn("\r\n\n",-1);
	if((cnt=countmail(inloggad,CURRENT_USER->brevpek))) {
		sprintf(outbuffer,"%4d Brevlådan\r\n", cnt);
		puttekn(outbuffer,-1);
		olasta=TRUE;
		tot+=cnt;
	}
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
		if(motpek->status & SUPERHEMLIGT) continue;
		if(IsMemberConf(motpek->nummer, inloggad, CURRENT_USER))
		{
			cnt=countunread(motpek->nummer);
			if(cnt) {
				sprintf(outbuffer,"%4d %s\r\n",cnt,motpek->namn);
				if(puttekn(outbuffer,-1)) return;
				olasta=TRUE;
				tot+=cnt;
			}
		}
	}
	if(!olasta) puttekn("Du har inga olästa texter.",-1);
	else {
		sprintf(outbuffer,"\r\nSammanlagt %d olästa texter.",tot);
		puttekn(outbuffer,-1);
	}
	puttekn("\r\n\n",-1);
	if(CURRENT_USER->flaggor & FILLISTA) nyafiler();
	else {
		for(x=0;x<Servermem->info.areor;x++) {
			if(!arearatt(x, inloggad, CURRENT_USER)) continue;
			olasta=0;
			for(sokpek=(struct Fil *)Servermem->areor[x].ar_list.mlh_TailPred;sokpek->f_node.mln_Pred;sokpek=(struct Fil *)sokpek->f_node.mln_Pred) {
				if((sokpek->flaggor & FILE_NOTVALID) && CURRENT_USER->status < Servermem->cfg->st.filer && sokpek->uppladdare != inloggad) continue;
				if(sokpek->validtime < CURRENT_USER->senast_in) continue;
				else olasta++;
			}
			if(olasta) {
				sprintf(outbuffer,"%2d %s\r\n",olasta,Servermem->areor[x].namn);
				if(puttekn(outbuffer,-1)) return;
			}
		}
	}
}

void listflagg(void) {
  int i;
  SendString("\r\n\n");
  for(i = 31; i > (31 - ANTFLAGG); i--) {
    SendString("%-4s %s\r\n",
               BAMTEST((char *)&CURRENT_USER->flaggor, i)
                 ? CATSTR(MSG_FLAG_ON) : CATSTR(MSG_FLAG_OFF),
               g_FlagNames[31 - i]);
  }
}

int parseFlagName(char *str) {
  int i;
  if(str[0] == '\0' || str[0] == ' ') {
    return -3;
  }
  for(i = 0; i < ANTFLAGG; i++) {
    if(InputMatchesSentence(str, g_FlagNames[i])) {
      return i;
    }
  }
  return -1;
}

void slaav(void) {
	int flagga;
	if((flagga=parseFlagName(argument))==-1) {
		puttekn("\r\n\nFinns ingen flagga som heter så!\r\n\n",-1);
		return;
	} else if(flagga==-3) {
		puttekn("\r\n\nSkriv: Slå Av <flaggnamn>\r\n\n",-1);
		return;
	}
	sprintf(outbuffer,"\r\n\nSlår av flaggan %s\r\n", g_FlagNames[flagga]);
	puttekn(outbuffer,-1);
	if(BAMTEST((char *)&CURRENT_USER->flaggor,31-flagga)) puttekn("Den var påslagen.\r\n\n",-1);
	else puttekn("Den var redan avslagen.\r\n\n",-1);
	BAMCLEAR((char *)&CURRENT_USER->flaggor,31-flagga);
}

void slapa(void) {
	int flagga;
	if((flagga=parseFlagName(argument))==-1) {
		puttekn("\r\n\nFinns ingen flagga som heter så!\r\n\n",-1);
		return;
	} else if(flagga==-3) {
		puttekn("\r\n\nSkriv: Slå På <flaggnamn>\r\n\n",-1);
		return;
	}
	sprintf(outbuffer,"\r\n\nSlår på flaggan %s\r\n", g_FlagNames[flagga]);
	puttekn(outbuffer,-1);
	if(!BAMTEST((char *)&CURRENT_USER->flaggor,31-flagga)) puttekn("Den var avslagen.\r\n\n",-1);
	else puttekn("Den var redan påslagen.\r\n\n",-1);
	BAMSET((char *)&CURRENT_USER->flaggor,31-flagga);
}

int ropa(void) {
  long signals, timesig = 1L << timerport->mp_SigBit,
    consig =1L << conreadport->mp_SigBit;
  int ch, i;
  SendString("\r\n\n");
  for(i = 0; i < 10; i++) {
    SendStringNoBrk("\rSYSOP!! %s vill dig något!! (%d)",
                    CURRENT_USER->namn, i);
    DisplayBeep(NULL);
    timerreq->tr_node.io_Command = TR_ADDREQUEST;
    timerreq->tr_node.io_Message.mn_ReplyPort = timerport;
    timerreq->tr_time.tv_secs = 1;
    timerreq->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)timerreq);
    signals = Wait(timesig | consig);
    if(signals & timesig) {
      WaitIO((struct IORequest *)timerreq);
    }
    if(signals & consig) {
      congettkn();
      if(!CheckIO((struct IORequest *)timerreq)) {
        AbortIO((struct IORequest *)timerreq);
        WaitIO((struct IORequest *)timerreq);
      }
      break;
    }
  }
  if(i == 10) {
    SendString("\r\n\nTyvärr, sysop verkar inte vara tillgänglig för "
               "tillfället\r\n\n");
    return 0;
  }
  SendString("\r\nSysop här! (Tryck Ctrl-Z för att avsluta samtalet.)\r\n");
  strcpy(vilkabuf,"pratar med sysop");
  Servermem->nodeInfo[nodnr].currentActivity = vilkabuf;
  Servermem->nodeInfo[nodnr].action = GORNGTANNAT;
  for(;;) {
    ch = GetChar();
    if(ch == GETCHAR_LOGOUT) {
      return 1;
    }
    if(ch > 0 && IsPrintableCharacter(ch)) {
      SendStringNoBrk("\x1b\x5b\x31\x40");
      eka(ch);
    } else if(ch == GETCHAR_RETURN) {
      SendStringNoBrk("\r\n");
    } else if(ch == GETCHAR_BACKSPACE) {
      SendStringNoBrk("\x1b\x5b\x44\x1b\x5b\x50");
    } else if(ch == GETCHAR_DELETE) {
      SendStringNoBrk("\x1b\x5b\x50");
    } else if(ch == GETCHAR_RIGHT) {
      SendStringNoBrk("\x1b\x5b\x43");
    } else if(ch == GETCHAR_LEFT) {
      SendStringNoBrk("\x1b\x5b\x44");
    } else if(ch == 26) { // Ctrl-Z
      return 0;
    } else if(ch == 7) {
      eka(7);
    }
  }
  return 0;
}

void writemeet(struct Mote *motpek) {
	BPTR fh;
	NiKForbid();
	if(!(fh=Open("NiKom:DatoCfg/Möten.dat",MODE_OLDFILE))) {
		puttekn("\r\n\nKunde inte öppna Möten.dat\r\n",-1);
		NiKPermit();
		return;
	}
	if(Seek(fh,motpek->nummer*sizeof(struct Mote),OFFSET_BEGINNING)==-1) {
		puttekn("\r\n\nKunde inte söka i Moten.dat!\r\n\n",-1);
		Close(fh);
		NiKPermit();
		return;
	}
	if(Write(fh,(void *)motpek,sizeof(struct Mote))==-1) {
		puttekn("\r\n\nFel vid skrivandet av Möten.dat\r\n",-1);
	}
	Close(fh);
	NiKPermit();
}

void addratt(void) {
  int userId, needsWrite;
  struct Mote *motpek;
  struct User *user;
  if(mote2==-1) {
    puttekn("\r\n\nDu kan inte addera rättigheter i brevlådan!\r\n",-1);
    return;
  }
  motpek=getmotpek(mote2);
  if(!MayAdminConf(mote2, inloggad, CURRENT_USER)) {
    puttekn("\r\n\nDu har inte rätt att addera rättigheter i det här mötet!\r\n\n",-1);
    return;
  }
  if((userId=parsenamn(argument))==-1) {
    puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n",-1);
    return;
  } else if(userId==-3) {
    puttekn("\r\n\nSkriv: Addera rättigheter <användare>\r\n(Se till så att du befinner dig i rätt möte)\r\n\n",-1);
    return;
  }

  if(!(user = GetUserDataForUpdate(userId, &needsWrite))) {
    return;
  }
  BAMSET(user->motratt, motpek->nummer);
  if(needsWrite) {
    if(!WriteUser(userId, user, FALSE)) {
      return;
    }
  }
  sprintf(outbuffer,"\n\n\rRättigheter i %s adderade för %s #%d\n\r", motpek->namn, user->namn, userId);
  puttekn(outbuffer,-1);
}

void subratt(void) {
  int userId, needsWrite;
  struct Mote *motpek;
  struct User *user;
  if(mote2==-1) {
    puttekn("\r\n\nDu kan inte subtrahera rättigheter i brevlådan!\r\n",-1);
    return;
  }
  motpek=getmotpek(mote2);
  if(!MayAdminConf(mote2, inloggad, CURRENT_USER)) {
    puttekn("\r\n\nDu har inte rätt att subtrahera rättigheter i det här mötet!\r\n\n",-1);
    return;
  }
  if((userId=parsenamn(argument))==-1) {
    puttekn("\r\n\nFinns ingen som heter så eller har det numret!\r\n\n",-1);
    return;
  } else if(userId==-3) {
    puttekn("\r\n\nSkriv: Subtrahera rättigheter <användare>\r\n(Se till så att du befinner dig i rätt möte)\r\n\n",-1);
    return;
  }

  if(!(user = GetUserDataForUpdate(userId, &needsWrite))) {
    return;
  }
  BAMCLEAR(user->motratt, motpek->nummer);
  BAMCLEAR(user->motmed, motpek->nummer);
  if(needsWrite) {
    if(!WriteUser(userId, user, FALSE)) {
      return;
    }
  }

  sprintf(outbuffer,"\n\n\rRättigheter i %s subtraherade för %s #%d\n\r", motpek->namn, user->namn, userId);
  puttekn(outbuffer,-1);
}
