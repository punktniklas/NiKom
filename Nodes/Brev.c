#include "NiKomCompat.h"
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#ifdef HAVE_PROTO_ALIB_H
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __GNUC__
/* In gcc access() is defined in unistd.h, while SAS/C has the
   prototype in stdio.h */
# include <unistd.h>
#endif
#include <time.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Logging.h"
#include "Terminal.h"
#include "StringUtils.h"
#include "Languages.h"

#include "Brev.h"

#define EKO		1
#define BREVKOM	-1


extern struct System *Servermem;
extern char outbuffer[],*argument,inmat[];
extern int inloggad,nodnr,senast_text_typ,senast_text_nr,senast_text_mote,senast_brev_nr,senast_brev_anv,nu_skrivs;
extern int g_lastKomTextType, g_lastKomTextNr, g_lastKomTextConf;
extern struct Inloggning Statstr;
extern struct MinList edit_list;
extern struct Header readhead;

struct ReadLetter brevread,brevspar;
char crashmail;

static void savefidocopy(struct FidoText *ft, int anv);

int getzone(char *adr) {
	int x;
	for(x=0;adr[x]!=':' && adr[x]!=' ' && adr[x]!=0;x++);
	if(adr[x]==':') return(atoi(adr));
	else return(0);
}

int getnet(char *adr) {
	int x;
	char *pek;
	for(x=0;adr[x]!=':' && adr[x]!=' ' && adr[x]!=0;x++);
	if(adr[x]==':') pek=&adr[x+1];
	else pek=adr;
	return(atoi(pek));
}

int getnode(char *adr) {
	int x;
	for(x=0;adr[x]!='/' && adr[x]!=' ' && adr[x]!=0;x++);
	return(atoi(&adr[x+1]));
}

int getpoint(char *adr) {
	int x;
	for(x=0;adr[x]!='.' && adr[x]!=' ' && adr[x]!=0;x++);
	if(adr[x]=='.') return(atoi(&adr[x+1]));
	else return(0);
}

int brev_kommentera(void) {
  BPTR fh;
  int nummer,editret,anv;
  char *brevpers,filnamn[50];
  if(argument[0]) {
    if(!IzDigit(argument[0])) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_MAIL_NO_SUCH_MAIL));
      return 0;
    }
    nummer = atoi(argument);
    brevpers = hittaefter(argument);
    if(!brevpers[0]) {
      anv = inloggad;
    } else {
      anv = parsenamn(brevpers);
      if(anv == -1) {
        SendString("\n\n\r%s\n\r", CATSTR(MSG_COMMON_NO_SUCH_USER));
        return 0;
      }
    }
    sprintf(filnamn, "NiKom:Users/%d/%d/%d.letter", anv / 100, anv, nummer);
    if(!(fh = Open(filnamn, MODE_OLDFILE))) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_MAIL_NO_SUCH_MAIL));
      return 0;
    }
    readletterheader(fh, &brevread);
    Close(fh);
    if(!strncmp(brevread.systemid, "Fido", 4)) {
      if(anv != inloggad) {
        SendString("\n\n\r%s\n\r", CATSTR(MSG_MAIL_NO_COMMENT_PERM));
        return 0;
      }
    } else {
      if(anv != inloggad && inloggad != atoi(brevread.from)
         && Servermem->inne[nodnr].status < Servermem->cfg->st.brev) {
        SendString("\r\n\n%s\r\n\n", CATSTR(MSG_MAIL_NOT_WRITTEN));
        return 0;
      }
    }
    senast_text_typ = BREV;
    senast_brev_nr = nummer;
    senast_brev_anv = anv;
  }
  if(!strncmp(brevread.systemid, "Fido", 4)) {
    nu_skrivs = BREV_FIDO;
    return(fido_brev(NULL, NULL, NULL));
  } else {
    nu_skrivs = BREV;
    editret = initbrevheader(BREVKOM);
  }
  if(editret == 1) {
    return 1;
  }
  if(editret == 2) {
    return 0;
  }
  if((editret = edittext(NULL)) == 1) {
    return 1;
  } else if(!editret) {
    sparabrev();
  }
  return 0;
}

void brev_lasa(int tnr) {
  int brevanv;
  char *arg2;
  arg2 = hittaefter(argument);
  if(arg2[0]) {
    brevanv = parsenamn(arg2);
    if(brevanv == -1) {
      SendString("\n\n\r%s\n\r", CATSTR(MSG_COMMON_NO_SUCH_USER));
      return;
    }
  } else {
    brevanv = inloggad;
  }
  if(tnr < getfirstletter(brevanv) || tnr >= getnextletter(brevanv)) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_MAIL_NO_SUCH_MAIL));
    return;
  }
  visabrev(tnr, brevanv);
}

