#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"

#define ERROR   10
#define OK              0
#define EKO             1
#define EJEKO   0
#define KOM             1
#define EJKOM   0
#define BREVKOM -1

extern struct System *Servermem;
extern char outbuffer[],*argument,inmat[];
extern int nodnr,senast_text_typ,senast_text_nr,senast_text_mote,nu_skrivs,inloggad,
        rad,mote2;
extern struct Header readhead,sparhead;
extern long textpek;
extern struct Inloggning Statstr;
extern struct MinList edit_list;

/* MENYNOD */
#define READTEXT 1
#define READBREV 2

#include "MenyStr.h"

extern struct Meny *activemenu;

/* END MENYNOD */


int org_skriv(void) {
        int editret;
        if(org_initheader(EJKOM)) return(1);
        nu_skrivs=TEXT;
        if((editret=edittext(NULL))==1) return(1);
        if(!editret) org_sparatext();
        return(0);
}

int org_kommentera(void) {
        int nummer,editret;
        struct Mote *motpek;
        nummer=atoi(argument);
        if(argument[0]) {
                if(nummer<Servermem->info.lowtext || nummer>Servermem->info.hightext) {
                        puttekn("\r\n\nTexten finns inte!\r\n\n",-1);
                        return(0);
                }
                if(!MayBeMemberConf(Servermem->texts[nummer%MAXTEXTS], inloggad, &Servermem->inne[nodnr])) {
                        puttekn("\r\n\nDu har inga rättigheter i mötet där texten finns!\r\n\n",-1);
                        return(0);
                }
                motpek=getmotpek(Servermem->texts[nummer%MAXTEXTS]);
                if(!motpek) {
                        puttekn("\n\n\rHmm.. Mötet där texten ligger finns inte..\n\r",-1);
                        return(0);
                }
                if(motpek->status & KOMSKYDD) {
                        if(!MayReplyConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
                                puttekn("\r\n\nDu får inte kommentera i kommentarsskyddade möten!\r\n\n",-1);
                                return(0);
                        } else {
                                puttekn("\r\n\nVill du verkligen kommentera i ett kommentarsskyddat möte? ",-1);
                                if(!jaellernej('j','n',2)) return(0);
                        }
                }
                if(readtexthead(nummer,&readhead)) return(0);
                senast_text_typ=TEXT;
                senast_text_nr=nummer;
                senast_text_mote=motpek->nummer;
        }
        motpek=getmotpek(senast_text_mote);
        if(!motpek) {
                puttekn("\n\n\rHmm.. Mötet där texten ligger finns inte..\n\r",-1);
                return(0);
        }
        if(org_initheader(KOM)) return(1);
        nu_skrivs=TEXT;
        if((editret=edittext(NULL))==1) return(1);
        else if(!editret)       {
                org_linkkom();
                org_sparatext();
        }
        return(0);
}

void org_lasa(int tnr) {
        if(tnr<Servermem->info.lowtext || tnr>Servermem->info.hightext) {
                puttekn("\r\n\nTexten finns inte!\r\n",-1);
                return;
        }
        org_visatext(tnr);
}

int checkmote(int conf) {
  long unreadText;

  unreadText = FindNextUnreadText(0, conf, &Servermem->unreadTexts[nodnr]);
  if(unreadText == -1) {
    Servermem->unreadTexts[nodnr].lowestPossibleUnreadText[conf] =
      Servermem->info.hightext+1;
  }
  return unreadText != -1;
}

int clearmote(int conf) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  int going=TRUE, textnr = 0, promret, komret;

  while(going) {
    textnr = FindNextUnreadText(textnr, conf, unreadTexts);
    if(textnr == -1) {
      return -5;
    }
    if((promret=prompt(210))==-1) return(-1);
    else if(promret==-3) return(-3);
    else if(promret==-4) puttekn("\r\n\nFinns ingen nästa kommentar!\r\n\n",-1);
    else if(promret==-8) return(-8);
    else if(promret==-9) return(-9);
    else if(promret==-11) return(-11);
    else if(promret>=0) return(promret);
    /* MENYNOD  tillagt: && activemenu->ret==READTEXT */
    else if(promret==-2 || promret==-6 && activemenu->ret==READTEXT) {
      ChangeUnreadTextStatus(textnr, 0, unreadTexts);
      if(org_visatext(textnr)) {
        if((komret=clearkom())==-1) return(-1);
        else if(komret==-3) return(-3);
        else if(komret==-8) return(-8);
        else if(komret==-9) return(-9);
        else if(komret==-11) return(-11);
        else if(komret>=0) return(komret);
      }
    }
    /* MENYNOD */
    if(activemenu->ret!=READTEXT)
      return(-9);
    /* END MENYNOD */
    if(promret!=-5) textnr++;
  }
}

