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
#include <ctype.h>
#include <limits.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"

#define ERROR   10
#define OK              0
#define EKO             1
#define EJEKO   0
#define KOM             1
#define EJKOM   0
#define BREVKOM -1

extern struct System *Servermem;
extern int nodnr,inloggad,radcnt,rxlinecount, nodestate;
extern char outbuffer[],inmat[],backspace[],commandhistory[][1024];
extern struct ReadLetter brevread;
extern struct MinList edit_list;

long logintime, extratime;
int mote2,rad,senast_text_typ,felcnt,nu_skrivs,area2,rexxlogout,
        senast_brev_nr,senast_brev_anv,senast_text_nr,senast_text_mote;
char *month[] = { "januari","februari","mars","april","maj","juni",
                "juli","augusti","september","oktober","november","december" },
                *veckodag[] = { "Söndag","Måndag","Tisdag","Onsdag","Torsdag","Fredag",
                "Lördag" },*argument,usernamebuf[50],aliasbuf[1081],argbuf[1081],vilkabuf[50];
struct Header sparhead,readhead;
struct Inloggning Statstr;
struct MinList aliaslist;

int prompt(int kmd) {
        long klockan,tidgrans,tidkvar;
        int parseret,kmdret, after;
        struct Alias *aliaspek;
        struct Kommando *kmdpek;
        if(Servermem->say[nodnr]) displaysay();
        tidgrans=60*(Servermem->cfg.maxtid[Servermem->inne[nodnr].status])+extratime;
        if(Servermem->cfg.maxtid[Servermem->inne[nodnr].status]) {
                if((time(&klockan)-logintime) > tidgrans) {
                        sendfile("NiKom:Texter/TidenSlut.txt");
                        return(-3);
                }
        }
        if(rexxlogout) {
                rexxlogout=FALSE;
                return(-3);
        }
        Servermem->idletime[nodnr] = time(NULL);
        Servermem->action[nodnr] = LASER;
        Servermem->varmote[nodnr] = mote2;
        if(kmd==221) {
                if(Servermem->cfg.ar.nextmeet) sendautorexx(Servermem->cfg.ar.nextmeet);
                puttekn("\r\n(Gå till) nästa möte ",-1);
        } else if(kmd==210) {
                if(mote2!=-1) {
                        if(Servermem->cfg.ar.nexttext) sendautorexx(Servermem->cfg.ar.nexttext);
                        puttekn("\r\n(Läsa) nästa text ",-1);
                } else {
                        if(Servermem->cfg.ar.nextletter) sendautorexx(Servermem->cfg.ar.nextletter);
                        puttekn("\r\n(Läsa) nästa brev ",-1);
                }
        } else if(kmd==211) {
                if(Servermem->cfg.ar.nextkom) sendautorexx(Servermem->cfg.ar.nextkom);
                puttekn("\r\n(Läsa) nästa kommentar ",-1);
        } else if(kmd==306) {
                Servermem->action[nodnr] = INGET;
                if(Servermem->cfg.ar.setid) sendautorexx(Servermem->cfg.ar.setid);
                puttekn("\r\n(Se) tiden ",-1);
        } else if(kmd==222) {
                if(Servermem->cfg.ar.nextmeet) sendautorexx(Servermem->cfg.ar.nextmeet);
                sprintf(outbuffer,"\r\nGå (till) %s ",Servermem->cfg.brevnamn);
                puttekn(outbuffer,-1);
        }
        tidkvar=logintime+tidgrans-klockan;
        if(tidgrans && tidkvar<300) {
                sprintf(outbuffer,"(%d) %s ",tidkvar/60,Servermem->inne[nodnr].prompt);
                puttekn(outbuffer,-1);
        } else {
                puttekn(Servermem->inne[nodnr].prompt,-1);
                eka(' ');
        }
        if(getstring(EKO,999,NULL)) return(-8);
        if(inmat[0]=='.' || inmat[0]==' ') return(-5);
        if(inmat[0] && (aliaspek=parsealias(inmat)))
        {
			strcpy(aliasbuf,aliaspek->blirtill);
			strcat(aliasbuf," ");
			strncat(aliasbuf,hittaefter(inmat),980);
			strcpy(inmat,aliasbuf);
        }
        if((parseret=parse(inmat))==-1) {
                puttekn("\r\n\nFelaktigt kommando!\r\n",-1);
                if(++felcnt>=2 && !(Servermem->inne[nodnr].flaggor & INGENHELP)) sendfile("NiKom:Texter/2fel.txt");
                return(-5);
        }
        else if(parseret==-3) return(-6);
        else if(parseret==-4) {
                puttekn("\r\n\nDu har ingen rätt att utföra det kommandot!\r\n",-1);
                if(Servermem->cfg.ar.noright) sendautorexx(Servermem->cfg.ar.noright);
                return(-5);
        } else if(parseret==-5) {
                puttekn("\r\n\nFelaktigt lösen!\r\n",-1);
                return(-5);
        } else {
                kmdpek = getkmdpek(parseret);
                if(!kmdpek) return(-5);
                if(kmdpek->before) sendautorexx(kmdpek->before);
                if(kmdpek->logstr[0]) {
                  LogEvent(USAGE_LOG, INFO, "%s %s",
                           getusername(inloggad), kmdpek->logstr);
                }
                if(kmdpek->vilkainfo[0])
                {
                        Servermem->action[nodnr] = GORNGTANNAT;
                        Servermem->vilkastr[nodnr] = kmdpek->vilkainfo;
                }
                after = kmdpek->after;
                kmdret=dokmd(parseret,kmd);
                if(after) sendautorexx(after);
                return(kmdret);
        }
}