int HasUnreadMail(void) {
  BPTR lock;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/%ld.letter", inloggad/100, inloggad,
          Servermem->inne[nodnr].brevpek);
  if((lock = Lock(filename, ACCESS_READ))) {
    UnLock(lock);
    return TRUE;
  }
  return FALSE;
}

void NextMail(void) {
  if(!HasUnreadMail()) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_MAIL_NO_UNREAD));
    return;
  }
  g_lastKomTextType = BREV;
  g_lastKomTextNr = Servermem->inne[nodnr].brevpek;
  g_lastKomTextConf = -1;
  visabrev(Servermem->inne[nodnr].brevpek++, inloggad);
}

int countmail(int user,int brevpek) {
  return(getnextletter(user) - brevpek);
}

void varmail(void) {
  int antal;
  antal = countmail(inloggad,Servermem->inne[nodnr].brevpek);
  SendString("\r\n\n%s\r\n", CATSTR(MSG_MAIL_YOU_ARE));
  if(antal == 0) {
    SendString("%s\r\n\n", CATSTR(MSG_MAIL_NO_UNREAD));
  } else if(antal==1) {
    SendString("%s\r\n\n", CATSTR(MSG_MAIL_ONE_UNREAD));
  } else {
    SendStringCat("%s\r\n\n", CATSTR(MSG_MAIL_ONE_UNREAD), antal);
  }
}

void visabrev(int brev,int anv) {
  BPTR fh;
  int i, length = 0, tillmig = FALSE;
  char filnamn[40], *mottagare, textbuf[100], *vemskrev;

  sprintf(filnamn, "NiKom:Users/%d/%d/%d.letter", anv / 100, anv, brev);
  if(!(fh = Open(filnamn, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Could not open %s when displaying mail.", filnamn);
    DisplayInternalError();
    return;
  }
  readletterheader(fh, &brevread);
  if(!strncmp(brevread.systemid, "Fido", 4)) {
    visafidobrev(&brevread, fh, brev, anv);
    return;
  }
  mottagare = brevread.to;
  while(mottagare[0]) {
    if(inloggad == atoi(mottagare)) {
      tillmig = TRUE;
      break;
    }
    mottagare = hittaefter(mottagare);
  }
  if(!tillmig && inloggad != atoi(brevread.from)
     && Servermem->inne[nodnr].status < Servermem->cfg->st.brev) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_MAIL_NO_READ_PERM));
    return;
  }
  Servermem->inne[nodnr].read++;
  Servermem->info.lasta++;
  Statstr.read++;

  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_MAIL_LINE1), brev, getusername(anv));
  if(!strncmp(brevread.systemid, "NiKom", 5)) {
    SendStringCat("%s\n\r", CATSTR(MSG_MAIL_LOCAL_MAIL), brevread.date);
  } else {
    SendStringCat("%s\n\r", CATSTR(MSG_MAIL_UNKNOWN_MAIL), brevread.date);
  }
  SendStringCat("%s\r\n", CATSTR(MSG_MAIL_FROM), getusername(atoi(brevread.from)));
  if(brevread.reply[0]) {
    vemskrev = hittaefter(hittaefter(brevread.reply));
    SendStringCat("%s\r\n", CATSTR(MSG_MAIL_COMMENT), getusername(atoi(vemskrev)));
  }
  mottagare = brevread.to;
  while(mottagare[0]) {
    SendStringCat("%s\n\r", CATSTR(MSG_MAIL_TO), getusername(atoi(mottagare)));
    mottagare = hittaefter(mottagare);
  }
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_SUBJECT), brevread.subject);
  if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
    length = strlen(outbuffer);
    for(i = 0; i < length - 2; i++) {
      textbuf[i]='-';
    }
    textbuf[i] = 0;
    SendString("%s\r\n\n", textbuf);
  } else {
    SendString("\n");
  }
  
  while(FGets(fh, textbuf, 99)) {
    if(SendString("%s\r", textbuf)) {
      break;
    }
  }
  Close(fh);
  SendStringCat("\r\n%s\r\n", CATSTR(MSG_MAIL_END), brev,getusername(atoi(brevread.from)));
  senast_text_typ=BREV;
  senast_brev_nr=brev;
  senast_brev_anv=anv;
}

