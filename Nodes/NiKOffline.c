#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"

char *hittaefter(char *);

extern struct System *Servermem;
extern int nodnr,inloggad,radcnt;
extern char outbuffer[];
extern struct Inloggning Statstr;
extern struct MinList edit_list;

struct Header grabhead;
struct ReadLetter brevgrab;

int grabfidotext(int text,struct Mote *motpek,FILE *fpgrab) {
	struct FidoText *ft;
	struct FidoLine *fl;
	char filnamn[20],fullpath[100];
	CURRENT_USER->read++;
	Servermem->info.lasta++;
	Statstr.read++;
	sprintf(filnamn,"%ld.msg",text - motpek->renumber_offset);
	strcpy(fullpath,motpek->dir);
	AddPart(fullpath,filnamn,99);
	if(CURRENT_USER->flaggor & SHOWKLUDGE) ft=ReadFidoTextTags(fullpath,TAG_DONE);
	else ft=ReadFidoTextTags(fullpath,RFT_NoKludges,TRUE,RFT_NoSeenBy,TRUE,TAG_DONE);
	if(!ft) {
		puttekn("\n\n\rKunde inte läsa texten\n\r",-1);
		return(1);
	}
	fprintf(fpgrab,"\n\nText %d  Möte: %s    %s\n",text,motpek->namn,ft->date);
	fprintf(fpgrab,"Skriven av %s (%d:%d/%d.%d)\n",ft->fromuser,ft->fromzone,ft->fromnet,ft->fromnode,ft->frompoint);
	fprintf(fpgrab,"Till: %s\n",ft->touser);
	fprintf(fpgrab,"Ärende: %s\n\n",ft->subject);
	for(fl=(struct FidoLine *)ft->text.mlh_Head;fl->line_node.mln_Succ;fl=(struct FidoLine *)fl->line_node.mln_Succ) {
		if(fl->text[0] == 1) fprintf(fpgrab,"^A%s\n",&fl->text[1]);
		else fprintf(fpgrab,"%s\n",fl->text);
	}
	fprintf(fpgrab,"\n(Slut på text %d av %s)\n",text,ft->fromuser);
	FreeFidoText(ft);
	fprintf(fpgrab,">>>>>");
	return(0);
}

int grabtext(int text,FILE *fpgrab) {
  int x, confId;
	struct EditLine *el;
	struct tm *ts;
	CURRENT_USER->read++;
	Servermem->info.lasta++;
	Statstr.read++;
	if(GetConferenceForText(text) == -1) {
		puttekn("\r\n\nTexten är raderad!\r\n\n",-1);
		if(CURRENT_USER->status<Servermem->cfg->st.medmoten) return(0);
	}
	if(readtexthead(text,&grabhead)) return(2);
	if(!MayReadConf(grabhead.mote, inloggad, CURRENT_USER)) {
		puttekn("\r\n\nDu har inte rätt att läsa den texten!\r\n\n",-1);
		return(0);
	}
        ChangeUnreadTextStatus(text, 0, CUR_USER_UNREAD);
	ts=localtime(&grabhead.tid);
	fprintf(fpgrab,"\n\nText %ld  Möte: %s    %4d%02d%02d %02d:%02d\n",
                grabhead.nummer, getmotnamn(grabhead.mote), ts->tm_year + 1900,
                ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min);
	if(grabhead.person!=-1) fprintf(fpgrab,"Skriven av %s\n",getusername(grabhead.person));
	else fprintf(fpgrab,"Skriven av <raderad användare>\n");
	if(grabhead.kom_till_nr!=-1)
		fprintf(fpgrab,"Kommentar till text %ld av %s\n",grabhead.kom_till_nr,getusername(grabhead.kom_till_per));
	fprintf(fpgrab,"Ärende: %s\n\n",grabhead.arende);
	if(readtextlines(grabhead.textoffset,grabhead.rader,grabhead.nummer))
		puttekn("\n\rFel vid läsandet i Text.dat\n\r",-1);
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) {
		if(fputs(el->text,fpgrab)) {
			freeeditlist();
			return(2);
		}
	}
	freeeditlist();
	fprintf(fpgrab,"\n(Slut på text %ld av %s)\n",grabhead.nummer,getusername(grabhead.person));
	x=0;
	while(grabhead.kom_i[x] != -1) {
          confId = GetConferenceForText(grabhead.kom_i[x]);
          if(confId != -1
             && IsMemberConf(confId, inloggad, CURRENT_USER)) {
            fprintf(fpgrab,"  (Kommentar i text %ld av %s)\n",
                    grabhead.kom_i[x],getusername(grabhead.kom_av[x]));
          }
          x++;
	}
	fprintf(fpgrab,">>>>>");
	if(grabhead.kom_i[0]!=-1) return(1);
	return(0);
}