int dokmd(int parseret,int kmd) {
        if(parseret==201) return(ga(argument));
        else if(parseret==210) return(-2);
        else if(parseret==211) return(-4);
        else if(parseret==221) return(-1);
        else if(parseret==222) return(-9);
        else if(parseret==301) return(-3);
        else if(parseret==101) listmot(argument);
        else if(parseret==102) { if(listaanv()) return(-8); }
        else if(parseret==103) listmed();
        else if(parseret==104) sendfile("NiKom:Texter/ListaKommandon.txt");
        else if(parseret==105) listratt();
        else if(parseret==106) listasenaste();
        else if(parseret==107) listnyheter();
        else if(parseret==108) listaarende();
        else if(parseret==109) listflagg();
        else if(parseret==111) listarea();
        else if(parseret==112) listnyckel();
        else if(parseret==113) listfiler();
        else if(parseret==114) listagrupper();
        else if(parseret==115) listgruppmed();
        else if(parseret==116) listabrev();
        else if(parseret==202) { if(skriv()) return(-8); }
        else if(parseret==203) { if(kommentera()) return(-8); }
        else if(parseret==204) { if(personlig()) return(-8); }
        else if(parseret==205) { if(skickabrev()) return(-8); }
        else if(parseret==206) igen();
        else if(parseret==207) atersekom();
        else if(parseret==208) medlem(argument);
        else if(parseret==209) uttrad(argument);
        else if(parseret==212) lasa();
        else if(parseret==213) return(endast());
        else if(parseret==214) {
                if(kmd==211) return(-10);
                else puttekn("\r\n\nFinns inga kommentarer att hoppa över!\r\n\n",-1);
        } else if(parseret==215) addratt();
        else if(parseret==216) subratt();
        else if(parseret==217) radtext();
        else if(parseret==218) { if(skapmot()) return(-8); }
        else if(parseret==219) return(radmot());
        else if(parseret==220) var(mote2);
        else if(parseret==223) andmot();
        else if(parseret==224) radbrev();
        else if(parseret==225) rensatexter();
        else if(parseret==226) rensabrev();
        else if(parseret==227) gamlatexter();
        else if(parseret==228) gamlabrev();
        else if(parseret==229) { if(dumpatext()) return(-8); }
        else if(parseret==231) { if(movetext()) return(-8); }
        else if(parseret==232) motesstatus();
        else if(parseret==233) hoppaarende();
        else if(parseret==234) flyttagren();
        else if(parseret==302) sendfile("NiKom:Texter/Help.txt");
        else if(parseret==303) { if(andraanv()) return(-8); }
        else if(parseret==304) slaav();
        else if(parseret==305) slapa();
        else if(parseret==306) tiden();
        else if(parseret==307) { if(ropa()) return(-8); }
        else if(parseret==308) status();
        else if(parseret==309) raderaanv();
        else if(parseret==310) vilka();
        else if(parseret==311) visainfo();
        else if(parseret==312) getconfig();
        else if(parseret==313) writeinfo();
        else if(parseret==314) { if(sag()) return(-8); }
        else if(parseret==315) { if(skrivlapp()) return(-8); }
        else if(parseret==316) radlapp();
        else if(parseret==317) { grab(); return(-11); }
        else if(parseret==318) { if(skapagrupp()) return(-8); }
        else if(parseret==319) { if(andragrupp()) return(-8); }
        else if(parseret==320) raderagrupp();
        else if(parseret==321) { if(adderagruppmedlem()) return(-8); }
        else if(parseret==322) { if(subtraheragruppmedlem()) return(-8); }
        else if(parseret==323) nikversion();
        else if(parseret==324) alias();
        else if(parseret==325) { nodestate |= NIKSTATE_RELOGIN; return(-3); }
        else if(parseret==326) { if(bytnodtyp()) return(-8); }
        else if(parseret==327) bytteckenset();
        else if(parseret==328) SaveCurrentUser(inloggad, nodnr);
        else if(parseret==401) bytarea();
        else if(parseret==402) filinfo();
        else if(parseret==403) { if(upload()) return(-8); }
        else if(parseret==404) { if(download()) return(-8); }
        else if(parseret==405) { if(skaparea()) return(-8); }
        else if(parseret==406) radarea();
        else if(parseret==407) { if(andraarea()) return(-8); }
        else if(parseret==408) { if(skapafil()) return(-8); }
        else if(parseret==409) radfil();
        else if(parseret==410) { if(andrafil()) return(-8); }
        else if(parseret==411) { if(lagrafil()) return(-8); }
        else if(parseret==412) { if(flyttafil()) return(-8); }
        else if(parseret==413) { if(sokfil()) return(-8); }
        else if(parseret==414) filstatus();
        else if(parseret==415) typefil();
        else if(parseret==416) nyafiler();
        else if(parseret==417) validerafil();
        else if(parseret>=500) return(sendrexx(parseret));
        felcnt=0;
        return(-5);
}

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
                        else org_visatext(readhead.kom_till_nr);
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
                if(motpek->type==MOTE_ORGINAL) org_visatext(senast_text_nr);
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

