#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Terminal.h"

extern int nodnr, rxlinecount;
extern char outbuffer[];
extern struct System *Servermem;

char inmat[1024];     /* Används av getstring() */
int radcnt = 0;

char commandhistory[10][1025];

int checkvalue(char *buffer);

void sendfile(char *filename) {
  FILE *fp;
  char buff[1025];
  if(!(fp=fopen(filename, "r"))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open file %s for reading.", filename);
    SendString("Internal error.\r\n");
    return;
  }
  for(;;) {
    if(!(fgets(buff, 1023, fp))) {
      break;
    }
    if(SendString("%s\r", buff)) {
      break;
    }
  }
  fclose(fp);
}

int getnumber(int *minvarde, int *maxvarde, int *defaultvarde)
{
	int pos = 0, antal = 0, minus = 0, ok;
	char buffer[20];
	UBYTE tillftkn;

	memset(inmat,0,20);
	if(defaultvarde != 0)
	{
		sprintf(inmat, "%d", *defaultvarde);
		putstring(inmat, -1, 0);
		antal = pos = strlen(inmat);
	}

	while((tillftkn=gettekn())!=13)
	{
		ok = 1;
		if(tillftkn==1 && pos) { /* CTRL-A */
			char movebuf[8];
			sprintf(movebuf,"\x1b\x5b%d\x44",pos);
			putstring(movebuf,-1,0);
			pos=0;
		}
		else if(tillftkn==5 && pos<antal) { /* CTRL-E */
			char movebuf[8];
			sprintf(movebuf,"\x1b\x5b%d\x43",antal-pos);
			putstring(movebuf,-1,0);
			pos=antal;
		}
		else if(tillftkn > 47 && tillftkn < 58) /* 0 -> 9 */
		{
			inmat[antal] = 0;
			strcpy(buffer, inmat);
			movmem(&buffer[pos], &buffer[pos+1], antal-pos);
			buffer[pos]=tillftkn;
			buffer[antal+1] = 0;

			if(!checkvalue(buffer))
				ok = 0;
			else
			{
				if(minvarde != 0 && atoi(buffer) < *minvarde)
					ok = 0;
				if((maxvarde != 0 && atoi(buffer) > *maxvarde) || (strlen(buffer) > 10+(buffer[0] == '-')))
					ok = 0;
			}

			if(ok)
			{
				if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) putstring("\x1b\x5b\x31\x40",-1,0);
				eka(tillftkn);
				movmem(&inmat[pos],&inmat[pos+1],antal-pos);
				inmat[pos++]=tillftkn;
				antal++;
			}
			else
				eka('\a');
		}
		else if(tillftkn == '-')
		{
			if(!pos && !minus && (minvarde == 0 || *minvarde < 0))
			{
				if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) putstring("\x1b\x5b\x31\x40",-1,0);
				eka(tillftkn);
				movmem(&inmat[pos],&inmat[pos+1],antal-pos);
				inmat[pos++]=tillftkn;
				antal++;
				minus = 1;
			}
			else
				eka('\a');
		}
		else if(tillftkn==8) /* Radera tecknet framför cursorn */
		{
			if(pos)
			{
				if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) putstring("\x1b\x5b\x44\x1b\x5b\x50",-1,0);
				else putstring("\b \b",-1,0);
				if(pos == 1 && inmat[1] == '-') minus = 0;
				movmem(&inmat[pos],&inmat[pos-1],antal-pos);
				pos--;
				antal--;
			}
			else
				eka('\a');
		}
		else if(tillftkn==127 && (Servermem->inne[nodnr].flaggor & SEKVENSANSI))
		{
			if(pos!=antal)
			{
				putstring("\x1b\x5b\x50",-1,0);
				movmem(&inmat[pos+1],&inmat[pos],antal-pos);
				antal--;
			}
			else
				eka('\a');
		}
		else if((tillftkn=='\x9b'
					|| (tillftkn=='\x1b'
						&& ((tillftkn=gettekn())=='['
							|| tillftkn=='Ä'))))
		{
			if((tillftkn=gettekn())=='\x44' && pos)
			{
				putstring("\x1b\x5b\x44",-1,0);
				pos--;
			} else if(tillftkn=='\x43' && pos<antal)
			{
				putstring("\x1b\x5b\x43",-1,0);
				pos++;
			} else if(tillftkn=='\x41' && (Servermem->inne[nodnr].flaggor & SEKVENSANSI))
			{
				inmat[antal] = 0;
				if(maxvarde == 0 || (maxvarde != 0 && atoi(inmat) < *maxvarde))
				{
					if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a",pos);
					else strcpy(outbuffer,"\x1b\x5b\x4a");
					putstring(outbuffer,-1,0);
					sprintf(inmat,"%d",atoi(inmat)+1);
					antal = pos = strlen(inmat);
					putstring(inmat,-1,0);
				}
				else
					eka('\a');
			}
			else if(tillftkn=='\x42' && (Servermem->inne[nodnr].flaggor & SEKVENSANSI))
			{
				inmat[antal] = 0;
				if(maxvarde == 0 || (maxvarde != 0 && atoi(inmat) > *minvarde))
				{
					if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a",pos);
					else strcpy(outbuffer,"\x1b\x5b\x4a");
					putstring(outbuffer,-1,0);
					sprintf(inmat,"%d",atoi(inmat)-1);
					antal = pos = strlen(inmat);
					putstring(inmat,-1,0);
				}
				else
					eka('\a');
			}
		}
		else if(tillftkn==10)
		{
			if(carrierdropped()) return(1);
		}
		else if(tillftkn==24 && (Servermem->inne[nodnr].flaggor & SEKVENSANSI))
		{
			if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a",pos);
			else strcpy(outbuffer,"\x1b\x5b\x4a");
			putstring(outbuffer,-1,0);
			memset(inmat,0,20);
			pos=antal=0;
		}
		else
			eka('\a');
	}

	inmat[antal] = 0;
	return atoi(inmat);
}