void visafidobrev(struct ReadLetter *brevread, BPTR fh, int brev, int anv) {
  int length, i;
  char textbuf[100];
  if(anv != inloggad && Servermem->inne[nodnr].status < Servermem->cfg->st.brev) {
    SendString("\n\n\rDu har inte rätt att läsa det brevet!\n\r");
    return;
  }
  Servermem->inne[nodnr].read++;
  Servermem->info.lasta++;
  Statstr.read++;
  SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_MAIL_LINE1), brev, getusername(anv));
  SendStringCat("%s\n\r", CATSTR(MSG_MAIL_FIDO_MAIL), brevread->date);
  SendStringCat("%s\r\n", CATSTR(MSG_MAIL_FROM), brevread->from);
  SendStringCat("%s\n\r", CATSTR(MSG_MAIL_TO), brevread->to);
  SendStringCat("%s\r\n", CATSTR(MSG_ORG_TEXT_SUBJECT), brevread->subject);
  if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
    length = strlen(outbuffer);
    for(i = 0; i < length - 2; i++) {
      textbuf[i] = '-';
    }
    textbuf[i] = 0;
    SendString("%s\r\n\n", textbuf);
  } else {
    SendString("\n");
  }
  
  while(FGets(fh, textbuf, 99)) {
    if(textbuf[0] == 1) {
      if(!(Servermem->inne[nodnr].flaggor & SHOWKLUDGE)) {
        continue;
      }
      if(SendString("^A%s\r", textbuf)) {
        break;
      }
    } else {
      if(SendString("%s\r", textbuf)) {
        break;
      }
    }
  }
  Close(fh);
  SendStringCat("\r\n%s\r\n", CATSTR(MSG_MAIL_END), brev,brevread->from);
  senast_text_typ = BREV;
  senast_brev_nr = brev;
  senast_brev_anv = anv;
}

int recisthere(char *str,int rec) {
  char *pek=str;
  while(pek[0]) {
    if(rec == atoi(pek)) {
      return 1;
    }
    pek = hittaefter(pek);
  }
  return 0;
}

int initbrevheader(int tillpers) {
  int length=0,i=0,lappnr;
  long tid,tempmott;
  struct tm *ts;
  struct User usr;
  char filnamn[40],*mottagare,tempbuf[100],*vemskrev;

  Servermem->action[nodnr] = SKRIVER;
  Servermem->varmote[nodnr] = -1;
  memset(&brevspar, 0, sizeof(struct ReadLetter));
  if(tillpers == -1) {
    strcpy(brevspar.to, brevread.from);
    mottagare = brevread.to;
    while(mottagare[0]) {
      tempmott = atoi(mottagare);
      if(tempmott == inloggad || recisthere(brevspar.to, tempmott)) {
        mottagare = hittaefter(mottagare);
        continue;
      }
      sprintf(tempbuf, " %ld", tempmott);
      strcat(brevspar.to, tempbuf);
      mottagare = hittaefter(mottagare);
    }
    sprintf(brevspar.reply, "%d %d %s", senast_brev_anv, senast_brev_nr, brevread.from);
  } else {
    sprintf(brevspar.to, "%d", tillpers);
  }
  sprintf(brevspar.from, "%d", inloggad);
  readuser(atoi(brevspar.to), &usr);
  if(usr.flaggor & LAPPBREV) {
    SendString("\r\n\n");
    lappnr = atoi(brevspar.to);
    sprintf(filnamn, "NiKom:Users/%d/%d/Lapp", lappnr / 100, lappnr);
    if(!access(filnamn, 0)) {
      sendfile(filnamn);
    }
    SendString("\r\n");
  }
  time(&tid);
  ts = localtime(&tid);
  sprintf(brevspar.date, "%02d%02d%02d %02d:%02d", ts->tm_year % 100,
          ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min);
  strcpy(brevspar.systemid, "NiKom");
  SendString("\r\n\n%s\r\n", CATSTR(MSG_MAIL_MAILBOX));
  SendStringCat("%s\n\r", CATSTR(MSG_MAIL_LOCAL_MAIL), brevspar.date);
  SendStringCat("%s\r\n", CATSTR(MSG_MAIL_FROM), getusername(inloggad));
  if(brevspar.reply[0]) {
    vemskrev = hittaefter(hittaefter(brevspar.reply));
    SendStringCat("%s\r\n", CATSTR(MSG_MAIL_COMMENT), getusername(atoi(vemskrev)));
  }
  mottagare = brevspar.to;
  while(mottagare[0]) {
    SendStringCat("%s\n\r", CATSTR(MSG_MAIL_TO), getusername(atoi(mottagare)));
    mottagare = hittaefter(mottagare);
  }
  SendString("%s ", CATSTR(MSG_WRITE_SUBJECT));
  if(tillpers == -1) {
    strcpy(brevspar.subject, brevread.subject);
    SendString(brevspar.subject);
  } else {
    if(getstring(EKO,40,NULL)) {
      return 1;
    }
    if(!inmat[0]) {
      SendString("\n");
      return 2;
    }
    strcpy(brevspar.subject,inmat);
  }
  SendString("\r\n");
  if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
    length = strlen(brevspar.subject);
    for(i = 0; i < length + 8; i++) {
      tempbuf[i]='-';
    }
    tempbuf[i]=0;
    SendString("%s\r\n\n", tempbuf);
  } else {
    SendString("\n");
  }
  return(0);
}