int kommentera(void) {
        struct Mote *motpek;
        if(argument[0]) {
                if(mote2==-1) return(brev_kommentera());
           else {
                motpek=getmotpek(mote2);
                        if(motpek->type==MOTE_ORGINAL) return(org_kommentera());
                        else if(motpek->type==MOTE_FIDO) {
                                if(!MayReplyConf(mote2, inloggad, &Servermem->inne[nodnr])) {
                                        puttekn("\r\n\nDu får inte kommentera den texten!\r\n\n",-1);
                                        return(0);
                                } else {
                                        if(motpek->status & KOMSKYDD) {
                                                puttekn("\r\n\nVill du verkligen kommentera i ett kommentarsskyddat möte? ",-1);
                                                if(!jaellernej('j','n',2)) return(0);
                                        }
                                }
                                return(fido_skriv(KOM,atoi(argument)));
                        }
                }
        }
        if(!senast_text_typ) {
                puttekn("\n\n\rDu har inte läst någon text ännu.\n\r",-1);
                return(0);
        }
        if(senast_text_typ==BREV) return(brev_kommentera());
        else {
                motpek=getmotpek(senast_text_mote);
                if(!motpek) {
                        puttekn("\n\n\rHmm.. Mötet där texten ligger finns inte..\n\r",-1);
                        return(0);
                }
                if(!MayReplyConf(senast_text_mote, inloggad, &Servermem->inne[nodnr])) {
                        puttekn("\r\n\nDu får inte kommentera den texten!\r\n\n",-1);
                        return(0);
                } else {
                        if(motpek->status & KOMSKYDD) {
                                puttekn("\r\n\nVill du verkligen kommentera i ett kommentarsskyddat möte? ",-1);
                                if(!jaellernej('j','n',2)) return(0);
                        }
                }
                if(motpek->type==MOTE_ORGINAL) return(org_kommentera());
                else if(motpek->type==MOTE_FIDO) return(fido_skriv(KOM,senast_text_nr));
                return(0);
        }
}

void lasa(void) {
        int tnr;
        struct Mote *motpek;
        if(!(argument[0]>='0' && argument[0]<='9')) {
                puttekn("\r\n\nSkriv: Läsa <textnr>\r\n\n",-1);
                return;
        }
        tnr=atoi(argument);
        if(mote2==-1) brev_lasa(tnr);
        else {
                motpek=getmotpek(mote2);
                if(!motpek) {
                        puttekn("Hmm.. Mötet du befinner dig i finns inte.\n\r",-1);
                        return;
                }
                if(motpek->type==MOTE_ORGINAL) org_lasa(tnr);
                else if(motpek->type==MOTE_FIDO) fido_lasa(tnr,motpek);
        }
}

void var(int mot) {
        if(mot==-1) varmail();
        else varmote(mot);
}

void varmote(int mote) {
        int cnt;
        struct Mote *motpek;
        motpek = getmotpek(mote);
        sprintf(outbuffer,"\r\n\nDu befinner dig i %s.\r\n",motpek->namn);
        puttekn(outbuffer,-1);
        if(motpek->type == MOTE_FIDO) {
                if(motpek->lowtext > motpek->texter) strcpy(outbuffer,"Det finns inga texter.\n\r");
                else sprintf(outbuffer,"Det finns texter numrerade från %d till %d.\n\r",motpek->lowtext,motpek->texter);
                puttekn(outbuffer,-1);
        }
        cnt=countunread(mote);
        if(!cnt) puttekn("Du har inga olästa texter\r\n",-1);
        else if(cnt==1) {
                puttekn("Du har 1 oläst text\r\n",-1);
        } else {
                sprintf(outbuffer,"Du har %d olästa texter\r\n",cnt);
                puttekn(outbuffer,-1);
        }
}

