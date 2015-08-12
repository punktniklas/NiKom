#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include "NiKomStr.h"
#include "Serverfuncs.h"
#include "NiKomLib.h"
#include "nikom_protos.h"
#include "nikom_pragmas.h"
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern struct System *Servermem;

void rxsendnodemess(struct RexxMsg *mess) {
	int avs, nodnr, ret;
	char *str, retstr[5];
	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}
	nodnr = atoi(mess->rm_Args[1]);
	if(nodnr != -1 && (nodnr < 0 || nodnr >= MAXNOD) && Servermem->nodtyp[nodnr] == 0) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	avs = atoi(mess->rm_Args[2]);
	if(avs != -1 && !userexists(avs)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	str = mess->rm_Args[3];
	if(strlen(str) > MAXSAYTKN-1) str[MAXSAYTKN-1] = 0;
	ret = SendNodeMessage(nodnr, avs, str);
	sprintf(retstr, "%d",ret - 1);
	mess->rm_Result1=0;
	if(!(mess->rm_Result2=(long)CreateArgstring(retstr,strlen(retstr))))
		printf("Kunde inte allokera en ArgString\n");
}

void rexxstatusinfo(struct RexxMsg *mess)
{
	int status;
	char str[100];
	if(!mess->rm_Args[1] || !mess->rm_Args[2]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	status = atoi(mess->rm_Args[1]);
	if(status < 0 || status > 100)
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	switch(mess->rm_Args[2][0])
	{
		case 'i' : case 'I' :
			sprintf(str,"%d",Servermem->cfg.inaktiv[status]);
			break;

		case 'r' : case 'R' :
			sprintf(str,"%d",Servermem->cfg.uldlratio[status]);
			break;

		case 't' : case 'T' :
			sprintf(str,"%d",Servermem->cfg.maxtid[status]);
			break;
		default :
			str[0] = 0;
	}

	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

/*

SystemInfo(status, subjekt)

Returnerar:

-2	Finns inget sådant subjekt.

*/

void rexxsysteminfo(struct RexxMsg *mess)
{
	char str[101];
	if(!mess->rm_Args[1])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	switch(mess->rm_Args[1][0])
	{
		case 'f' : case 'F' :
			sprintf(str,"%d",Servermem->cfg.defaultflags);
			break;

		case 'd' : case 'D' :
			sprintf(str,"%d",Servermem->cfg.diskfree);
			break;

		case 'm' : case 'M' :
			sprintf(str,"%d",Servermem->cfg.logmask);
			break;

		case 'l' : case 'L' :
			sprintf(str,"%d",Servermem->cfg.logintries);
			break;

		case 'r' : case 'R' :
			sprintf(str,"%d",(int) Servermem->cfg.defaultrader);
			break;

		case 'p' : case 'P' :
			sprintf(str,"%d",(int) Servermem->cfg.defaultprotokoll);
			break;

		case 's' : case 'S' :
			sprintf(str,"%d",(int) Servermem->cfg.defaultstatus);
			break;

		case 'b' : case 'B' :
			strcpy(str,Servermem->cfg.brevnamn);
			break;

		case 'n' : case 'N' :
			strcpy(str,Servermem->cfg.ny);
			break;

		case 'u' : case 'U' :
			strcpy(str,Servermem->cfg.ultmp);
			break;

		case 'o' : case 'O' :
			strcpy(str,Servermem->cfg.logfile);
			break;
		default:
			strcpy(str,"-2");
			break;
	}

	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

/*
Arearight(användarnummer, areanr)

Returnerar:
0	Användaren har inga rättigheter i den aktuella arean.
1	Användaren har rättigheter i den aktuella arean.
-1	Om användaren inte finns.
-2	Arean är raderad
-3	Arean finns inte.
*/

void rexxarearight(struct RexxMsg *mess)
{
	int anvnr, areanr, i;
	struct User user;

	if(!mess->rm_Args[1] || !mess->rm_Args[2]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	anvnr = atoi(mess->rm_Args[1]);
	areanr = atoi(mess->rm_Args[2]);

	if(areanr < 0 || areanr >= Servermem->info.areor)
	{
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	if(!Servermem->areor[areanr].namn[0])
	{
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	if(!userexists(anvnr)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	for(i=0;i<MAXNOD;i++)
		if(anvnr==Servermem->inloggad[i]) break;

	if(i<MAXNOD)
	{
		memcpy(&user,&Servermem->inne[i],sizeof(struct User));
	}
	else
	{
		if(readuser(anvnr,&user))
		{
			if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
				printf("Kunde inte allokera en ArgString\n");
			mess->rm_Result1=0;
			return;
		}
	}

	if(arearatt(areanr, anvnr, &user))
	{
		if(!(mess->rm_Result2=(long)CreateArgstring("1",1)))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	else
	{
		if(!(mess->rm_Result2=(long)CreateArgstring("0",1)))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
}

int arearatt(int area, int usrnr, struct User *usr) {
	int x=0;
	if(usr->status>=Servermem->cfg.st.bytarea) return(1);
	if(Servermem->areor[area].mote==-1 || MayBeMemberConf(Servermem->areor[area].mote, usrnr, usr)) x++;
	if(!Servermem->areor[area].grupper || (Servermem->areor[area].grupper & usr->grupper)) x++;
	return(x==2 ? 1 : 0);
}

/*
ChgMeet(mötesnummer, ändra till, subjekt)

Subjekt:

a - skapat av användarnummer
c - Vilken teckenuppsättning som används i mötet. (Endast Fido)
    1 - ISO Latin 1
    2 - IBM CodePage
    3 - SIS-7 (SF7, måsvingar)
    4 - Macintosh
d - Katalog för .msg-filerna (Endast Fido)
f - Mötets tag-namn. (Endast Fido)
g - Vilka grupper som har rättigheter i mötet. (Se UserInfo() för
    beskrivning av formatet)
h - Högsta text i mötet. (Endast Fido)
l - Lägsta text i mötet. (Endast Fido)
m - MAD (mötesadminstratör) för mötet
n - namn på mötet
o - Origin-rad (Endast Fido)
p - Sorteringsprioritet
r - Omnumreringsoffset, dvs differansen mellan tetxnummer i mötet och
    textnummer på .msg-filerna. (Endast Fido)
s - status
   1=Slutet
   2=Skrivskyddat
   4=Kommentarsskyddat
   8=Hemligt
  16=<Reserverad>
  32=Auto
  64=ARexxstyrt
 128=Skrivstyrt

t - skapat tid
y - Mötets typ. 0 - Interna möten
                2 - Fido-möten

Returvärden:

-1	Mötet finns inte.

*/
void rexxchgmeet(struct RexxMsg *mess)
{
	int nummer;
	struct Mote *motpek;

	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	motpek=getmotpek(nummer=atoi(mess->rm_Args[1]));
	if(!motpek) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	switch(mess->rm_Args[3][0]) {
		case 'a' : case 'A' :
			motpek->skapat_av = atoi(mess->rm_Args[2]);
			break;
		case 'c' : case 'C' :
			motpek->charset = atoi(mess->rm_Args[2]);
			break;
		case 'd' : case 'D' :
			strcpy(motpek->dir, mess->rm_Args[2]);
			break;
		case 'f' : case 'F' :
			strcpy(motpek->tagnamn, mess->rm_Args[2]);
			break;
/*		case 'g' : case 'G' :
			sprintf(str,"%d,%d",(unsigned int)motpek->grupper/65536,(unsigned int)motpek->grupper%65536);
			break; */
		case 'h' : case 'H' :
			motpek->texter = atoi(mess->rm_Args[2]);
			break;
/*		case 'l' : case 'L' :
			sprintf(str,"%d",motpek->lowtext);
			break; */
		case 'm' : case 'M' :
			motpek->mad = atoi(mess->rm_Args[2]);
			break;
		case 'n' : case 'N' :
			strcpy(motpek->namn, mess->rm_Args[2]);
			break;
		case 'o' : case 'O' :
			strcpy(motpek->origin, mess->rm_Args[2]);
			break;
		case 'p' : case 'P' :
			motpek->sortpri = atoi(mess->rm_Args[2]);
			break;
/*		case 'r' : case 'R' :
			sprintf(str,"%d",motpek->renumber_offset);
			break; */
		case 's' : case 'S' :
			motpek->status = atoi(mess->rm_Args[2]);
			break;
		case 'y' : case 'Y' :
			motpek->type = atoi(mess->rm_Args[2]);
			break;
		default :

			break;
	}


}

/*
getprogramdata(anvnummer, kategori, typ)
*/

void rexxgetprogramdata(struct RexxMsg *mess)
{
	int anvnummer;
	char buffer[257];

	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	anvnummer=atoi(mess->rm_Args[1]);
	if(!userexists(anvnummer)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	if(GetProgramData(anvnummer, NULL, mess->rm_Args[2], mess->rm_Args[3]))
	{
		strcpy(buffer, GetProgramData(anvnummer, NULL, mess->rm_Args[2], mess->rm_Args[3]));
	}
	else
	{
		buffer[0] = NULL;
	}

	if(!(mess->rm_Result2=(long)CreateArgstring(buffer, strlen(buffer))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
	return;
}

/*
addprogramdata(anvnummer, kategori, typ, data)
*/

void rexxaddprogramdata(struct RexxMsg *mess)
{
	int anvnummer;
	char buffer[257];

	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3] || !mess->rm_Args[4]) {
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	anvnummer=atoi(mess->rm_Args[1]);
	if(!userexists(anvnummer)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
			printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}

	if(AddProgramData(anvnummer, NULL, mess->rm_Args[2], mess->rm_Args[3], mess->rm_Args[4]))
		strcpy(buffer, "0");
	else
		strcpy(buffer, "-2");

	if(!(mess->rm_Result2=(long)CreateArgstring(buffer, strlen(buffer))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
	return;
}


void rexxmarktextread(struct RexxMsg *mess)
{
	int textnr, anvnr, error;
	char retur[10];

	if(!mess->rm_Args[1] || !mess->rm_Args[2])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	anvnr = atoi(mess->rm_Args[1]);
	textnr = atoi(mess->rm_Args[2]);
	error = -6; /* MarkTextRead(anvnr, textnr); */
	sprintf(retur,"%d", error);

	if(!(mess->rm_Result2=(long)CreateArgstring(retur, strlen(retur))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
	return;
}

void rexxmarktextunread(struct RexxMsg *mess)
{
	int textnr, anvnr, error;
	char retur[10];

	if(!mess->rm_Args[1] || !mess->rm_Args[2])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	anvnr = atoi(mess->rm_Args[1]);
	textnr = atoi(mess->rm_Args[2]);
	error = -6; /* MarkTextUnRead(anvnr, textnr); */
	sprintf(retur,"%d", error);

	if(!(mess->rm_Result2=(long)CreateArgstring(retur, strlen(retur))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
	return;
}

void rexxconsoletext(struct RexxMsg *mess)
{
	int nodnr, varde;

	if(!mess->rm_Args[1] || !mess->rm_Args[2])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}

	nodnr = atoi(mess->rm_Args[1]);
	varde = atoi(mess->rm_Args[2]);

	Servermem->watchserial[nodnr] = varde;

	if(!(mess->rm_Result2=(long)CreateArgstring("0", 1)))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void rexxcheckuserpassword(struct RexxMsg *mess)
{
	int anvnummer;
	char retvarde[5];

	if(!mess->rm_Args[1] || !mess->rm_Args[2])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	anvnummer = atoi(mess->rm_Args[1]);

	sprintf(retvarde, "%d", CheckPassword(anvnummer, mess->rm_Args[2]));

	if(!(mess->rm_Result2=(long)CreateArgstring(retvarde, strlen(retvarde))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void rexxdumptext(struct RexxMsg *mess)
{
	int x,length, text;
	struct tm *ts;
	struct EditLine *el;

/*	int tnr,x;
	struct Header dumphead;
	FILE *fpr,*fpd;
	char foostr[82],filnamn[40];

	if(!mess->rm_Args[1] || !mess->rm_Args[2])
	{
		mess->rm_Result1=1;
		mess->rm_Result2=NULL;
		return;
	}

	text = atoi(mess->rm_Args[1]);
	if(Servermem->texts[text%MAXTEXTS]==-1)
	{
		return;
	}
	if(readtexthead(text,&readhead)) return(0);
	if(!MayReadConf(readhead.mote, inloggad, &Servermem->inne[nodnr])) {
		puttekn("\r\n\nDu har inte rätt att läsa den texten!\r\n\n",-1);
		return(0);
	}
	ts=localtime(&readhead.tid);
	sprintf(outbuffer,"\r\n\nText %d  Möte: %s    %4d%02d%02d %02d:%02d\r\n",readhead.nummer,getmotnamn(readhead.mote),ts->tm_year + 1900,ts->tm_mon+1,ts->tm_mday,ts->tm_hour,ts->tm_min);
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
	for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ)
	{
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
	} */
}