int checkvalue(char *buffer)
{
	int i = 0, minok = 1, maxok = 1;
	char *MAX = "2147483647", *MIN = "2147483646";

	if(buffer[0] == '-') i = 1;

	if(strlen(buffer) == 10 || (i == 1 && strlen(buffer) == 11))
	{
		while(i < 10+(buffer[0] == '-') && minok && maxok)
		{
			if(buffer[0] == '-')
			{
				if(buffer[i] > MIN[i-1])
						minok = 0;
			}
			else
			{
				if(buffer[i] > MAX[i])
					maxok = 0;
			}
			i++;
		}
	}

	if(!minok) printf("-%s not OK\n", buffer);
	if(!maxok) printf("+%s not OK\n", buffer);

	return maxok && minok;
}

int getstring(int eko,int maxtkn, char *defaultstring) {
	int antal=0,pos=0,x,modified=FALSE;
	UBYTE tillftkn;
	static int historycnt=-1;
	radcnt=0;

	memset(inmat,0,1023);
	if(defaultstring != NULL)
	{
		antal = strlen(defaultstring);
		pos = strlen(defaultstring);
		putstring(defaultstring,-1,0);
		strcpy(inmat,defaultstring);
	}

	while((tillftkn=gettekn())!=13)
	{
		if(tillftkn==1 && pos) { /* CTRL-A */
			char movebuf[8];
			sprintf(movebuf,"\x1b\x5b%d\x44",pos);
			putstring(movebuf,-1,0);
			pos=0;
		} else if(tillftkn==5 && pos<antal) { /* CTRL-E */
			char movebuf[8];
			sprintf(movebuf,"\x1b\x5b%d\x43",antal-pos);
			putstring(movebuf,-1,0);
			pos=antal;
		}

		if((tillftkn>31 && tillftkn<127) || (tillftkn>159 && tillftkn<=255)) {
			if(antal<maxtkn) {
				if(eko)
				{
					if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) putstring("\x1b\x5b\x31\x40",-1,0);
					if(eko!=STAREKO)
						eka(tillftkn);
					else
						eka('*');
				}
				movmem(&inmat[pos],&inmat[pos+1],antal-pos);
				inmat[pos++]=tillftkn;
				antal++;
				modified=TRUE;
			} else eka('\a');
		} else if(tillftkn==8) {
			if(pos) {
				if(eko) {
					if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) putstring("\x1b\x5b\x44\x1b\x5b\x50",-1,0);
					else putstring("\b \b",-1,0);
				}
				movmem(&inmat[pos],&inmat[pos-1],antal-pos);
				pos--;
				antal--;
				modified=TRUE;
			}
		} else if(tillftkn==10) {
			if(carrierdropped()) return(1);
		} else if(tillftkn==24 && (Servermem->inne[nodnr].flaggor & SEKVENSANSI))
		{
			if(eko) {
				if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a",pos);
				else strcpy(outbuffer,"\x1b\x5b\x4a");
				putstring(outbuffer,-1,0);
			}
			memset(inmat,0,1023);
			pos=antal=0;
		} else if(tillftkn==127 && (Servermem->inne[nodnr].flaggor & SEKVENSANSI))
		{
			if(pos!=antal) {
				if(eko) putstring("\x1b\x5b\x50",-1,0);
				movmem(&inmat[pos+1],&inmat[pos],antal-pos);
				antal--;
				modified=TRUE;
			}
		} else if(eko
		          && (tillftkn=='\x9b'
		              || (tillftkn=='\x1b'
		                  && ((tillftkn=gettekn())=='['
		                       || tillftkn=='Ä')))) {
			if((tillftkn=gettekn())=='\x44' && pos) {
				putstring("\x1b\x5b\x44",-1,0);
				pos--;
			} else if(tillftkn=='\x43' && pos<antal) {
				putstring("\x1b\x5b\x43",-1,0);
				pos++;
			} else if(tillftkn=='\x41' && historycnt<9 && commandhistory[historycnt+1][0]) {
				historycnt++;
				if(strlen(commandhistory[historycnt]) > maxtkn)
				{
					strncpy(inmat,commandhistory[historycnt], maxtkn);
					inmat[strlen(inmat)] = NULL;
				}
				else
					strcpy(inmat,commandhistory[historycnt]);
				if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a%s",pos,inmat);
				else sprintf(outbuffer,"\x1b\x5b\x4a%s",inmat);
				putstring(outbuffer, -1, 0);
				pos=antal=strlen(inmat);
				modified=FALSE;
			} else if(tillftkn=='\x42' && historycnt>0 && commandhistory[historycnt-1][0]) {
				historycnt--;
				if(strlen(commandhistory[historycnt]) > maxtkn)
				{
					strncpy(inmat,commandhistory[historycnt], maxtkn);
					inmat[strlen(inmat)] = NULL;
				}
				else
					strcpy(inmat,commandhistory[historycnt]);
				if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a%s",pos,inmat);
				else sprintf(outbuffer,"\x1b\x5b\x4a%s",inmat);
				putstring(outbuffer, -1, 0);
				pos=antal=strlen(inmat);
				modified=FALSE;
			} else if(tillftkn=='\x42' && (historycnt==0 || historycnt==-1)) {
				historycnt=-1;
				if(eko==1) {
					if(pos) sprintf(outbuffer,"\x1b\x5b%d\x44\x1b\x5b\x4a",pos);
					else strcpy(outbuffer,"\x1b\x5b\x4a");
					putstring(outbuffer,-1,0);
				}
				memset(inmat,0,1023);
				pos=antal=0;
/* BEGIN SHIFT-CSI */
			}
			else if(tillftkn==' ')
			{
				if((tillftkn=gettekn())=='A' && pos) { /* shift left */
					char movebuf[8];
					sprintf(movebuf,"\x1b\x5b%d\x44",pos);
					putstring(movebuf,-1,0);
					pos=0;
				} else if(tillftkn=='@' && pos<antal) { /* shift right */
					char movebuf[8];
					sprintf(movebuf,"\x1b\x5b%d\x43",antal-pos);
					putstring(movebuf,-1,0);
					pos=antal;
				}
/* END SHIFT-CSI */
			}
		}
	}
	inmat[antal]=0;
	if(eko==STAREKO)
	{
		if(pos!=antal)
		{
			char movebuf[8];
			sprintf(movebuf,"\x1b\x5b%d\x43",antal-pos);
			putstring(movebuf,-1,0);
		}

		for(x=0;x<antal;x++)
		{
			if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) putstring("\x1b\x5b\x44\x1b\x5b\x50",-1,0);
			else putstring("\b \b",-1,0);
		}
	}
	eka('\r');
	if(inmat[0] && eko==1 && modified) {
		for(x=9;x>0;x--) strcpy(commandhistory[x],commandhistory[x-1]);
		strncpy(commandhistory[0],inmat,1023);
		historycnt=-1;
	}
	if(historycnt>-1) historycnt--;
	return(0);
}