int ga(char *foo) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  int motnr;
  char buffer[121], kor;
  struct Mote *motpek;

  if(matchar(foo,Servermem->cfg.brevnamn)) return(-9);
  motnr=parsemot(foo);
  if(motnr==-3) {
    puttekn("\r\n\nSkriv: Gå <mötesnamn>\r\n\n",-1);
    return(-5);
  }
  if(motnr==-1) {
    puttekn("\r\n\nFinns inget sådant möte!\r\n\n", -1);
    return(-5);
  }

  if(!IsMemberConf(motnr, inloggad, &Servermem->inne[nodnr])) {
    motpek=getmotpek(motnr);
    if(MayBeMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
      sprintf(buffer,
        "\r\n\nDu är inte medlem i mötet %s, vill du bli medlem? (J/n) ",
        motpek->namn);
      puttekn(buffer , -1);
      while((kor=gettekn())!='j' && kor!='J' && kor!='n' && kor!='N' && kor!='\r');
      if(kor=='j' || kor=='J' || kor=='\r') {
        puttekn("Ja!",-1);
        BAMSET(Servermem->inne[nodnr].motmed, motnr);
        if(motpek->type == MOTE_ORGINAL) {
          unreadTexts->lowestPossibleUnreadText[motnr] = 0;
        }
        else if(motpek->type == MOTE_FIDO) {
          unreadTexts->lowestPossibleUnreadText[motnr] = motpek->lowtext;
        }
      }
      else {
        puttekn("Nej!\r\n", -1);
        return(-5);
      }
    }
    else
      return(-5);
  }
  return(motnr);
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
        struct ShortUser *userletpek;
        int going=TRUE,x=0,y,mad,clearmed,setratt,changed;
        struct FidoDomain *domainpek;
        BPTR lock;
        struct User skuser;
        struct Mote tempmote,*motletpek,*allok;
        char tkn;
        memset(&tempmote,0,sizeof(struct Mote));
        if(argument[0]==0) {
                puttekn("\r\n\nNamn på mötet: ",-1);
                if(getstring(EKO,40,NULL)) return(1);
                strcpy(tempmote.namn,inmat);
        } else strcpy(tempmote.namn,argument);
        if(parsemot(tempmote.namn)!=-1) {
                puttekn("\r\n\nFinns redan ett sådant möte!\r\n",-1);
                return(0);
        }
        tempmote.skapat_tid=time(NULL);;
        tempmote.skapat_av=inloggad;