void sprattgok(char *str) {
  int i;
  for(i=0;str[i];i++) {
    switch(str[i]) {
    case 'å' : str[i] = 'a'; break;
    case 'ä' : str[i] = 'a'; break;
    case 'ö' : str[i] = 'o'; break;
    case 'Å' : str[i] = 'A'; break;
    case 'Ä' : str[i] = 'A'; break;
    case 'Ö' : str[i] = 'O'; break;
    }
  }
}

int fido_brev(char *tillpers,char *adr,struct Mote *motpek) {
  int length = 0, i = 0, editret, chrs=CHRS_LATIN1, inputch, wantCopy, textId;
  struct FidoDomain *fd;
  struct FidoText *komft,ft;
  struct MinNode *first, *last;
  char *foo, tmpfrom[100], fullpath[100], filnamn[20], subject[80], msgid[50];

  if(!(Servermem->inne[nodnr].grupper & Servermem->cfg->fidoConfig.mailgroups)
     && Servermem->inne[nodnr].status < Servermem->cfg->fidoConfig.mailstatus) {
    SendString("\n\n\rDu har ingen rätt att skicka FidoNet NetMail.\n\r");
    return 0;
  }
  Servermem->action[nodnr] = SKRIVER;
  Servermem->varmote[nodnr] = -1;
  memset(&ft, 0, sizeof(struct FidoText));
  if(!tillpers) { /* Det handlar om en kommentar */
    if(motpek) { /* Det är en personlig kommentar */
      strcpy(fullpath, motpek->dir);
      sprintf(filnamn, "%ld.msg", senast_text_nr - motpek->renumber_offset);
      AddPart(fullpath,filnamn, 99);
      komft = ReadFidoTextTags(fullpath, RFT_HeaderOnly, TRUE,TAG_DONE);
      if(!komft) {
        LogEvent(SYSTEM_LOG, ERROR, "Can't read fido text %s.", fullpath);
        DisplayInternalError();
        return 0;
      }
      strcpy(ft.touser, komft->fromuser);
      ft.tozone = komft->fromzone;
      ft.tonet = komft->fromnet;
      ft.tonode = komft->fromnode;
      ft.topoint = komft->frompoint;
      strcpy(subject, komft->subject);
      strcpy(msgid, komft->msgid);
      FreeFidoText(komft);
    } else { /* Det är en kommentar av ett brev */
      strcpy(tmpfrom, brevread.from);
      foo = strchr(tmpfrom, '(');
      if(!foo) {
        LogEvent(SYSTEM_LOG, ERROR,
                 "Trying to comment a Fido netmail that is missing an address. (%s)", tmpfrom);
        DisplayInternalError();
        return 0;
      }
      *(foo - 1) = '\0';
      foo++;
      strcpy(ft.touser, tmpfrom);
      ft.tozone = getzone(foo);
      ft.tonet = getnet(foo);
      ft.tonode = getnode(foo);
      ft.topoint = getpoint(foo);
      strcpy(subject, brevread.subject);
      strcpy(msgid, brevread.messageid);
      
    }
  } else { /* Det är ett helt nytt brev */
    strcpy(ft.touser, tillpers);
    sprattgok(ft.touser);
    ft.tozone = getzone(adr);
    ft.tonet = getnet(adr);
    ft.tonode = getnode(adr);
    ft.topoint = getpoint(adr);
  }
  fd = getfidodomain(0, ft.tozone);
  if(!fd) {
    SendStringCat("\n\n\r%s\n\r", CATSTR(MSG_MAIL_FIDO_BAD_ZONE), ft.tozone);
    return 0;
  }
  if(!tillpers && !motpek) {
    foo = strchr(brevread.to, '(') + 1;
    ft.fromzone = getzone(foo);
    ft.fromnet = getnet(foo);
    ft.fromnode = getnode(foo);
    ft.frompoint = getpoint(foo);
  } else {
    ft.fromzone = fd->zone;
    ft.fromnet = fd->net;
    ft.fromnode = fd->node;
    ft.frompoint = fd->point;
  }
  ft.attribut = FIDOT_PRIVATE | FIDOT_LOCAL;
  makefidousername(ft.fromuser, inloggad);
  makefidodate(ft.date);
  SendString("\r\n%s\r\n", CATSTR(MSG_MAIL_MAILBOX));
  SendStringCat("%s\n\r", CATSTR(MSG_MAIL_FIDO_MAIL), ft.date);
  SendString(CATSTR(MSG_MAIL_FROM), ft.fromuser);
  SendString(" (%d:%d/%d.%d)\r\n", ft.fromzone, ft.fromnet, ft.fromnode, ft.frompoint);
  SendString(CATSTR(MSG_MAIL_TO), ft.touser);
  SendString(" (%d:%d/%d.%d)\n\r", ft.tozone, ft.tonet, ft.tonode, ft.topoint);
  SendString("%s ", CATSTR(MSG_WRITE_SUBJECT));
  if(!tillpers) {
    if(!strncmp(subject, "Re:", 3)) {
      strcpy(ft.subject,subject);
    } else {
      sprintf(ft.subject,"Re: %s",subject);
    }
    SendString(ft.subject);
  } else {
    if(getstring(EKO,40,NULL)) {
      return 1;
    }
    if(!inmat[0]) {
      SendString("\n");
      return 0;
    }
    strcpy(ft.subject, inmat);
  }
  SendString("\r\n");
  if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
    length = strlen(ft.subject);
    for(i = 0; i < length + 8; i++) {
      tmpfrom[i] = '-';
    }
    tmpfrom[i] = 0;
    SendString("%s\r\n\n", tmpfrom);
  } else {
    SendString("\n");
  }
  crashmail = FALSE;
  editret = edittext(NULL);
  if(editret == 1) {
    return 1;
  }
  if(editret == 2) {
    return 0;
  }
  if(crashmail) {
    ft.attribut |= FIDOT_CRASH;
  }
  Servermem->inne[nodnr].skrivit++;
  Servermem->info.skrivna++;
  Statstr.write++;
  SendString("\n\n\r%s\r\n\n", CATSTR(MSG_MAIL_FIDO_WHAT_CHARSET));
  SendString("1: ISO Latin 8-bitars tecken (Default)\n\r");
  SendString("2: IBM PC 8-bitars tecken\r\n");
  SendString("3: Macintosh 8-bitars tecken\r\n");
  SendString("4: Svenska 7-bitars tecken\r\n\n");
  SendString("%s ", CATSTR(MSG_COMMON_CHOICE));
  for(;;) {
    inputch = GetChar();
    if(inputch == GETCHAR_LOGOUT) {
      return 1;
    }
    if(inputch == GETCHAR_RETURN) {
      inputch = '1';
    }
    if(inputch >= '1' && inputch <= '4') {
      break;
    }
  }
  SendString("%c\r\n\n", inputch);
  switch(inputch) {
  case '1' : chrs=CHRS_LATIN1; break;
  case '2' : chrs=CHRS_CP437; break;
  case '3' : chrs=CHRS_MAC; break;
  case '4' : chrs=CHRS_SIS7; break;
  }
  NewList((struct List *)&ft.text);
  first =  edit_list.mlh_Head;
  last = edit_list.mlh_TailPred;
  ft.text.mlh_Head = first;
  ft.text.mlh_TailPred = last;
  last->mln_Succ = (struct MinNode *)&ft.text.mlh_Tail;
  first->mln_Pred = (struct MinNode *)&ft.text;
  if(tillpers) {
    textId = WriteFidoTextTags(&ft,WFT_MailDir,Servermem->cfg->fidoConfig.matrixdir,
                               WFT_Domain,fd->domain,
                               WFT_CharSet,chrs,
                               TAG_DONE);
  } else {
    textId = WriteFidoTextTags(&ft,WFT_MailDir,Servermem->cfg->fidoConfig.matrixdir,
                               WFT_Domain,fd->domain,
                               WFT_Reply,msgid,
                               WFT_CharSet,chrs,
                               TAG_DONE);
  }
  SendString("%s\r\n\n", CATSTR(MSG_MAIL_GOT_NUMBER), textId);
  if(Servermem->cfg->logmask & LOG_BREV) {
    LogEvent(USAGE_LOG, INFO, "%s skickar brev %d till %s (%d:%d/%d.%d)",
             getusername(inloggad), textId,
             ft.touser, ft.tozone, ft.tonet, ft.tonode, ft.topoint);
  }
  
  if(GetYesOrNo(NULL, CATSTR(MSG_MAIL_FIDO_WANT_COPY),
                NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n\n",
                FALSE, &wantCopy)) {
    return 1;
  }
  if(wantCopy) {
    savefidocopy(&ft, inloggad);
  }
  
  while((first=(struct MinNode *)RemHead((struct List *)&ft.text))) {
    FreeMem(first,sizeof(struct EditLine));
  }
  NewList((struct List *)&edit_list);
  return 0;
}