int incantrader(void)
{
	char tecken, aborted = FALSE;

	radcnt++;
	if(Servermem->inne[nodnr].rader && radcnt >= Servermem->inne[nodnr].rader && rxlinecount)
	{
		putstring("\r<RETURN>",-1,0);
		while((tecken=gettekn())!=3 && tecken!='\r' && !aborted)
		{
			if(carrierdropped()) aborted=TRUE;
		}
		if(tecken==3)
			aborted=TRUE;

		radcnt=0;
		putstring("\r        \r",-1,0);
	}

	return aborted;
}

int conputtekn(char *pekare,int size)
{
	int aborted=FALSE,x;
	char *tmppek, buffer[1200], constring[1201];

	strncpy(constring,pekare,1199);
	constring[1199]=0;
	pekare = &constring[0];

	if(!(Servermem->inne[nodnr].flaggor & ANSICOLOURS)) StripAnsiSequences(pekare);

	if(size == -1 && !pekare[0]) return(aborted);

	tmppek = pekare;
	x = 0;
	while((x < size || size == -1) && !aborted)
	{
		if(size < 1 && tmppek[0] == 0)
		{
			if(tmppek != pekare)
			{
				if(sendtocon(pekare, size)) aborted = TRUE;
			}
			break;
		}
		if(size > 0 && x++ == size)
		{
			if(pekare != tmppek)
			{
				tmppek[1] = 0;
				if(sendtocon(pekare, -1)) aborted = TRUE;
			}
			break;
		}
		if(tmppek[0] == '\n')
		{
			buffer[0] = 0;
			if(tmppek != pekare)
			{
				tmppek[0] = 0;
				if(pekare[0] != NULL)
					strcpy(buffer, pekare);

				strcat(buffer, "\n");
				pekare = ++tmppek;
			}
			else
			{
				buffer[0] = '\n';
				buffer[1] = NULL;
				pekare = ++tmppek;
			}

			if(sendtocon(&buffer[0], -1)) aborted = TRUE;
			tmppek = pekare - 1;
		}
		tmppek++;
	}

	return(aborted);
}

/*
 * Sends a string to the user. The arguments are compatible with printf().
 * The first argument is a char pointer to a format string and it's
 * followed by zero or more arguments to resolve the placeholders in the
 * format string.
 *
 * Returns 0 if everything is normal or non zero if sending was interrupted.
 * Interruption could be e.g. because the user has pressed Ctrl-C or that
 * the carrier has been dropped.
 *
 * TODO: This should be the default function to use for any code that
 * wants to do normal output to the user. Over time calls to puttekn()
 * should be replaced with calls to SendString() and the global variable
 * outbuffer should be made internal to this function/module.
 * There will probably have to be added other versions of this function
 * like SendStringCon() and a function to replace putstring() (that
 * doesn't count lines and pause with a <RETURN> prompt). They should
 * all just be wrappers to one common code path to send strings.
 */
int SendString(char *fmt, ...) {
  va_list arglist;
  int ret;

  va_start(arglist, fmt);
  vsprintf(outbuffer, fmt, arglist);
  ret = puttekn(outbuffer, -1);
  va_end(arglist);
  return ret;
}