/*      while(going) {
*               puttekn("\n\rMaximalt antal texter i mötet: ",-1);
*               if(getstring(EKO,5,NULL)) return(1);
*               if(tempmote.max_texter=atoi(inmat)) going=FALSE;
*       } */
        going=TRUE;
        while(going)
        {
                puttekn("\r\nMötesAdministratör (MAD) : ",-1);
                if(getstring(EKO,5,NULL)) return(1);
                if(inmat[0]) {
                        if((mad=parsenamn(inmat))==-1) puttekn("\r\nFinns ingen sådan användare!",-1);
                        else {
                                tempmote.mad=mad;
                                break;
                        }
                }
        }
        for(;;) {
                puttekn("\n\rSorteringsvärde: ",-1);
                if(getstring(EKO,10,NULL)) return(1);
                if(!inmat[0]) continue;
                tempmote.sortpri=atoi(inmat);
                if(tempmote.sortpri<0 || tempmote.sortpri>LONG_MAX) {
                        sprintf(outbuffer,"\n\rVärdet måste vara mellan 0 och %d!",LONG_MAX);
                        puttekn(outbuffer,-1);
                        continue;
                }
                break;
        }
        puttekn("\r\nSka mötet vara slutet? ",-1);
        if(!jaellernej('j','n',2)) {
                puttekn("Öppet",-1);
        } else {
                tempmote.status |= SLUTET;
                puttekn("Slutet",-1);
                puttekn("\r\nVilka grupper ska ha tillgång till mötet? (? för lista)\r\n",-1);
                if(editgrupp((char *)&tempmote.grupper)) return(1);
        }
        puttekn("\r\nSka mötet vara skrivskyddat? ",-1);
        if(!jaellernej('j','n',2)) {
                puttekn("Oskyddat",-1);
        } else {
                tempmote.status |= SKRIVSKYDD;
                puttekn("Skyddat",-1);
        }
        puttekn("\r\nSka mötet vara kommentarsskyddat? ",-1);
        if(!jaellernej('j','n',2)) {
                puttekn("Oskyddat",-1);
        } else {
                tempmote.status |= KOMSKYDD;
                puttekn("Skyddat",-1);
        }
        puttekn("\r\nSka mötet vara hemligt? ",-1);
        if(!jaellernej('j','n',2)) {
                puttekn("Ej hemligt",-1);
        } else {
                tempmote.status |= HEMLIGT;
                puttekn("Hemligt",-1);
        }
        if(!(tempmote.status & SLUTET)) {
                puttekn("\r\nSka alla användare bli medlemmar automagiskt? ",-1);
                if(!jaellernej('j','n',2)) {
                        puttekn("Nej",-1);
                } else {
                        tempmote.status |= AUTOMEDLEM;
                        puttekn("Ja",-1);
                }
                puttekn("\r\nSka rättigheterna styra skrivmöjlighet? ",-1);
                if(!jaellernej('j','n',2)) {
                        puttekn("Nej",-1);
                } else {
                        tempmote.status |= SKRIVSTYRT;
                        puttekn("Ja",-1);
                        puttekn("\r\nVilka grupper ska ha tillgång till mötet? (? för lista)\r\n",-1);
                        if(editgrupp((char *)&tempmote.grupper)) return(1);
                }
        }
        puttekn("\r\nSka mötet enbart kommas åt från ARexx? ",-1);
        if(!jaellernej('j','n',2)) {
                puttekn("Nej",-1);
        } else {
                tempmote.status |= SUPERHEMLIGT;
                puttekn("Ja",-1);
        }
        puttekn("\n\n\rVilken typ av möte ska det vara?\n\r",-1);
        puttekn("1: Lokalt möte\n\r",-1);
        puttekn("2: Fido-möte\n\n\r",-1);
        puttekn("Val: ",-1);
        for(;;) {
                tkn=gettekn();
                if(tkn=='1' || tkn=='2') break;
        }
        if(tkn=='1') {
                puttekn("Lokalt möte\n\n\r",-1);
                tempmote.type=MOTE_ORGINAL;
        } else {
                puttekn("Fido-möte\n\n\r",-1);
                tempmote.type=MOTE_FIDO;
                puttekn("I vilken katalog ligger .msg-filerna? ",-1);
                if(getstring(EKO,79,NULL)) return(1);
                strcpy(tempmote.dir,inmat);
                if(!(lock=Lock(tempmote.dir,SHARED_LOCK))) {
                        if(!(lock=CreateDir(tempmote.dir)))
                                puttekn("\n\rKunde inte skapa katalogen\n\r",-1);
                }
                if(lock) UnLock(lock);
                puttekn("\n\rVilket tag-namn har mötet? ",-1);
                if(getstring(EKO,49,NULL)) return(1);
                strcpy(tempmote.tagnamn,inmat);
                puttekn("\n\rVilken origin-rad ska användas för mötet? ",-1);
                if(getstring(EKO, 69, Servermem->fidodata.defaultorigin)) return(1);
                strcpy(tempmote.origin,inmat);
                puttekn("\n\n\rVilken teckenuppsättning ska användas för utgående texter?\n\r",-1);
                puttekn("1: ISO Latin 1 (ISO 8859-1)\n\r",-1);
                puttekn("2: SIS-7 (SF7, 'Måsvingar')\n\r",-1);
                puttekn("3: IBM CodePage\n\r",-1);
                puttekn("4: Mac\n\n\r",-1);
                puttekn("Val: ",-1);
                for(;;) {
                        tkn=gettekn();
                        if(tkn=='1' || tkn=='2' || tkn=='3' || tkn=='4') break;
                }
                if(tkn=='1') {
                        puttekn("ISO Latin 1\n\n\r",-1);
                        tempmote.charset = CHRS_LATIN1;
                } else if(tkn=='2') {
                        puttekn("SIS-7\n\n\r",-1);
                        tempmote.charset = CHRS_SIS7;
                } else if(tkn=='3') {
                        puttekn("IBM CodePage\n\n\r",-1);
                        tempmote.charset = CHRS_CP437;
                } else if(tkn=='4') {
                        puttekn("Mac\n\n\r",-1);
                        tempmote.charset = CHRS_MAC;
                }
                puttekn("Vilken domän är mötet i?\n\r",-1);
                for(x=0;x<10;x++) {
                        if(!Servermem->fidodata.fd[x].domain[0]) break;
                        sprintf(outbuffer,"%3d: %s (%d:%d/%d.%d)\n\r",Servermem->fidodata.fd[x].nummer,
                                                                     Servermem->fidodata.fd[x].domain,
                                                                     Servermem->fidodata.fd[x].zone,
                                                                     Servermem->fidodata.fd[x].net,
                                                                     Servermem->fidodata.fd[x].node,
                                                                     Servermem->fidodata.fd[x].point);
                        puttekn(outbuffer,-1);
                }
                if(!x) {
                        puttekn("\n\rDu måste definiera en domän i NiKomFido.cfg först!\n\r",-1);
                        return(0);
                }
                going = TRUE;
                while(going) {
                        if(getstring(EKO,10,NULL)) return(1);
                        if(domainpek=getfidodomain(atoi(inmat),0)) break;
                        else puttekn("\n\rFinns ingen sådan domän.\n\r",-1);
                }
                tempmote.domain = domainpek->nummer;
                puttekn(domainpek->domain,-1);
                puttekn("\n\n\r",-1);
        }
        for(x=0;x<MAXMOTE;x++) if(!getmotpek(x)) break;
        if(x>=MAXMOTE) {
                puttekn("\n\n\rDet finns inte plats för fler möten.\n\r",-1);
                return(0);
        }
        tempmote.nummer=x;
        if(!(allok=(struct Mote *)AllocMem(sizeof(struct Mote),MEMF_CLEAR | MEMF_PUBLIC))) {
                puttekn("\n\n\rKunde inte allokera en struct Mote\n\r",-1);
                return(0);
        }
        memcpy(allok,&tempmote,sizeof(struct Mote));
        for(motletpek=(struct Mote *)Servermem->mot_list.mlh_Head;motletpek->mot_node.mln_Succ;motletpek=(struct Mote *)motletpek->mot_node.mln_Succ) {
                if(motletpek->sortpri > allok->sortpri) break;
        }

        motletpek=(struct Mote *)motletpek->mot_node.mln_Pred;
        Insert((struct List *)&Servermem->mot_list,(struct Node *)allok,(struct Node *)motletpek);
        writemeet(allok);
        if((allok->status & AUTOMEDLEM) && !(allok->status & SKRIVSTYRT)) return(0);
        if(allok->status & SUPERHEMLIGT) return(0);
        if(allok->status & AUTOMEDLEM) clearmed=FALSE;
        else clearmed=TRUE;
        if(allok->status & (SLUTET | SKRIVSTYRT)) setratt=FALSE;
        else setratt=TRUE;
        for(y=0;y<MAXNOD;y++) {
                BAMCLEAR(Servermem->inne[y].motmed,allok->nummer);
                if(setratt) BAMSET(Servermem->inne[y].motratt,allok->nummer);
                else BAMCLEAR(Servermem->inne[y].motratt,allok->nummer);
        }
        puttekn("\r\nÄndrar i Users.dat..\r\n",-1);
        for(userletpek=(struct ShortUser *)Servermem->user_list.mlh_Head;userletpek->user_node.mln_Succ;userletpek=(struct ShortUser *)userletpek->user_node.mln_Succ) {
                if(!(userletpek->nummer%10)) {
                        sprintf(outbuffer,"\r%d",userletpek->nummer);
                        puttekn(outbuffer,-1);
                }
                if(readuser(userletpek->nummer,&skuser)) return(0);
                changed=FALSE;
                if(setratt!=BAMTEST(skuser.motratt,allok->nummer)) {
                        if(setratt) BAMSET(skuser.motratt,allok->nummer);
                        else BAMCLEAR(skuser.motratt,allok->nummer);
                        changed=TRUE;
                }
                if(clearmed && BAMTEST(skuser.motmed,allok->nummer)) {
                        BAMCLEAR(skuser.motmed,allok->nummer);
                        changed=TRUE;
                }
                if(changed && writeuser(userletpek->nummer,&skuser)) return(0);

        }
        for(y=0;y<MAXNOD;y++) {
                BAMCLEAR(Servermem->inne[y].motmed,allok->nummer);
                if(setratt) BAMSET(Servermem->inne[y].motratt,allok->nummer);
                else BAMCLEAR(Servermem->inne[y].motratt,allok->nummer);
        }
        BAMSET(Servermem->inne[nodnr].motratt,allok->nummer);
        BAMSET(Servermem->inne[nodnr].motmed,allok->nummer);
        if(allok->type == MOTE_FIDO) ReScanFidoConf(allok,0);
        return(0);
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
        if(isdigit(arg2[0])) argtyp=KOMARGNUM;
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
                puttekn("\r\n\nLösen: ",-1);
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