int updatenextletter(int user) {
  BPTR fh;
  long nr;
  char nrstr[20],filnamn[50];
  sprintf(filnamn,"NiKom:Users/%d/%d/.nextletter",user/100,user);
  if(!(fh=Open(filnamn,MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open %s", filnamn);
    DisplayInternalError();
    return -1;
  }
  memset(nrstr,0,20);
  if(Read(fh,nrstr,19)==-1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not read from %s", filnamn);
    DisplayInternalError();
    Close(fh);
    return -1;
  }
  nr=atoi(nrstr);
  sprintf(nrstr,"%ld",nr+1);
  if(Seek(fh,0,OFFSET_BEGINNING)==-1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not rewind %s", filnamn);
    DisplayInternalError();
    Close(fh);
    return -1;
  }
  if(Write(fh,nrstr,strlen(nrstr))==-1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not write to %s", filnamn);
    DisplayInternalError();
    Close(fh);
    return -1;
  }
  Close(fh);
  return(nr);
}

void sparabrev(void) {
  BPTR fh, lock = 0;
  struct EditLine *el;
  char buf[100], orgfilename[50], *motstr;
  int userId, nr, mot;

  Servermem->inne[nodnr].skrivit++;
  Servermem->info.skrivna++;
  Statstr.write++;
  userId = atoi(brevspar.to);
  if((nr = updatenextletter(userId)) == -1) {
    freeeditlist();
    return;
  }
  sprintf(orgfilename, "NiKom:Users/%d/%d/%d.letter", userId/100, userId,nr);
  if(!(fh = Open(orgfilename, MODE_NEWFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not create new file %s", orgfilename);
    DisplayInternalError();
    freeeditlist();
    return;
  }
  strcpy(buf, "System-ID: NiKom\n");
  FPuts(fh, buf);
  sprintf(buf, "From: %d\n", inloggad);
  FPuts(fh, buf);
  sprintf(buf, "To: %s\n", brevspar.to);
  FPuts(fh, buf);
  if(brevspar.reply[0]) {
    sprintf(buf, "Reply: %s\n", brevspar.reply);
    FPuts(fh, buf);
  }
  sprintf(buf, "Date: %s\n", brevspar.date);
  FPuts(fh, buf);
  sprintf(buf, "Subject: %s\n", brevspar.subject);
  FPuts(fh, buf);
  ITER_EL(el, edit_list, line_node, struct EditLine *) {
    if(FPuts(fh, el->text) == -1) {
      LogEvent(SYSTEM_LOG, WARN, "Error writing to %s", orgfilename);
      DisplayInternalError();
      break;
    }
    FPutC(fh,'\n');
  }
  Close(fh);
  freeeditlist();
  SendStringCat("\r\n%s\r\n", CATSTR(MSG_MAIL_GOT_NUMBER_AT_USER), nr, getusername(userId));
  if(Servermem->cfg->logmask & LOG_BREV) {
    strcpy(buf, getusername(inloggad));
    LogEvent(USAGE_LOG, INFO, "%s skickar brev %d till %s",
             buf, nr, getusername(userId));
  }
  motstr = hittaefter(brevspar.to);
  if(motstr[0]) {
    if(!(lock = Lock(orgfilename,ACCESS_READ))) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not Lock() %s", orgfilename);
      DisplayInternalError();
      return;
    }
  }
  while(motstr[0]) {
    mot = atoi(motstr);
    if((nr = updatenextletter(mot)) == -1) {
      UnLock(lock);
      return;
    }
    sprintf(buf,"NiKom:Users/%d/%d/%d.letter",mot/100,mot,nr);
    if(!MakeLink(buf, lock, FALSE)) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not create link %s", buf);
      DisplayInternalError();
      UnLock(lock);
      return;
    }
    SendStringCat("\r\n%s\r\n", CATSTR(MSG_MAIL_GOT_NUMBER_AT_USER), nr, getusername(userId));
    if(Servermem->cfg->logmask & LOG_BREV) {
      strcpy(buf, getusername(inloggad));
      LogEvent(USAGE_LOG, INFO, "%s skickar brev %d till %s",
               buf, nr, getusername(mot));
    }
    motstr = hittaefter(motstr);
  }
  if(lock) {
    UnLock(lock);
  }
}

static void savefidocopy(struct FidoText *ft, int anv) {
  struct FidoLine *fl;
  BPTR fh;
  int nummer;
  char *foo,buffer[100];

  nummer = updatenextletter(anv);
  if(nummer == -1) {
    return;
  }
  sprintf(buffer, "NiKom:Users/%d/%d/%d.letter", anv/100, anv, nummer);
  if(!(fh = Open(buffer,MODE_NEWFILE))) {
    return;
  }
  FPuts(fh, "System-ID: Fido\n");
  sprintf(buffer, "From: %s (%d:%d/%d.%d)\n",
          ft->fromuser, ft->fromzone, ft->fromnet, ft->fromnode, ft->frompoint);
  FPuts(fh,buffer);
  sprintf(buffer, "To: %s (%d:%d/%d.%d)\n",
          ft->touser, ft->tozone, ft->tonet, ft->tonode, ft->topoint);
  FPuts(fh,buffer);
  sprintf(buffer, "Date: %s\n", ft->date);
  FPuts(fh, buffer);
  foo = hittaefter(ft->subject);
  if(ft->subject[0] == 0 || (ft->subject[0] == ' ' && foo[0] == 0)) {
    strcpy(buffer,"Subject: -\n");
  } else {
    sprintf(buffer, "Subject: %s\n", ft->subject);
  }
  FPuts(fh,buffer);
  ITER_EL(fl, ft->text, line_node, struct FidoLine *) {
    FPuts(fh, fl->text);
    FPutC(fh, '\n');
  }
  Close(fh);
  SendStringCat("\n\n\r%s\r\n\n", CATSTR(MSG_MAIL_GOT_NUMBER), nummer);
}

int skickabrev(void) {
  int pers=0, editret;
  char *adr;
  if(!(adr = strchr(argument, '@'))) {
    if((pers = parsenamn(argument)) == -1) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
      return 0;
    } else if(pers == -3) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_MAIL_LOCAL_SYNTAX));
      return 0;
    }
    nu_skrivs = BREV;
  } else {
    nu_skrivs = BREV_FIDO;
    *adr = '\0';
    adr++;
    if(!getzone(adr) || !getnet(adr)) {
      SendString("\n\n\r%s\n\r", CATSTR(MSG_MAIL_FIDO_SYNTAX));
      return 0;
    }
  }
  if(nu_skrivs == BREV) {
    editret=initbrevheader(pers);
  } else if(nu_skrivs==BREV_FIDO) {
    return fido_brev(argument,adr,NULL);
  } else {
    return 0;
  }
  if(editret == 1) {
    return 1;
  }
  if(editret == 2) {
    return 0;
  }
  if((editret = edittext(NULL)) == 1) {
    return 1;
  }
  if(!editret) {
    sparabrev();
  }
  return 0;
}