int clearkom(void) {
  long kom[MAXKOM];
  int promret,komret,x=0;
  memcpy(kom,readhead.kom_i,MAXKOM*sizeof(long));
  while(kom[x]!=-1) {
    if(Servermem->texts[kom[x]%MAXTEXTS]!=-1
       && IsTextUnread(kom[x], &Servermem->unreadTexts[nodnr])
       && IsMemberConf(Servermem->texts[kom[x]%MAXTEXTS], inloggad,
            &Servermem->inne[nodnr])) {
      if((promret=prompt(211))==-1) return(-1);
      else if(promret==-2) return(-2);
      else if(promret==-3) return(-3);
      else if(promret==-8) return(-8);
      else if(promret==-9) return(-9);
      else if(promret==-10) {
        sprintf(outbuffer,"\r\n\nDu hoppade över %d texter.\r\n",hoppaover(readhead.nummer,0)-1);
        puttekn(outbuffer,-1);
        return(-5);
      }
      else if(promret==-11) return(-11);
      else if(promret>=0) return(promret);
      /* MENYNOD  tillagt: && activemenu->ret==READTEXT */
      else if(promret==-4 || promret==-6 && activemenu->ret==READTEXT) {
        ChangeUnreadTextStatus(kom[x], 0, &Servermem->unreadTexts[nodnr]);
        if(org_visatext(kom[x])) {
          if((komret=clearkom())==-1) return(-1);
          else if(komret==-3) return(-3);
          else if(komret==-8) return(-8);
          else if(komret==-9) return(-9);
          else if(komret==-11) return(-11);
          else if(komret>=0) return(komret);
        }
      }
      /* MENYNOD */
      if(activemenu->ret!=READTEXT)
        return(-9);
      /* END MENYNOD */
      if(promret!=-5) x++;
    } else x++;
  }
  return(-5);
}

int org_visatext(int text) {
        int x,length;
        struct tm *ts;
        struct EditLine *el;
        Servermem->inne[nodnr].read++;
        Servermem->info.lasta++;
        Statstr.read++;
        if(Servermem->texts[text%MAXTEXTS]==-1) {
                sprintf(outbuffer,"\n\n\rText %d är raderad!\n\n\r",text);
                puttekn(outbuffer,-1);
                if(Servermem->inne[nodnr].status<Servermem->cfg.st.medmoten) return(0);
        }
        if(readtexthead(text,&readhead)) return(0);
        if(!MayReadConf(readhead.mote, inloggad, &Servermem->inne[nodnr])) {
                puttekn("\r\n\nDu har inte rätt att läsa den texten!\r\n\n",-1);
                return(0);
        }
        ts=localtime(&readhead.tid);
        sprintf(outbuffer,"\r\n\nText %d  Möte: %s    %4d%02d%02d %02d:%02d\r\n",
                readhead.nummer, getmotnamn(readhead.mote),
                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
                ts->tm_min);
        puttekn(outbuffer,-1);
        if(readhead.person!=-1) sprintf(outbuffer,"Skriven av %s\r\n",getusername(readhead.person));
        else sprintf(outbuffer,"Skriven av <raderad användare>\r\n");
        puttekn(outbuffer,-1);
        if(readhead.kom_till_nr!=-1) {
                sprintf(outbuffer,"Kommentar till text %d av %s\r\n",readhead.kom_till_nr,getusername(readhead.kom_till_per));
                puttekn(outbuffer,-1);
        }
        sprintf(outbuffer,"Ärende: %s\r\n",readhead.arende);
        puttekn(outbuffer,-1);
        if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
                length=strlen(outbuffer);
                for(x=0;x<length-2;x++) outbuffer[x]='-';
                outbuffer[x]=0;
                puttekn(outbuffer,-1);
                puttekn("\r\n\n",-1);
        } else puttekn("\n",-1);
        if(readtextlines(TEXT,readhead.textoffset,readhead.rader,readhead.nummer))
                puttekn("\n\rFel vid läsandet i Text.dat\n\r",-1);
        for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) {
                if(puttekn(el->text,-1)) break;
                eka('\r');
        }
        freeeditlist();
        sprintf(outbuffer,"\n(Slut på text %d av %s)\r\n",readhead.nummer,getusername(readhead.person));
        puttekn(outbuffer,-1);
        x=0;
        while(readhead.kom_i[x]!=-1) {
                if(Servermem->texts[readhead.kom_i[x]%MAXTEXTS]!=-1 && IsMemberConf(Servermem->texts[readhead.kom_i[x]%MAXTEXTS], inloggad, &Servermem->inne[nodnr])) {
                        sprintf(outbuffer,"  (Kommentar i text %d av %s)\r\n",readhead.kom_i[x],getusername(readhead.kom_av[x]));
                        puttekn(outbuffer,-1);
                }
                x++;
        }
        senast_text_typ=TEXT;
        senast_text_nr=readhead.nummer;
        senast_text_mote=readhead.mote;
        if(readhead.kom_i[0]!=-1) return(1);
        return(0);
}