void medlem(char *foo) {
  struct UnreadTexts *unreadTexts = &Servermem->unreadTexts[nodnr];
  struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
  BPTR lock;
  int motnr, patret, ant = 0;
  char filnamn[40], motpattern[101], buffer[61];

  if(foo[0]=='-' && (foo[1]=='a' || foo[1]=='A')) {
    for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
      if(MayBeMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])
         && !IsMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
        BAMSET(Servermem->inne[nodnr].motmed,motpek->nummer);
        if(motpek->type==MOTE_ORGINAL) {
          unreadTexts->lowestPossibleUnreadText[motpek->nummer] = 0;
        }
        else if(motpek->type==MOTE_FIDO) {
          unreadTexts->lowestPossibleUnreadText[motpek->nummer] = motpek->lowtext;
        }
      }
    }
    return;
  }
  /* TK: 970201 wildcard stöd! */
  strncpy(buffer,foo,50);
  patret = ParsePatternNoCase(buffer, motpattern, 98);
  if(patret == -1)
    return;
  else if(patret == 1) {
    puttekn("\r\n\n", -1);
    for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
      if(MayBeMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])
         && !IsMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])) {
        strncpy(buffer,foo,50);
        ParsePatternNoCase(buffer, motpattern, 98);
        if(MatchPatternNoCase(motpattern, motpek->namn)) {
          ant++;
          sprintf(buffer,"Du är nu med i mötet %s\r\n", motpek->namn);
          puttekn(buffer,-1);
          BAMSET(Servermem->inne[nodnr].motmed, motpek->nummer);
          if(motpek->type==MOTE_ORGINAL) {
            unreadTexts->lowestPossibleUnreadText[motpek->nummer] = 0;
          }
          else if(motpek->type==MOTE_FIDO) {
            unreadTexts->lowestPossibleUnreadText[motpek->nummer] = motpek->lowtext;
          }
        }
      }
    }
    if(ant != 1)
      sprintf(buffer,"\r\nDu gick med i totalt %d möten\r\n",ant);
    else
      strcpy(buffer,"\r\nDu gick med i ett möte\r\n");

    puttekn(buffer,-1);
    return;
  }

  motnr=parsemot(foo);
  if(motnr==-3) {
    puttekn("\r\n\nSkriv: Medlem <mötesnamn>\r\neller: Medlem -a för att bli med i alla möten.\r\n\n",-1);
    return;
  }
  if(motnr==-1) {
    puttekn("\r\n\nFinns inget sådant möte!\r\n\n",-1);
    return;
  }
  if(!MayBeMemberConf(motnr, inloggad, &Servermem->inne[nodnr])) {
    puttekn("\r\n\nDu har inte rätt att vara med i det mötet!\r\n\n",-1);
    return;
  }
  BAMSET(Servermem->inne[nodnr].motmed,motnr);
  sprintf(outbuffer, "\r\n\nDu är nu med i mötet %s.\r\n\n",getmotnamn(motnr));
  puttekn(outbuffer, -1);
  motpek=getmotpek(motnr);
  if(motpek->type==MOTE_ORGINAL) {
    unreadTexts->lowestPossibleUnreadText[motnr] = 0;
  }
  else if(motpek->type==MOTE_FIDO) {
    unreadTexts->lowestPossibleUnreadText[motnr] = motpek->lowtext;
  }
  sprintf(filnamn,"NiKom:Lappar/%d.motlapp",motnr);
  if(lock=Lock(filnamn,ACCESS_READ)) {
    UnLock(lock);
    sendfile(filnamn);
  }
}