void initpersheader(void) {
  long tid, lappnr, length, i;
  struct tm *ts;
  struct User usr;
  char filnamn[40], buf[100];

  Servermem->action[nodnr] = SKRIVER;
  Servermem->varmote[nodnr] = -1;
  memset(&brevspar,0,sizeof(struct ReadLetter));
  sprintf(brevspar.to, "%ld", readhead.person);
  sprintf(brevspar.from, "%d", inloggad);
  readuser(readhead.person, &usr);
  if(usr.flaggor & LAPPBREV) {
    SendString("\r\n\n");
    lappnr = atoi(brevspar.to);
    sprintf(filnamn, "NiKom:Users/%ld/%ld/Lapp", lappnr/100, lappnr);
    if(!access(filnamn, 0)) {
      sendfile(filnamn);
    }
    SendString("\r\n");
  }
  time(&tid);
  ts = localtime(&tid);
  sprintf(brevspar.date,"%02d%02d%02d %02d:%02d", ts->tm_year % 100,
          ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min);
  strcpy(brevspar.systemid, "NiKom");
  SendString("\r\n\n%s\r\n", CATSTR(MSG_MAIL_MAILBOX));
  SendStringCat("%s\n\r", CATSTR(MSG_MAIL_LOCAL_MAIL), brevspar.date);
  SendStringCat("%s\r\n", CATSTR(MSG_MAIL_FROM), getusername(inloggad));
  SendStringCat("%s\n\r", CATSTR(MSG_MAIL_TO), getusername(readhead.person));
  strcpy(brevspar.subject, readhead.arende);
  SendStringCat("%s\n\r", CATSTR(MSG_ORG_TEXT_SUBJECT), brevspar.subject);
  if(Servermem->inne[nodnr].flaggor & STRECKRAD) {
    length = strlen(brevspar.subject);
    for(i = 0; i < length + 8; i++) buf[i] = '-';
    buf[i] = '\0';
    SendString("%s\r\n\n", buf);
  } else {
    SendString("\n");
  }
}