void grabfidobrev(struct ReadLetter *brevread, BPTR fh, int brev, FILE *fpgrab) {
	int length,x;
	char textbuf[100];
	CURRENT_USER->read++;
	Servermem->info.lasta++;
	Statstr.read++;
	fprintf(fpgrab,"\n\nText %d  i brevlådan hos %s\n",brev,getusername(inloggad));
	fprintf(fpgrab,"Fido-nätbrev,  %s\n",brevread->date);
	fprintf(fpgrab,"Avsändare: %s\n",brevread->from);
	fprintf(fpgrab,"Mottagare: %s\n",brevread->to);
	fprintf(fpgrab,"Ärende: %s\n",brevread->subject);
	if(CURRENT_USER->flaggor & STRECKRAD) {
		length=strlen(outbuffer);
		for(x=0;x<length-2;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		fputs(outbuffer,fpgrab);
		fputs("\n\n",fpgrab);
	} else fputs("\n",fpgrab);
	while(FGets(fh,textbuf,99)) {
		if(textbuf[0]==1) {
			if(!(CURRENT_USER->flaggor & SHOWKLUDGE)) continue;
			fputs("^A",fpgrab);
			fputs(&textbuf[1],fpgrab);
		} else {
			fputs(textbuf,fpgrab);
		}
	}
	Close(fh);
	fprintf(fpgrab,"\n(Slut på text %d av %s)\n",brev,brevread->from);
	fprintf(fpgrab,">>>>>");
}

int grabbrev(int text, FILE *fpgrab) {
	BPTR fh;
	int x,length=0;
	char filnamn[40],*mottagare,textbuf[100];
	CURRENT_USER->read++;
	Servermem->info.lasta++;
	Statstr.read++;
	sprintf(filnamn,"NiKom:Users/%d/%d/%d.letter",inloggad/100,inloggad,text);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		sprintf(outbuffer,"\n\n\rKunde inte öppna %s\n\r",filnamn);
		puttekn(outbuffer,-1);
		return(1);
	}
	readletterheader(fh,&brevgrab);
	if(!strncmp(brevgrab.systemid,"Fido",4)) {
		grabfidobrev(&brevgrab,fh,text,fpgrab);
		return(0);
	}
	fprintf(fpgrab,"\n\nText %d  i brevlådan hos %s\n",text, getusername(inloggad));
	if(!strncmp(brevgrab.systemid,"NiKom",5)) fprintf(fpgrab,"Lokalt brev,  %s\n",brevgrab.date);
	else fprintf(fpgrab,"<Okänd brevtyp>  %s\n",brevgrab.date);
	fprintf(fpgrab,"Avsändare: %s\n",getusername(atoi(brevgrab.from)));
	if(brevgrab.reply[0]) fprintf(fpgrab,"Kommentar till en text av %s\n",getusername(atoi(brevgrab.reply)));
	mottagare=brevgrab.to;
	while(mottagare[0]) {
		fprintf(fpgrab,"Mottagare: %s\n",getusername(atoi(mottagare)));
		mottagare=hittaefter(mottagare);
	}
	fprintf(fpgrab,"Ärende: %s\n",brevgrab.subject);
	if(CURRENT_USER->flaggor & STRECKRAD) {
		length=strlen(outbuffer);
		for(x=0;x<length-2;x++) outbuffer[x]='-';
		outbuffer[x]=0;
		fputs(outbuffer,fpgrab);
		fputs("\n\n",fpgrab);
	} else fputs("\n",fpgrab);

	while(FGets(fh,textbuf,99)) fputs(textbuf,fpgrab);
	Close(fh);
	fprintf(fpgrab,"\n(Slut på text %d av %s)\n",text,getusername(atoi(brevgrab.from)));
	fprintf(fpgrab,">>>>>");
	return(0);
}