int uttrad(char *foo) {
        struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        int motnr, ret=-5, patret, ant = 0;
        char motpattern[101], buffer[61];

        if(foo[0]=='-' && (foo[1]=='a' || foo[1]=='A'))
        {
			for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ)
			{
				if(IsMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr]))
					BAMCLEAR(Servermem->inne[nodnr].motmed,motpek->nummer);
			}
			return -1;
        }
										/* TK: 970201 wildcard stöd! */
		strncpy(buffer,foo,50);
		patret = ParsePatternNoCase(buffer, motpattern, 98);

		if(patret == -1)
			return -4;
		else if(patret == 1)
		{
			puttekn("\r\n\n", -1);
			for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ)
			{
				if(IsMemberConf(motpek->nummer, inloggad, &Servermem->inne[nodnr])
				  && MatchPatternNoCase(motpattern, motpek->namn))
				{
					ant++;
					sprintf(buffer,"Du gick ur mötet %s\r\n", motpek->namn);
					puttekn(buffer,-1);
					BAMCLEAR(Servermem->inne[nodnr].motmed, motpek->nummer);
				}
			}

			if(ant != 1)
				sprintf(buffer,"\r\nDu gick ur totalt %d möten\r\n",ant);
			else
				strcpy(buffer,"\r\nDu gick ur ett möte\r\n");

			puttekn(buffer,-1);

			return -1;
		}

        motnr=parsemot(foo);
        if(motnr==-3) {
                puttekn("\r\n\nSkriv: Utträd <mötesnamn>\r\n\n",-1);
                return(-5);
        }
        if(motnr==-1) {
                puttekn("\r\n\nFinns inget sådant möte!\r\n\n",-1);
                return(-5);
        }
        motpek=getmotpek(motnr);
        if(motpek->status & AUTOMEDLEM) {
                puttekn("\n\n\rDu kan inte utträda ur det mötet!\n\r",-1);
                return(-5);
        }
        if(!IsMemberConf(motnr, inloggad, &Servermem->inne[nodnr])) {
                puttekn("\r\n\nDu är inte medlem i det mötet!\r\n\n",-1);
                return(-5);
        }
        if(motnr==mote2) ret=-1;
        BAMCLEAR(Servermem->inne[nodnr].motmed,motnr);
        sprintf(outbuffer,"\r\n\nDu har nu utträtt ur mötet %s.\r\n\n",getmotnamn(motnr));
        puttekn(outbuffer,-1);
        return(ret);
}