void listabrev(void) {
  BPTR fh;
  int anv, i, first, sendRet;
  char filnamn[50], namn[50];
  struct ReadLetter listhead;
  if(argument[0]) {
    anv = parsenamn(argument);
    if(anv == -1) {
      SendString("\n\n\r%s\n\r", CATSTR(MSG_COMMON_NO_SUCH_USER));
      return;
    }
  } else {
    anv = inloggad;
  }
  first = getfirstletter(anv);
  i = getnextletter(anv) - 1;
  SendStringCat("\n\n\r%s\n\n\r", CATSTR(MSG_MAIL_LIST_SUMMARY), first, i);
  SendString("%s\n\r", CATSTR(MSG_MAIL_LIST_HEAD));
  SendString("-------------------------------------------------------------------------\r\n");
  for(; i >= first ; i--) {
    sprintf(filnamn,"NiKom:Users/%d/%d/%d.letter", anv / 100, anv, i);
    if(!(fh = Open(filnamn, MODE_OLDFILE))) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not open %s", filnamn);
      DisplayInternalError();
      return;
    }
    if(readletterheader(fh,&listhead)) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not read %s", filnamn);
      DisplayInternalError();
      Close(fh);
      return;
    }
    Close(fh);
    listhead.subject[26] = '\0';
    listhead.date[6] = '\0';
    if(!strcmp(listhead.systemid, "NiKom")) {
      if(anv != inloggad && inloggad != atoi(listhead.from)) {
        continue;
      }
      strcpy(namn, getusername(atoi(listhead.from)));
      sendRet = SendString("%-34s%5d %s %s\r\n", namn, i, listhead.date, listhead.subject);
    } else if(!strcmp(listhead.systemid, "Fido")) {
      if(anv != inloggad) {
        continue;
      }
      sendRet = SendString("%-34s%5d %s %s\r\n", listhead.from, i, listhead.date, listhead.subject);
    } else {
      sendRet = SendString("%s\r\n", CATSTR(MSG_MAIL_LIST_UNKNOWN), i);
    }
    if(sendRet) {
      return;
    }
  }
}