void org_sparatext(void) {
        int x, nummer;
        Servermem->inne[nodnr].skrivit++;
        Servermem->info.skrivna++;
        Statstr.write++;
        sparhead.rader=rad;
        for(x=0;x<MAXKOM;x++) {
                sparhead.kom_i[x]=-1;
                sparhead.kom_av[x]=-1;
        }
        nummer = sendservermess(SPARATEXTEN,(long)&sparhead);
        sprintf(outbuffer,"\r\nTexten fick nummer %d\r\n",nummer);
        puttekn(outbuffer,-1);
        if(Servermem->cfg.logmask & LOG_TEXT) {
                sprintf(outbuffer,"%s skriver text %d i %s",getusername(inloggad),nummer,getmotnamn(sparhead.mote));
                logevent(outbuffer);
        }
        freeeditlist();
}

void org_linkkom(void) {
        int x=0;
        struct Header linkhead;
        if(readtexthead(readhead.nummer,&linkhead)) return;
        while(linkhead.kom_i[x]!=-1 && x<MAXKOM) x++;
        if(x==MAXKOM) {
                puttekn("\r\n\nRedan maximalt med kommentarer till den texten, sparar texten i alla fall.\r\n\n",-1);
                return;
        }
        linkhead.kom_i[x]=Servermem->info.hightext+1;
        linkhead.kom_av[x]=sparhead.person;
        writetexthead(readhead.nummer,&linkhead);
}

int org_initheader(int komm) {
        int length=0,x=0;
        long tid;
        struct tm *ts;
        sparhead.person=inloggad;
        if(komm) {
                sparhead.kom_till_nr=readhead.nummer;
                sparhead.kom_till_per=readhead.person;
                sparhead.mote=readhead.mote;
        } else {
                sparhead.kom_till_nr=-1;
                sparhead.kom_till_per=-1;
                sparhead.mote=mote2;
        }
        Servermem->action[nodnr] = SKRIVER;
        Servermem->varmote[nodnr] = sparhead.mote;
        time(&tid);
        ts=localtime(&tid);
        sparhead.tid=tid;
        sparhead.textoffset=(long)&edit_list;
        sparhead.status=0;
        sprintf(outbuffer,"\r\n\nMöte: %s    %4d%02d%02d %02d:%02d\r\n",
                getmotnamn(sparhead.mote), ts->tm_year + 1900, ts->tm_mon + 1,
                ts->tm_mday, ts->tm_hour, ts->tm_min);
        puttekn(outbuffer,-1);
        sprintf(outbuffer,"Skriven av %s\r\n",getusername(inloggad));
        puttekn(outbuffer,-1);
        if(komm) {
                sprintf(outbuffer,"Kommentar till text %d av %s\r\n",sparhead.kom_till_nr,getusername(sparhead.kom_till_per));
                puttekn(outbuffer,-1);
        }
        puttekn("Ärende: ",-1);
        if(komm) {
                strcpy(sparhead.arende,readhead.arende);
                puttekn(sparhead.arende,-1);
        } else {
                if(getstring(EKO,40,NULL)) return(1);
                strcpy(sparhead.arende,inmat);
        }
        puttekn("\r\n",-1);
        if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
                length=strlen(sparhead.arende);
                for(x=0;x<length+8;x++) outbuffer[x]='-';
                outbuffer[x]=0;
                puttekn(outbuffer,-1);
                puttekn("\r\n\n",-1);
        } else puttekn("\n",-1);
        return(0);
}

void org_endast(int conf,int amount) {
  SetUnreadTexts(conf, amount, &Servermem->unreadTexts[nodnr]);
  if(textpek > Servermem->unreadTexts[nodnr].lowestPossibleUnreadText[conf]) {
    textpek = Servermem->unreadTexts[nodnr].lowestPossibleUnreadText[conf];
  }
}