int grabkom(FILE *fp) {
  long kom[MAXKOM];
  int grabret, x=0, confId;
  memcpy(kom,grabhead.kom_i,MAXKOM*sizeof(long));
  while(kom[x]!=-1 && x<MAXKOM) {
    confId = GetConferenceForText(kom[x]);
    if(confId != -1
       && IsTextUnread(kom[x], CUR_USER_UNREAD)
       && IsMemberConf(confId, inloggad, CURRENT_USER)) {
      if((grabret=grabtext(kom[x],fp))==1) {
        if(grabkom(fp)) return(1);
      }
      else if(grabret==2) return(1);
    }
    x++;
  }
  return(0);
}

void grab(void) {
  int x, y, grabret, unreadText;
  struct Mote *motpek;
  FILE *fp;
  char filnamn[20];
  sprintf(filnamn,"T:Grab%d",nodnr);
  if(!(fp=fopen(filnamn,"w"))) {
    puttekn("\r\n\nKunde inte öppna grabfilen!\r\n",-1);
    return;
  }
  // Turn off inactivity checking somehow
  radcnt=-9999;
  sprintf(outbuffer,"\r\nRensar brevlådan..");
  puttekn(outbuffer,-1);
  y=getnextletter(inloggad);
  for(x=CURRENT_USER->brevpek;x<y;x++) {
    if(grabbrev(x,fp)==2) {
      sprintf(outbuffer,"\r\nFel under vid läsandet/skrivandet av brev %d!\r\n",x);
      puttekn(outbuffer,-1);
      fclose(fp);
      return;
    }
  }
  CURRENT_USER->brevpek=y;
  for(motpek=(struct Mote *)Servermem->mot_list.mlh_Head;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
    if(motpek->status & SUPERHEMLIGT) continue;
    if(!IsMemberConf(motpek->nummer, inloggad, CURRENT_USER)) continue;
    sprintf(outbuffer,"\r\nRensar mötet %s...",motpek->namn);
    puttekn(outbuffer,-1);

    unreadText = 0;
    if(motpek->type==MOTE_ORGINAL) {
      while((unreadText = FindNextUnreadText(unreadText, motpek->nummer,
          CUR_USER_UNREAD)) != -1) {
        if((grabret = grabtext(unreadText, fp)) == 2) {
          sprintf(outbuffer,"\r\nFel under vid läsandet/skrivandet av text %d!\r\n",y);
          puttekn(outbuffer,-1);
          fclose(fp);
          return;
        } else if(grabret && grabkom(fp)) {
          fclose(fp);
          return;
        }
      }
    } else if(motpek->type==MOTE_FIDO) {
      for(unreadText = CUR_USER_UNREAD->lowestPossibleUnreadText[motpek->nummer];unreadText <= motpek->texter; unreadText++) {
        if(grabfidotext(unreadText, motpek, fp)) {
          sprintf(outbuffer,"\r\nFel under vid läsandet/skrivandet av text %d!\r\n",y);
          puttekn(outbuffer,-1);
          fclose(fp);
          return;
        }
      }
      CUR_USER_UNREAD->lowestPossibleUnreadText[motpek->nummer] =
        motpek->texter+1;
    }
  }
  fclose(fp);
  puttekn("\r\n\nPackar...",-1);
  sendautorexx(5);
}