void rensabrev(void) {
  BPTR fh;
  int first, next, antal, i;
  char filnamn[50], nrbuf[20];

  first = getfirstletter(inloggad);
  next = getnextletter(inloggad);
  if(!argument[0]) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_MAIL_PURGE_SYNTAX));
    return;
  }
  antal = atoi(argument);
  if(antal <= 0) {
    SendString("\n\n\r%s\n\r", CATSTR(MSG_MAIL_PURGE_ABOVE_ZERO));
    return;
  }
  if(first + antal > next) {
    SendStringCat("\n\n\r%s\n\r", CATSTR(MSG_MAIL_PURGE_ONLY_X_MAIL), next - first);
    return;
  }
  SendStringCat("\n\n\r%s\n\r", CATSTR(MSG_MAIL_PURGE_SUMMARY), first, next - 1, antal);
  SendString("%s\n\r", CATSTR(MSG_MAIL_PURGE_PURGING));
  for(i = first; i < first + antal; i++) {
    if(!(i % 10)) {
      SendString("%d\r", i);
    }
    sprintf(filnamn, "NiKom:Users/%d/%d/%d.letter", inloggad / 100, inloggad, i);
    if(!DeleteFile(filnamn)) {
      LogEvent(SYSTEM_LOG, ERROR, "Could not delete %s", filnamn);
      DisplayInternalError();
    }
  }
  sprintf(filnamn, "NiKom:Users/%d/%d/.firstletter", inloggad / 100, inloggad);
  if(!(fh = Open(filnamn, MODE_NEWFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open %s", filnamn);
    DisplayInternalError();
    return;
  }
  sprintf(nrbuf, "%d", first + antal);
  if(Write(fh, nrbuf, strlen(nrbuf)) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not write %s", filnamn);
    DisplayInternalError();
  }
  Close(fh);
  SendStringCat("\n\r%s\n\r", CATSTR(MSG_MAIL_PURGE_RESULT), antal);
  if((first + antal) > Servermem->inne[nodnr].brevpek) {
    Servermem->inne[nodnr].brevpek = first + antal;
  }
}

int readletterheader(BPTR fh,struct ReadLetter *rl) {
  int len;
  char buffer[100];
  memset(rl,0,sizeof(struct ReadLetter));
  while(!rl->subject[0]) {
    if(!FGets(fh,buffer,99)) return(1);
    len=strlen(buffer);
    if(buffer[len-1]=='\n') buffer[len-1]=0;
    if(!strncmp(buffer,"System-ID:",10)) strcpy(rl->systemid,&buffer[11]);
    if(!strncmp(buffer,"From:",5)) strcpy(rl->from,&buffer[6]);
    if(!strncmp(buffer,"To:",3)) strcpy(rl->to,&buffer[4]);
    if(!strncmp(buffer,"Message-ID:",11)) strcpy(rl->messageid,&buffer[12]);
    if(!strncmp(buffer,"Reply:",6)) strcpy(rl->reply,&buffer[7]);
    if(!strncmp(buffer,"Date:",5)) strcpy(rl->date,&buffer[6]);
    if(!strncmp(buffer,"Subject:",8)) strcpy(rl->subject,&buffer[9]);
  }
  if(!rl->systemid[0]) return(1);
  return(0);
}
