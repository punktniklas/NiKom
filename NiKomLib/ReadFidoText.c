/* Hur man letar upp from och to i en Fido-text. Från ANet.
* +------+-----------+-----------+
* !      !  Brev     !   Inlägg  !
* +------+-----------+-----------+
* !Till  !  DOMAIN   !           !
* !      !  INTL     !           !
* !      !  FTS-1    !           !
* +------+-----------+-----------+
* !Från  !  REPLYTO  !   REPLYTO ! Antar att REPLY ska vara "Till". Läggs mellan INTL och FTS
* !      !  DOMAIN   !   ORIGIN  !
* !      !  INTL     !           !
* !      !  MSGID    !   MSGID   !
* !      !  FTS-1    !           !
* +------+-----------+-----------+
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include <proto/exec.h>
#ifdef __GNUC__
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/utility.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"
#include "VersionStrings.h"
#include "Logging.h"

int getfidoline(char *fidoline,char *buffer,int linelen, int chrs, BPTR fh,char *quotepre,struct NiKomBase *NiKomBase) {
	int anttkn,foo,tmpret,hasquoted=FALSE,donotwordwrap=FALSE;
	UBYTE tmp,*findquotesign,insquotebuf[81];
	strcpy(fidoline,buffer);
	anttkn=strlen(fidoline);
	buffer[0]=0;
	for(;;) {
		tmpret=FGetC(fh);
		if(tmpret==-1) return(FALSE);
		tmp=tmpret;
		if(tmp==0x8d) continue;
		if(tmp==0x0a) continue;
		if(tmp==0x00) continue;
		if(tmp==0x0d) {
			if(quotepre && !hasquoted) {                // Om detta är sant så har ett return kommit
				if(fidoline[0]!=1 && fidoline[0]!=0) {   // men raden var kortare än fem tecken så
					findquotesign=strchr(fidoline,'>');   // den har inte blivit citerad.
					if(findquotesign) {                   // Det kan också vara så att strängen som
						strcpy(insquotebuf,findquotesign); // kommer i buffervariabeln följs direkt av
						findquotesign[1]=0;                // return.
						strcat(fidoline,insquotebuf);
					} else {
						strcpy(insquotebuf,fidoline);
						strcpy(fidoline,quotepre);
						strcat(fidoline,insquotebuf);
					}
				}
			}
			break;
		}
		switch(chrs) {
			case CHRS_CP437 :
				if(tmp<128) fidoline[anttkn++]=tmp;
				else fidoline[anttkn++]=NiKomBase->IbmToAmiga[tmp];
				break;
			case CHRS_SIS7 :
				fidoline[anttkn++]=NiKomBase->SF7ToAmiga[tmp];
				break;
			case CHRS_MAC :
				if(tmp<128) fidoline[anttkn++]=tmp;
				else fidoline[anttkn++]=NiKomBase->MacToAmiga[tmp];
				break;
			case CHRS_LATIN1 :
				fidoline[anttkn++]=tmp;
				break;
			default :
				fidoline[anttkn++]=convnokludge(tmp);
		}
		fidoline[anttkn]=0;
		if(quotepre && !hasquoted && anttkn>=5) {    // När antal tecken överstiger fem är det dags
			hasquoted = TRUE;                         // att kolla om raden redan är ett citat eller
			if(fidoline[0]!=1) {                      // inte. Om det är det ska bara ett extra '>'
				findquotesign=strchr(fidoline,'>');    // in. Annars ska hela quotepre in.
				if(findquotesign) {
					if(linelen<79) linelen = 79;
					donotwordwrap = TRUE;
					strcpy(insquotebuf,findquotesign);
					findquotesign[1]=0;
					strcat(fidoline,insquotebuf);
				} else {
					strcpy(insquotebuf,fidoline);
					strcpy(fidoline,quotepre);
					strcat(fidoline,insquotebuf);
				}
				anttkn=strlen(fidoline);
			}
		}
		if(anttkn>=linelen) {
			for(foo=anttkn;fidoline[foo]!=' ' && foo>0;foo--);
			if(foo!=0) {
				if(quotepre && foo <= strlen(quotepre)); // Om hela citatet är ett ord, wrappa inte
				else if(!donotwordwrap) {
					fidoline[foo]=0;
					strncpy(buffer,&fidoline[foo+1],anttkn-foo-1);
					buffer[anttkn-foo-1]=0;
				}
			}
			break;
		}
	}
	return(TRUE);
}

char *hittaefter(strang)
char *strang;
{
	int test=TRUE;
	while(test) {
		test=FALSE;
		while(*(++strang)!=' ' && *strang!=0);
		if(*(strang+1)==' ') test=TRUE;
	}
	return(*strang==0 ? strang : ++strang);
}

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

USHORT getTwoByteField(UBYTE *bytes, char littleEndian) {
  if(littleEndian) {
    return (USHORT) (bytes[1] * 0x100 + bytes[0]);
  } else {
    return (USHORT) (bytes[0] * 0x100 + bytes[1]);
  }
}

void writeTwoByteField(USHORT value, UBYTE *bytes, char littleEndian) {
  if(littleEndian) {
    bytes[1] = value / 0x100;
    bytes[0] = value % 0x100;
  } else {
    bytes[0] = value / 0x100;
    bytes[1] = value % 0x100;
  }
}

struct FidoText * __saveds __asm LIBReadFidoText(register __a0 char *filename, register __a1 struct TagItem *taglist, register __a6 struct NiKomBase *NiKomBase) {
	int nokludge, noseenby,chrset=0, x, tearlinefound=FALSE, headeronly,quote,linelen;
	struct FidoText *fidotext;
	struct FidoLine *fltmp;
	BPTR fh;
	UBYTE intl[30],replyto[20],origin[20],topt[5],fmpt[5],ftshead[190],fidoline[81],flbuffer[81],*foo,prequote[10];

	if(!NiKomBase->Servermem) return(NULL);

	intl[0]=replyto[0]=origin[0]=topt[0]=fmpt[0]=0;
	nokludge=GetTagData(RFT_NoKludges,0,taglist);
	noseenby=GetTagData(RFT_NoSeenBy,0,taglist);
	headeronly=GetTagData(RFT_HeaderOnly,0,taglist);
	quote = GetTagData(RFT_Quote,0,taglist);
	linelen = GetTagData(RFT_LineLen,79,taglist);
	if(!(fidotext = (struct FidoText *)AllocMem(sizeof(struct FidoText),MEMF_CLEAR))) return(NULL);
	NewList((struct List *)&fidotext->text);
	if(!(fh=Open(filename,MODE_OLDFILE))) {
          FreeMem(fidotext,sizeof(struct FidoText));
          return(NULL);
	}
	Read(fh,ftshead,190);
	strcpy(fidotext->fromuser,&ftshead[0]);
	strcpy(fidotext->touser,&ftshead[36]);
	strcpy(fidotext->subject,&ftshead[72]);
	strcpy(fidotext->date,&ftshead[144]);
	fidotext->tonode = getTwoByteField(&ftshead[166], LITTLE_ENDIAN);
	fidotext->fromnode = getTwoByteField(&ftshead[168], LITTLE_ENDIAN);
	fidotext->fromnet = getTwoByteField(&ftshead[172], LITTLE_ENDIAN);
	fidotext->tonet = getTwoByteField(&ftshead[174], LITTLE_ENDIAN);
	fidotext->attribut = getTwoByteField(&ftshead[186], LITTLE_ENDIAN);
	Flush(fh);
	if(quote) {
		prequote[0]=' ';
		prequote[1]=0;
		x=1;
		foo=strcpy(flbuffer,fidotext->fromuser); // Bara lånar flbuffer tillfälligt..
		LIBConvChrsToAmiga(foo,0,0,NiKomBase);
		while(foo[0]) {
			prequote[x]=foo[0];
			prequote[x+1]=0;
			x++;
			foo = hittaefter(foo);
		}
		prequote[x]='>';
		prequote[x+1]=' ';
		prequote[x+2]=0;
	}
	flbuffer[0]=0;
	while(getfidoline(fidoline,flbuffer,linelen,chrset,fh,quote ? prequote : NULL,NiKomBase)) {
		if(fidoline[0]==1) {
			foo=hittaefter(&fidoline[1]);
			if(!strncmp(&fidoline[1],"INTL",4)) strcpy(intl,foo);
			else if(!strncmp(&fidoline[1],"TOPT",4)) strcpy(topt,foo);
			else if(!strncmp(&fidoline[1],"FMPT",4)) strcpy(fmpt,foo);
			else if(!strncmp(&fidoline[1],"MSGID",5)) {
				strncpy(fidotext->msgid,foo,49);
				fidotext->msgid[49]=0;
			} else if(!strncmp(&fidoline[1],"REPLY",5)) {
				strncpy(replyto,foo,19);
				replyto[19]=0;
			} else if(!strncmp(&fidoline[1],"CHRS",4)) {
				if(!strncmp(foo,"LATIN-1 2",9)) chrset=CHRS_LATIN1;
				if(!strncmp(foo,"IBMPC 2",7)) chrset=CHRS_CP437;
				if(!strncmp(foo,"SWEDISH 1",9)) chrset=CHRS_SIS7;
				if(!strncmp(foo,"MAC 2",9)) chrset=CHRS_MAC;
			}
			if(nokludge) continue;
		}
		if(quote) foo=hittaefter(&fidoline[1]);
		else foo=fidoline;
		if(tearlinefound && noseenby &&  !strncmp(foo,"SEEN-BY:",8)) continue;
		if(tearlinefound && !strncmp(foo," * Origin:",10)) {
			for(x=strlen(fidoline)-1;x>0 && fidoline[x]!='('; x--);
			if(x>0) {
				strncpy(origin,&fidoline[x+1],19);
				origin[19]=0;
			}
		}
		if(!strncmp(foo,"---",3) && foo[3]!='-') tearlinefound=TRUE;
		if(headeronly) continue;
		if(!(fltmp = (struct FidoLine *)AllocMem(sizeof(struct FidoLine),MEMF_CLEAR))) {
			Close(fh);
			while(fltmp=(struct FidoLine *)RemHead((struct List *)&fidotext->text)) FreeMem(fltmp,sizeof(struct FidoLine));
			FreeMem(fidotext,sizeof(struct FidoText));
			return(NULL);
		}
		strcpy(fltmp->text,fidoline);
		AddTail((struct List *)&fidotext->text,(struct Node *)fltmp);
	}
	Close(fh);
	if(fidotext->attribut & FIDOT_PRIVATE) {
		if(fidotext->msgid[0]) {
			if(x=getzone(fidotext->msgid)) fidotext->fromzone=x;
			if(x=getnet(fidotext->msgid)) fidotext->fromnet=x;
			if(x=getnode(fidotext->msgid)) fidotext->fromnode=x;
			if(x=getpoint(fidotext->msgid)) fidotext->frompoint=x;
		}
		if(replyto[0]) {
			if(x=getzone(replyto)) fidotext->tozone=x;
			if(x=getnet(replyto)) fidotext->tonet=x;
			if(x=getnode(replyto)) fidotext->tonode=x;
			if(x=getpoint(replyto)) fidotext->topoint=x;
		}
		if(intl[0]) {
			if(x=getzone(intl)) fidotext->tozone=x;
			if(x=getnet(intl)) fidotext->tonet=x;
			if(x=getnode(intl)) fidotext->tonode=x;
			foo=hittaefter(intl);
			if(x=getzone(foo)) fidotext->fromzone=x;
			if(x=getnet(foo)) fidotext->fromnet=x;
			if(x=getnode(foo)) fidotext->fromnode=x;
		}
		if(topt[0]) fidotext->topoint = atoi(topt);
		if(fmpt[0]) fidotext->frompoint = atoi(fmpt);
	} else {
		if(fidotext->msgid[0]) {
			if(x=getzone(fidotext->msgid)) fidotext->fromzone=x;
			if(x=getnet(fidotext->msgid)) fidotext->fromnet=x;
			if(x=getnode(fidotext->msgid)) fidotext->fromnode=x;
			if(x=getpoint(fidotext->msgid)) fidotext->frompoint=x;
		}
		if(replyto[0]) {
			if(x=getzone(replyto)) fidotext->tozone=x;
			if(x=getnet(replyto)) fidotext->tonet=x;
			if(x=getnode(replyto)) fidotext->tonode=x;
			if(x=getpoint(replyto)) fidotext->topoint=x;
		}
		if(origin[0]) {
			if(x=getzone(origin)) fidotext->fromzone=x;
			if(x=getnet(origin)) fidotext->fromnet=x;
			if(x=getnode(origin)) fidotext->fromnode=x;
			if(x=getpoint(origin)) fidotext->frompoint=x;
		}
	}
	if(fidotext->tozone==0) fidotext->tozone=fidotext->fromzone;
	LIBConvChrsToAmiga(fidotext->subject,0,chrset,NiKomBase);
	LIBConvChrsToAmiga(fidotext->fromuser,0,chrset,NiKomBase);
	LIBConvChrsToAmiga(fidotext->touser,0,chrset,NiKomBase);
	return(fidotext);
}

void __saveds __asm LIBFreeFidoText(register __a0 struct FidoText *fidotext) {
	struct FidoLine *fltmp;
	while(fltmp=(struct FidoLine *)RemHead((struct List *)&fidotext->text)) FreeMem(fltmp,sizeof(struct FidoLine));
	FreeMem(fidotext,sizeof(struct FidoText));
}

int gethwm(char *dir, char littleEndian) {
	BPTR fh;
	int nummer;
	char fullpath[100], buf[2];
	strcpy(fullpath,dir);
	AddPart(fullpath,"1.msg",99);
	if(!(fh=Open(fullpath,MODE_OLDFILE))) return(0);
	Seek(fh,184,OFFSET_BEGINNING);
        buf[0] = FGetC(fh);
        buf[1] = FGetC(fh);
	nummer = getTwoByteField(buf, littleEndian);
	Close(fh);
	return(nummer);
}

int sethwm(char *dir, int nummer, char littleEndian) {
	BPTR fh;
	char fullpath[100], buf[2];
	strcpy(fullpath,dir);
	AddPart(fullpath,"1.msg",99);
	if(!(fh=Open(fullpath,MODE_OLDFILE))) return(0);
	Seek(fh,184,OFFSET_BEGINNING);
        writeTwoByteField(nummer, buf, littleEndian);
	FPutC(fh, buf[0]);
	FPutC(fh, buf[1]);
	Close(fh);
	return(1);
}

int __saveds __asm LIBWriteFidoText(register __a0 struct FidoText *fidotext, register __a1 struct TagItem *taglist,register __a6 struct NiKomBase *NiKomBase) {
	BPTR lock,fh;
	struct FidoLine *fl,*next;
	int nummer,going=TRUE,charset;
	UBYTE *dir,filename[20],fullpath[100],ftshead[190],*domain,*reply,flowchar,datebuf[14],timebuf[10];
	struct DateTime dt;

	if(!NiKomBase->Servermem) return(FALSE);

	dir=(UBYTE *)GetTagData(WFT_MailDir,NULL,taglist);
	charset=GetTagData(WFT_CharSet,CHRS_LATIN1,taglist);
	domain=(UBYTE *)GetTagData(WFT_Domain,NULL,taglist);
	reply=(UBYTE *)GetTagData(WFT_Reply,NULL,taglist);
	if(!dir) return(0);
	memset(ftshead,0,190);
	strncpy(&ftshead[0],fidotext->fromuser,35);
	LIBConvChrsFromAmiga(fidotext->touser,0,charset,NiKomBase);
	strncpy(&ftshead[36],fidotext->touser,35);
	LIBConvChrsFromAmiga(fidotext->subject,0,charset,NiKomBase);
	strncpy(&ftshead[72],fidotext->subject,71);
	strncpy(&ftshead[144],fidotext->date,19);
        writeTwoByteField(fidotext->tonode, &ftshead[166], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->fromnode, &ftshead[168], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->fromnet, &ftshead[172], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->tonet, &ftshead[174], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->tozone, &ftshead[176], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->fromzone, &ftshead[178], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->topoint, &ftshead[180], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->frompoint, &ftshead[182], LITTLE_ENDIAN);
        writeTwoByteField(fidotext->attribut, &ftshead[186], LITTLE_ENDIAN);
	if(!(nummer = gethwm(dir, LITTLE_ENDIAN))) nummer = 2;
	while(going) {
		sprintf(filename,"%d.msg",nummer);
		strcpy(fullpath,dir);
		AddPart(fullpath,filename,99);
		if(lock = Lock(fullpath,ACCESS_READ)) {
			UnLock(lock);
			nummer++;
		} else going=FALSE;
	}
	if(!(fh=Open(fullpath,MODE_NEWFILE))) return(FALSE);
	if(Write(fh,ftshead,190)==-1) {
		Close(fh);
		return(FALSE);
	}
	Flush(fh);
	if(domain) sprintf(ftshead,"\001MSGID: %d:%d/%d.%d@%s %x\r",fidotext->fromzone,fidotext->fromnet,
		fidotext->fromnode, fidotext->frompoint, domain, time(NULL));
	else sprintf(ftshead,"\001MSGID: %d:%d/%d.%d %d\r",fidotext->fromzone,fidotext->fromnet,
		fidotext->fromnode, fidotext->frompoint, time(NULL));
	FPuts(fh,ftshead);
	if(reply) {
		sprintf(ftshead,"\001REPLY: %s\r",reply);
		FPuts(fh,ftshead);
	}
	switch(charset) {
		case CHRS_CP437 :
			strcpy(ftshead,"\001CHRS: IBMPC 2\r");
			break;
		case CHRS_SIS7 :
			strcpy(ftshead,"\001CHRS: SWEDISH 1\r");
			break;
		case CHRS_MAC :
			strcpy(ftshead,"\001CHRS: MAC 2\r");
			break;
		case CHRS_LATIN1 :
		default :
			strcpy(ftshead,"\001CHRS: LATIN-1 2\r");
			break;
	}
	FPuts(fh,ftshead);
	if(fidotext->attribut & FIDOT_PRIVATE) {
		sprintf(ftshead,"\001INTL %d:%d/%d %d:%d/%d\r",fidotext->tozone,fidotext->tonet,fidotext->tonode,
			fidotext->fromzone,fidotext->fromnet,fidotext->fromnode);
		FPuts(fh,ftshead);
		if(fidotext->frompoint) {
			sprintf(ftshead,"\001FMPT %d\r",fidotext->frompoint);
			FPuts(fh,ftshead);
		}
		if(fidotext->topoint) {
			sprintf(ftshead,"\001TOPT %d\r",fidotext->topoint);
			FPuts(fh,ftshead);
		}
	}
	FPuts(fh, "\001PID: NiKom " NIKRELEASE "\r");
	for(fl=(struct FidoLine *)fidotext->text.mlh_Head;fl->line_node.mln_Succ;fl=(struct FidoLine *)fl->line_node.mln_Succ) {
		next=(struct FidoLine *)fl->line_node.mln_Succ;
		strcpy(ftshead,fl->text);
		LIBConvChrsFromAmiga(ftshead,0,charset,NiKomBase);
		if(!next->line_node.mln_Succ) next=NULL;
		flowchar='\r';
		if(next) {
			if(next->text[0] == ' ' || next->text[0]=='\t' || next->text[0]==0) flowchar = '\r';
			else flowchar=' ';
		}
		if(strlen(ftshead)<50) flowchar='\r';
		FPuts(fh,ftshead);
		FPutC(fh,flowchar);
	}
	if(fidotext->attribut & FIDOT_PRIVATE) {
		DateStamp(&dt.dat_Stamp);
		dt.dat_Format = FORMAT_DOS;
		dt.dat_Flags = 0;
		dt.dat_StrDay = NULL;
		dt.dat_StrDate = datebuf;
		dt.dat_StrTime = timebuf;
		DateToStr(&dt);
		dt.dat_StrDate[2] = ' ';
		dt.dat_StrDate[6] = ' ';
		if(dt.dat_StrDate[4] == 'k') dt.dat_StrDate[4] = 'c'; /* Okt -> Oct */
		if(dt.dat_StrDate[5] == 'j') dt.dat_StrDate[5] = 'y'; /* Maj -> May */
		if(domain) sprintf(ftshead,"\001Via NiKom %d:%d/%d.%d@%s %s %s\r",fidotext->fromzone,fidotext->fromnet,
			fidotext->fromnode,fidotext->frompoint,domain,datebuf,timebuf);
		else sprintf(ftshead,"\001Via NiKom %d:%d/%d.%d %s %s\r",fidotext->fromzone,fidotext->fromnet,
			fidotext->fromnode,fidotext->frompoint,datebuf,timebuf);
		FPuts(fh,ftshead);
	} else {
		if(fidotext->frompoint) sprintf(ftshead,"SEEN-BY: %d/%d.%d\r",fidotext->fromnet,fidotext->fromnode,fidotext->frompoint);
		else sprintf(ftshead,"SEEN-BY: %d/%d\r",fidotext->fromnet,fidotext->fromnode);
		FPuts(fh,ftshead);
		if(fidotext->frompoint) sprintf(ftshead,"\001PATH: %d/%d.%d\r",fidotext->fromnet,fidotext->fromnode,fidotext->frompoint);
		else sprintf(ftshead,"\001PATH: %d/%d\r",fidotext->fromnet,fidotext->fromnode);
		FPuts(fh,ftshead);
	}
	Close(fh);
	return(nummer);
}