int unread(int meet) {
        struct Mote *motpek;
        if(meet==-1) return(checkmail());
        if(!IsMemberConf(meet, inloggad, &Servermem->inne[nodnr])) {
                return(FALSE);
        }
        motpek=getmotpek(meet);
        if(motpek->type==MOTE_ORGINAL) return(checkmote(meet));
        if(motpek->type==MOTE_FIDO) return(checkfidomote(motpek));
        return(0);
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

int nextmeet(int curmeet) {
        struct Mote *motpek;
        int seekmeet=0,mailiscur=FALSE;
        if(checkmail()) return(-1);
        if(curmeet==-1) {
                motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
                mailiscur=TRUE;
        } else {
                motpek=getmotpek(curmeet);
                motpek=(struct Mote *)motpek->mot_node.mln_Succ;
                if(!motpek->mot_node.mln_Succ) motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
        }
        seekmeet=motpek->nummer;
        while(seekmeet!=curmeet) {
                if(!(motpek->status & SUPERHEMLIGT) && unread(seekmeet)) return(seekmeet);
                motpek=(struct Mote *)motpek->mot_node.mln_Succ;
                if(!motpek->mot_node.mln_Succ) {
                        if(mailiscur) return(-2);
                        motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
                }
                seekmeet=motpek->nummer;
        }
        return(-2);
}

int clearmeet(int meet) {
        struct Mote *motpek;
        if(meet==-1) return(mail());
        motpek=getmotpek(meet);
        if(motpek->type==MOTE_ORGINAL) return(clearmote(meet));
        if(motpek->type==MOTE_FIDO) return(clearfidomote(motpek));
        return(-5);
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

int connection(void)
{
        int promret,motret,foo;
        char tellstr[100];
        dellostsay();
        LoadProgramCategory(inloggad);
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
          LogEvent(USAGE_LOG, INFO, "%s loggar in på nod %d",
                   getusername(inloggad), nodnr);
        }
        sprintf(tellstr,"loggade just in på nod %d",nodnr);
        tellallnodes(tellstr);
        area2=Servermem->inne[nodnr].defarea;
        if(area2<0 || area2>Servermem->info.areor || !Servermem->areor[area2].namn || !arearatt(area2, inloggad, &Servermem->inne[nodnr])) area2=-1;
        initgrupp();
        rexxlogout=FALSE;
        rxlinecount = TRUE;
        radcnt=0;
        if(Servermem->cfg.ar.postinlogg) sendrexx(Servermem->cfg.ar.postinlogg);
        nikversion();
        var(mote2);
        for(;;) {
                if(unread(mote2)) {
                        motret=clearmeet(mote2);
                        if(motret==-1) {          /* Nästa Möte */
                                foo=nextmeet(mote2);
                                if(foo==-2) puttekn("\n\n\rFinns inget mer möte med olästa texter!\n\r",-1);
                                else {
                                        mote2=foo;
                                        var(mote2);
                                }
                                continue;
                        }
                        else if(motret==-3) return(0); /* Logga ut */
                        else if(motret==-8) return(1); /* Carriern släppt */
                        else if(motret==-9) {          /* Gå till brevlådan */
                                mote2=-1;
                                var(mote2);
                                continue;
                        }
                        else if(motret==-11) {         /* Endast anropat */
                                var(mote2);
                                continue;
                        }
                        else if(motret>=0) {           /* Gå till ngt möte */
                                mote2=motret;
                                var(mote2);
                                continue;
                        }
                }
                foo=nextmeet(mote2);
                if(foo!=-2) {
                        if(foo==-1) promret=prompt(222);
                        else promret=prompt(221);
                        if(promret==-1 || promret==-6) {  /* Gå till mötet */
                                mote2=foo;
                                var(mote2);
                                continue;
                        }
                        else if(promret==-2) {            /* Läsa nästa text */
                                puttekn("\n\n\rFinns inga olästa texter i detta möte\n\r",-1);
                                continue;
                        }
                        else if(promret==-3) return(0);
                        else if(promret==-4) {            /* Läsa nästa kommentar */
                                puttekn("\n\n\rFinns inga fler olästa kommentarer\n\r",-1);
                                continue;
                        }
                        else if(promret==-5) continue;    /* Ngt oviktigt kommando givet */
                        else if(promret==-8) return(1);
                        else if(promret==-9) {            /* Gå till brevlådan */
                                mote2=-1;
                                var(mote2);
                                continue;
                        }
                        else if(promret==-11) {           /* Endast anropat */
                                var(mote2);
                                continue;
                        }
                        else if(promret>=0) {             /* Gå till ngt möte */
                                mote2=promret;
                                var(mote2);
                                continue;
                        }
                }
                promret=prompt(306);
                if(promret==-1) puttekn("\r\n\nFinns inga fler möten med olästa texter\r\n",-1);
                else if(promret==-2) puttekn("\r\n\nFinns inga olästa texter i detta möte\r\n",-1);
                else if(promret==-3) return(0);
                else if(promret==-4) puttekn("\r\n\nFinns inga olästa kommentarer\r\n",-1);
                else if(promret==-6) tiden();
                else if(promret==-8) return(1);
                else if(promret==-9) {
                        mote2=-1;
                        var(mote2);
                }
                else if(promret==-11) var(mote2);
                else if(promret>=0) {
                        mote2=promret;
                        var(mote2);
                }
        }
}



/* Temp! timing .. */

void starttimer(unsigned int *start)
{
	timer(start);
}

void endtimer(char *string, unsigned int *startclock)
{
	unsigned int curclock[2];
	unsigned int sekunder;
	int millisekunder;

	timer(curclock);

	sekunder = curclock[0] - startclock[0];
	millisekunder = curclock[1] - startclock[1] - 500;

	while(millisekunder < 0)
	{
		if(sekunder > 0)
		{
			sekunder--;
			millisekunder += 1000000;
		}
		else
			millisekunder = -millisekunder;
	}

	printf("Tid: %u.%d (%s)\n", sekunder, millisekunder, string);
}

