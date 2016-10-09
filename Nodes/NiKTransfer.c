#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <devices/serial.h>
#include <devices/timer.h>
#include <xproto.h>
#include <xpr_lib.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "DiskUtils.h"
#include "FileAreaUtils.h"
#include "Logging.h"
#include "Terminal.h"
#include "BasicIO.h"

#define EKO		1
#define EJEKO	0

extern struct System *Servermem;
extern int area2,inloggad,nodnr,rad;
extern struct IOExtSer *serwritereq,*serreadreq,*serchangereq;
extern struct timerequest *timerreq;
extern struct MsgPort *serwriteport,*serreadport,*serchangeport,*timerport;
extern char outbuffer[],inmat[],*argument,zinitstring[],serinput;
extern struct Inloggning Statstr;
extern struct MinList edit_list;

struct XPR_IO *xio;
struct Library *XProtocolBase;
/*
char zmodeminit[] = "TN OR B16 F0 AN DN KY SN RN PNiKom:Program/Amig",
	xmodeminit[] = "T0,C1,K0";
*/
char xprfilnamn[100];
long countbytes,filesize,cps, ulfiles;
struct TransferFiles *currentfil;
struct MinList tf_list;

extern int writefiles(int);
extern int updatefile(int,struct Fil *);

long __saveds __regargs nik_fopen(char *filename,char *accessmode) {
	BPTR fh;
	switch(accessmode[0]) {
		case 'r' :
			fh=Open(filename,MODE_OLDFILE);
			break;
		case 'w' :
			fh=Open(filename,MODE_NEWFILE);
			break;
		case 'a' :
			if(!(fh=Open(filename,MODE_OLDFILE)))
				fh=Open(filename,MODE_NEWFILE);
			else Seek(fh,0,OFFSET_END);
			break;
		default :
			printf("Felaktig accessmode i nik_fopen!\n");
			break;
	}
	return((long)fh);
}

long __saveds __regargs nik_fclose(LONG *fh) {
	/* printf("Xpr_fclose anropad, fp=%d\n",fp); */
	if(!fh) return(NULL);
	if(Close((BPTR)fh)) return(NULL);
	else return(EOF);
}

long __saveds __regargs nik_fread(char *databuffer,long size,long count,LONG *fh) {
	LONG ret;
	ret=Read((BPTR)fh,databuffer,size*count);
	if(ret==-1) return(0);
	else return(ret/size);
}

long __saveds __regargs nik_fwrite(char *databuffer,long size,long count,LONG *fh) {
	LONG ret;
	ret=Write((BPTR)fh,databuffer,size*count);
	if(ret==-1) return(0);
	else return(ret/size);
}

long __saveds __regargs nik_fseek(LONG *fh,long offset,long origin) {
	LONG mode;
	switch(origin) {
		case 0 :
			mode=OFFSET_BEGINNING;
			break;
		case 1 :
			mode=OFFSET_CURRENT;
			break;
		case 2 :
			mode=OFFSET_END;
			break;
	}
	if(Seek((BPTR)fh,offset,mode)==-1) return(-1);
	else return(0);
}

long __saveds __regargs nik_sread(char *databuffer,long size,long timeout) {
  ULONG signals, sersig = 1L << serreadport->mp_SigBit,
    timersig = 1L << timerport->mp_SigBit;
  if(timeout) {
    serreadreq->IOSer.io_Command = CMD_READ;
    serreadreq->IOSer.io_Length = size;
    serreadreq->IOSer.io_Data = (APTR)databuffer;
    SendIO((struct IORequest *)serreadreq);
    timerreq->tr_node.io_Command = TR_ADDREQUEST;
    timerreq->tr_time.tv_secs = timeout / 1000000;
    timerreq->tr_time.tv_micro = timeout % 1000000;
    timerreq->tr_node.io_Message.mn_ReplyPort = timerport;
    SendIO((struct IORequest *)timerreq);
    for(;;) {
      signals = Wait(sersig | timersig);
      if((signals & sersig) && CheckIO((struct IORequest *)serreadreq)) {
        WaitIO((struct IORequest *)serreadreq);
        AbortIO((struct IORequest *)timerreq);
        WaitIO((struct IORequest *)timerreq);
        QueryCarrierDropped();
        if(serreadreq->IOSer.io_Error || ImmediateLogout()) {
          return(-1L);
        }
        return((long)serreadreq->IOSer.io_Actual);
      }
      if((signals & timersig) && CheckIO((struct IORequest *)timerreq)) {
        WaitIO((struct IORequest *)timerreq);
        AbortIO((struct IORequest *)serreadreq);
        WaitIO((struct IORequest *)serreadreq);
        QueryCarrierDropped();
        /* printf("Timeout! Size=%d Timeout=%d Actual=%d",size,timeout,serreadreq->IOSer.io_Actual); */
        return((long)serreadreq->IOSer.io_Actual);
      }
    }
  } else {
    serreadreq->IOSer.io_Command=SDCMD_QUERY;
    DoIO((struct IORequest *)serreadreq);
    serreadreq->IOSer.io_Command=CMD_READ;
    if(serreadreq->IOSer.io_Actual > size) serreadreq->IOSer.io_Length=size;
    else serreadreq->IOSer.io_Length=serreadreq->IOSer.io_Actual;
    serreadreq->IOSer.io_Data=(APTR)databuffer;
    DoIO((struct IORequest *)serreadreq);
    QueryCarrierDropped();
    return((long)serreadreq->IOSer.io_Actual);
  }
}

long __saveds __regargs nik_swrite(char *databuffer,long size) {
  if(ConnectionLost()) {
    return -1L;
  }
  serwritereq->IOSer.io_Command = CMD_WRITE;
  serwritereq->IOSer.io_Length = size;
  serwritereq->IOSer.io_Data = (APTR)databuffer;
  if(DoIO((struct IORequest *)serwritereq) || ConnectionLost()) {
    return(-1L);
  }
  return(0L);
}

long __saveds __regargs nik_update(struct XPR_UPDATE *update) {
	struct TransferFiles *tf, *tmptf;
	int equalulfiles = 0;

	if(update->xpru_updatemask & XPRU_FILENAME) {
		sprintf(outbuffer,"\r\nF�r �ver %s\r\n",update->xpru_filename);
		conputtekn(outbuffer,-1);
		strcpy(xprfilnamn,update->xpru_filename);
		if(Servermem->action[nodnr]==UPLOAD || Servermem->action[nodnr]==DOWNLOAD) Servermem->vilkastr[nodnr] = FilePart(xprfilnamn);
	}
	if(update->xpru_updatemask & XPRU_FILESIZE) {
		sprintf(outbuffer,"Filens l�ngd %d\r\n",update->xpru_filesize);
		conputtekn(outbuffer,-1);
		filesize=update->xpru_filesize;
	}
	if(update->xpru_updatemask & XPRU_MSG) { conputtekn(update->xpru_msg,-1); conputtekn("\r\n",-1); }
	if(update->xpru_updatemask & XPRU_ERRORMSG) { conputtekn(update->xpru_errormsg,-1); conputtekn("\r\n",-1); }
	if(update->xpru_updatemask & XPRU_DATARATE) cps=update->xpru_datarate;
	if(update->xpru_updatemask & XPRU_BYTES) {
		sprintf(outbuffer,"\rBytes: %d (%d cps)",update->xpru_bytes,update->xpru_datarate);
		conputtekn(outbuffer,-1);
		countbytes=update->xpru_bytes;
		if(countbytes == filesize && Servermem->action[nodnr]==UPLOAD) {
			tmptf = (struct TransferFiles *)tf_list.mlh_Head;
			while(tmptf)
			{
				if(!stricmp(tmptf->Filnamn,xprfilnamn))
					equalulfiles = 1;
				tmptf = (struct TransferFiles *)tmptf->node.mln_Succ;
			}
			if(!equalulfiles)
			{
				if(!(tf=(struct TransferFiles *)AllocMem(sizeof(struct TransferFiles),MEMF_CLEAR))) {
					puttekn("Kunde inte allokera en TransferFiles-struktur\n\r",-1);
				}
				else
				{
					if(Servermem->cfg.logmask & LOG_RECFILE) {
                                          LogEvent(USAGE_LOG, INFO, "Tog emot filen %s fr�n %s",
                                                   xprfilnamn, getusername(inloggad));
					}
					strcpy(tf->Filnamn,xprfilnamn);
					tf->cps = cps;
					AddTail((struct List *)&tf_list,(struct Node *)tf);
					ulfiles++;
				}
			}

		}
	}
	return(0L);
}

long __saveds __regargs nik_sflush(void) {
	long ret;
	serchangereq->IOSer.io_Command=CMD_CLEAR;
	DoIO((struct IORequest *)serchangereq);
	serchangereq->IOSer.io_Command=CMD_FLUSH;
	ret=(long)DoIO((struct IORequest *)serchangereq);
	return(ret);
}

long __saveds __regargs nik_chkabort(void) {
  if(ImmediateLogout()) {
    return -1L;
  }
  return 0L;
}

long __saveds __regargs nik_gets(char *prompt,char *buffer) {
	conputtekn("\r\n\nNu promptas det!!\r\n",-1);
	conputtekn(prompt,-1);
	strcpy(buffer,"");
	return(1L);
}

long __saveds __regargs nik_finfo(char *filename,long typeinfo) {
	__aligned struct FileInfoBlock info;
	dfind(&info,filename,0);
	switch(typeinfo) {
		case 1 :
			return((long)info.fib_Size);
		case 2 :
			return(1L);
	}
}

long __saveds __regargs nik_ffirst(char *buffer,char *pattern) {
	struct TransferFiles *tf;
	tf=(struct TransferFiles *)tf_list.mlh_Head;
	if(tf->node.mln_Succ) {
		strcpy(buffer,tf->path);
		currentfil=tf;
		return((long)tf);
	}
	return(0L);
}

long __saveds __regargs nik_fnext(long oldstate, char *buffer, char *pattern) {
	struct TransferFiles *tf;
	if(countbytes==filesize) currentfil->sucess=TRUE;
	currentfil->cps=cps;
	tf=(struct TransferFiles *)currentfil->node.mln_Succ;
	if(tf->node.mln_Succ) {
		strcpy(buffer,tf->path);
		currentfil=tf;
		return((long)tf);
	}
	return(0L);
}

void xpr_setup(struct XPR_IO *sio) {
	sio->xpr_filename=NULL;
	sio->xpr_fopen=(long (* )())nik_fopen;
	sio->xpr_fclose=(long (* )())nik_fclose;
	sio->xpr_fread=(long (* )())nik_fread;
	sio->xpr_fwrite=(long (* )())nik_fwrite;
	sio->xpr_sread=(long (* )())nik_sread;
	sio->xpr_swrite=(long (* )())nik_swrite;
	sio->xpr_sflush=(long (* )())nik_sflush;
	sio->xpr_update=(long (* )())nik_update;
	sio->xpr_chkabort=(long (* )())nik_chkabort;
	sio->xpr_chkmisc=NULL;
	sio->xpr_gets=(long (* )())nik_gets;
	sio->xpr_setserial=NULL;
	sio->xpr_ffirst=(long (* )())nik_ffirst;
	sio->xpr_fnext=(long (* )())nik_fnext;
	sio->xpr_finfo=(long (* )())nik_finfo;
	sio->xpr_fseek=(long (* )())nik_fseek;
	sio->xpr_extension=0L;
	sio->xpr_options=NULL;
	sio->xpr_data=NULL;
	return;
}

int download(void) {
	int cnt,freedlonly=FALSE, global = 0, matchall = 0;
	char *nextfile,*nextnext;
	struct Fil *filpek;
	struct TransferFiles *tf;
	if(Servermem->inne[nodnr].upload+1==0) {
		puttekn("\r\n\nVARNING!! Uploads = -1",-1);
		return(0);
	}
	if(Servermem->cfg.uldlratio[Servermem->inne[nodnr].status] && Servermem->cfg.uldlratio[Servermem->inne[nodnr].status] < (Servermem->inne[nodnr].download+1)/(Servermem->inne[nodnr].upload+1)) {
		sprintf(outbuffer,"\r\n\nDu m�ste ladda upp en fil per %d filer du laddar ner.\r\n",Servermem->cfg.uldlratio[Servermem->inne[nodnr].status]);
		puttekn(outbuffer,-1);
		puttekn("Endast filer med fri download till�ts.\n\r",-1);
		freedlonly=TRUE;
	}
	if(area2==-1) {
		puttekn("\r\n\nDu befinner dig inte i n�gon area!\r\n",-1);
		return(0);
	}
	if((!global && Servermem->areor[area2].flaggor & AREA_NODOWNLOAD) && (Servermem->inne[nodnr].status < Servermem->cfg.st.laddaner)) {
		puttekn("\n\n\rDu har ingen r�tt att ladda ner fr�n den h�r arean!\n\r",-1);
		return(0);
	}
	if(!argument[0]) {
		puttekn("\r\n\nSkriv: Download <filnamn> [<filnamn> [<filnamn> [...]]]\r\n",-1);
		return(0);
	}
	nextfile=argument;
	if(nextfile[0] == '-')
	{
		puttekn("\r\n\nTest\r\n",-1);
		if(nextfile[1] == 'g' || nextfile[1] == 'G')
			global = 1;

		if(nextfile[1] == 'a' || nextfile[1] == 'A')
			matchall = 1;

		if(!global && !matchall)
		{
			puttekn("\r\n\nOk�nd option, bara -A eller -G st�djs.\r\n",-1);
			return(0);
		}

		nextfile=hittaefter(nextfile);
	}

	nextnext=hittaefter(nextfile);
	puttekn("\n\n\r",-1);
	NewList((struct List *)&tf_list);
	while(nextfile[0])
	{
		for(cnt=0;nextfile[cnt]!=' ' && nextfile[cnt]!=0;cnt++);
		nextfile[cnt]=0;
		if(!global && !(filpek=parsefil(nextfile,area2))) {
			sprintf(outbuffer,"Finns ingen fil som matchar \"%s\"\n\r",nextfile);
			puttekn(outbuffer,-1);
			nextfile=nextnext;
			nextnext=hittaefter(nextnext);
			continue;
		}
		if(!filpek && global && !(filpek=parsefilallareas(nextfile)))
		{
			sprintf(outbuffer,"Finns ingen fil som matchar \"%s\"\n\r",nextfile);
			puttekn(outbuffer,-1);
			nextfile=nextnext;
			nextnext=hittaefter(nextnext);
			continue;
		}

		if(filpek->status>Servermem->inne[nodnr].status && Servermem->inne[nodnr].status<Servermem->cfg.st.laddaner)
		{
			sprintf(outbuffer,"Du har ingen r�tt att ladda ner %s!\n\r",filpek->namn);
			puttekn(outbuffer,-1);
			nextfile=nextnext;
			nextnext=hittaefter(nextnext);
			continue;
		}
		if(freedlonly && !(filpek->flaggor & FILE_FREEDL))
		{
			nextfile=nextnext;
			nextnext=hittaefter(nextnext);
			continue;
		}
		if(!(tf=(struct TransferFiles *)AllocMem(sizeof(struct TransferFiles),MEMF_CLEAR))) {
			puttekn("Kunde inte allokera en TransferFiles-struktur\n\r",-1);
			nextfile=nextnext;
			nextnext=hittaefter(nextnext);
			continue;
		}
		sprintf(tf->path,"%s%s",Servermem->areor[area2].dir[filpek->dir],filpek->namn);
		tf->filpek=filpek;
		AddTail((struct List *)&tf_list,(struct Node *)tf);
		sprintf(outbuffer,"Skickar filen %s. Storlek: %d %s\r\n",filpek->namn,filpek->size,(filpek->flaggor & FILE_FREEDL) ? "(Fri download)" : "");
		puttekn(outbuffer,-1);
		nextfile=nextnext;
		nextnext=hittaefter(nextnext);
	}
	if(!tf_list.mlh_Head->mln_Succ) {
		puttekn("\n\rInga filer att skicka.\n\r",-1);
		return(0);
	}
	Servermem->action[nodnr] = DOWNLOAD;
	Servermem->varmote[nodnr] = area2;
	Servermem->vilkastr[nodnr] = NULL;
	sendbinfile();
	if(Servermem->cfg.logmask & LOG_DOWNLOAD) {
		for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ)
			if(tf->sucess) {
                          LogEvent(USAGE_LOG, INFO, "%s laddar ner %s (%d cps)",
                                   getusername(inloggad), tf->filpek->namn, tf->cps);
			}
	}
	for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ) {
		if(tf->sucess) {
			if(!(tf->filpek->flaggor & FILE_FREEDL)) Servermem->inne[nodnr].download++;
			Statstr.dl++;
			raisefiledl(tf->filpek);
			sprintf(outbuffer,"%s, %d cps\n\r",tf->filpek->namn,tf->cps);
			puttekn(outbuffer,-1);
		} else {
			sprintf(outbuffer,"%s misslyckades.\n\r",tf->filpek->namn);
			puttekn(outbuffer,-1);
		}
	}
	while(tf=(struct TransferFiles *)RemHead((struct List *)&tf_list))
		FreeMem(tf,sizeof(struct TransferFiles));

	if(ImmediateLogout()) {
          return 1;
        } else {
          return 0;
        }
}

int upload(void) {
  int area, ret, editret, dirnr, writeLong, isCorrect;
  long tid;
  struct EditLine *el;
  FILE *fp;
  __aligned struct FileInfoBlock info;
  struct Fil *allokpek;
  char nikfilename[100],errbuff[100],filnamn[50],tmpfullname[100], outbuffer[81];
  struct TransferFiles *tf;

  if(Servermem->cfg.ar.preup1) sendautorexx(Servermem->cfg.ar.preup1);
  if(area2==-1) {
    puttekn("\r\nI vilken area? ",-1);
  } else {
    sprintf(outbuffer,"\r\nI vilken area? (<RETURN> f�r %s)",Servermem->areor[area2].namn);
    puttekn(outbuffer,-1);
  }
  if(getstring(EKO,40,NULL)) return(1);
  if((area=parsearea(inmat))==-1) {
    puttekn("\n\rFinns ingen s�dan area!\n\r",-1);
    return(0);
  } else if(area==-3) {
    if(area2==-1) return(0);
    area=area2;
  }
  if(!arearatt(area, inloggad, &Servermem->inne[nodnr])) {
    puttekn("\n\rFinns ingen s�dan area!\n\r",-1);
    return(0);
  }
  if((Servermem->areor[area].flaggor & AREA_NOUPLOAD) && (Servermem->inne[nodnr].status < Servermem->cfg.st.laddaner)) {
    puttekn("\n\n\rDu har ingen r�tt att ladda upp till den arean!\n\r",-1);
    return(0);
  }
  Servermem->action[nodnr] = UPLOAD;
  Servermem->varmote[nodnr] = area;
  Servermem->vilkastr[nodnr] = NULL;
  if(ret=recbinfile(Servermem->cfg.ultmp)) {
    while(tf=(struct TransferFiles *)RemHead((struct List *)&tf_list))
      FreeMem(tf,sizeof(struct TransferFiles));
    if(ImmediateLogout()) {
      return 1;
    }
    return 0;
  }

  for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ) {
    strcpy(xprfilnamn,tf->Filnamn);
    if(stcgfn(filnamn,xprfilnamn)>40) puttekn("\r\nVARNING! Filnamnet st�rre �n 40 tecken!\r\n",-1);
    if(!filnamn[0]) {
      puttekn("\r\n\nHmm... Filen har inget namn. Skriv ett brev till sysop.\r\n",-1);
      continue;
    }
    if(!valnamn(filnamn,area,errbuff)) {
      for(;;) {
        puttekn(errbuff,-1);
        puttekn("\r\nNytt namn: ",-1);
        if(getstring(EKO,40,NULL)) { DeleteFile(xprfilnamn); return(1); }
        if(!inmat[0]) continue;
        if(valnamn(inmat,area,errbuff)) break;
      }
      strcpy(filnamn,inmat);
      sprintf(tmpfullname,"%s%s",Servermem->cfg.ultmp,filnamn);
      if(!Rename(xprfilnamn,tmpfullname)) {
        sprintf(outbuffer,"\r\n\nKunde inte d�pa om filen fr�n '%s'\r\ntill '%s'.\r\n",xprfilnamn,tmpfullname);
        puttekn(outbuffer,-1);
        DeleteFile(xprfilnamn);
        continue;
      }
    } else strcpy(tmpfullname,xprfilnamn);
    if(!(allokpek=(struct Fil *)AllocMem(sizeof(struct Fil),MEMF_PUBLIC | MEMF_CLEAR))) {
      puttekn("\r\n\nKunde inte allokera minne f�r filen!\r\n",-1);
      continue;
    }
    time(&tid);
    allokpek->tid=allokpek->validtime=tid;
    allokpek->uppladdare=inloggad;
    if(dfind(&info,tmpfullname,0)) {
      sprintf(outbuffer,"\r\nHittar inte filen %s!\r\n",tmpfullname);
      puttekn(outbuffer,-1);
      FreeMem(allokpek,sizeof(struct Fil));
      continue;
    }
    allokpek->size=info.fib_Size;
    
    sprintf(outbuffer,"\r\n\r\nFilnamn: %s", filnamn);
    puttekn(outbuffer,-1);
    puttekn("\r\nVilken status ska beh�vas f�r att ladda ner filen? (0)",-1);
    if(getstring(EKO,3,NULL)) { FreeMem(allokpek,sizeof(struct Fil)); return(1); }
    allokpek->status=atoi(inmat);
    if(Servermem->inne[nodnr].status >= Servermem->cfg.st.filer) {
      if(GetYesOrNo("\n\rSka filen valideras?", 'j', 'n', "Ja", "Nej",
                    TRUE, &isCorrect)) {
        return 1;
      }
      if(!isCorrect) {
        allokpek->flaggor|=FILE_NOTVALID;
      }
      if(GetYesOrNo("\n\rSka filen ha fri download?", 'j', 'n', "Ja", "Nej",
                    FALSE, &isCorrect)) {
        return 1;
      }
      if(isCorrect) {
        allokpek->flaggor|=FILE_FREEDL;
      }
    } else if(Servermem->cfg.cfgflags & NICFG_VALIDATEFILES) allokpek->flaggor|=FILE_NOTVALID;
    sendfile("NiKom:Texter/Nyckelhj�lp.txt");
    puttekn("\r\nVilka s�knycklar ska filen ha? (? f�r att f� en lista)\r\n",-1);
    if(editkey(allokpek->nycklar)) { FreeMem(allokpek,sizeof(struct Fil)); return(1); }
    puttekn("\r\nBeskrivning:\r\n",-1);
    if(getstring(EKO,70,NULL)) { FreeMem(allokpek,sizeof(struct Fil)); return(1); }
    strcpy(allokpek->beskr,inmat);
    dirnr = ChooseDirectoryInFileArea(
                                      area, allokpek->nycklar, allokpek->size);
    if(dirnr==-1) {
      puttekn("\r\n\nKunde inte hitta n�got l�mpligt directory f�r filen!\r\n",-1);
      DeleteFile(tmpfullname);
      FreeMem(allokpek,sizeof(struct Fil));
      continue;
    }
    allokpek->dir=dirnr;
    strcpy(allokpek->namn,filnamn);
    sprintf(inmat,"%s %s",tmpfullname,Servermem->areor[area].dir[dirnr]);
    argument=inmat;
    sendrexx(10);
    AddTail((struct List *)&Servermem->areor[area].ar_list,(struct Node *)allokpek);
    if(writefiles(area)) {
      puttekn("\r\n\nKunde inte skriva till datafilen\r\n",-1);
      
    }
    
    Servermem->inne[nodnr].upload++;
    Statstr.ul++;
    if(Servermem->cfg.logmask & LOG_UPLOAD) {
      LogEvent(USAGE_LOG, INFO, "%s laddar upp %s",
               getusername(inloggad), allokpek->namn);
    }
    if(Servermem->cfg.ar.postup1) sendautorexx(Servermem->cfg.ar.postup1);

    if(GetYesOrNo("\r\n\nVill du skriva en l�ngre beskrivning?",
                  'j', 'n', "\r\n\nOk, g�r in i editorn.\r\n", "Nej\r\n\n",
                  TRUE, &writeLong)) {
      return 1;
    }

    if(writeLong) {
      if((editret=edittext(NULL))==1) return(1);
      else if(editret==2) continue;
      sprintf(nikfilename,"%slongdesc/%s.long",Servermem->areor[area].dir[dirnr],filnamn);
      if(!(fp=fopen(nikfilename,"w"))) {
        puttekn("\r\n\nKunde inte �ppna longdesc-filen\r\n",-1);
        freeeditlist();
        continue;
      }
      for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) {
        if(fputs(el->text,fp)) {
          freeeditlist();
          fclose(fp);
          continue;
        }
        fputc('\n',fp);
      }
      freeeditlist();
      fclose(fp);
      puttekn("\r\n",-1);
      allokpek->flaggor|=FILE_LONGDESC;
      updatefile(area,allokpek);
    }
  }
  
  while(tf=(struct TransferFiles *)RemHead((struct List *)&tf_list))
    FreeMem(tf,sizeof(struct TransferFiles));
  
  return(0);
}

int recbinfile(char *dir) {
  int xprreturkod;
  char zmodeminit[100];

  ulfiles = 0;
  if(access(dir,0)) {
    puttekn("\r\nDirectoryt finns inte!\r\n",-1);
    return 2;
  }

  if(Servermem->cfg.diskfree != 0
     && !HasPartitionEnoughFreeSpace(dir, Servermem->cfg.diskfree)) {
    puttekn("\r\nTyv�rr, gr�nsen f�r hur full disken f�r bli har �verskridits!\r\n",-1);
    return 2;
  }

  if(Servermem->cfg.ar.preup2) {
    sendautorexx(Servermem->cfg.ar.preup2);
  }
  sprintf(zmodeminit,"%s%s",zinitstring,dir);
  if(!(XProtocolBase = (struct Library *) OpenLibrary("xprzmodem.library", 0L))) {
    puttekn("\r\n\nKunde inte �ppna xprzmodem.library!\r\n",-1);
    return 2;
  }
  if(!(xio = (struct XPR_IO *)
       AllocMem(sizeof(struct XPR_IO), MEMF_PUBLIC | MEMF_CLEAR))) {
    puttekn("\r\n\nKunde inte allokera en io-struktur\r\n",-1);
    CloseLibrary(XProtocolBase);
    return 2;
  }
  NewList((struct List *)&tf_list);
  
  puttekn("\r\nDu kan b�rja s�nda med Zmodem. Du kan nu skicka fler filer!",-1);
  puttekn("\r\nTryck Ctrl-X n�gra g�nger f�r att avbryta.\r\n",-1);
  AbortIO((struct IORequest *)serreadreq);
  WaitIO((struct IORequest *)serreadreq);
  AbortInactive();
  xpr_setup(xio);
  xio->xpr_filename = zmodeminit;
  XProtocolSetup(xio);
  xprreturkod = XProtocolReceive(xio);
  Delay(30);
  XProtocolCleanup(xio);
  CloseLibrary(XProtocolBase);
  if(!CheckIO((struct IORequest *)serreadreq)) {
    AbortIO((struct IORequest *)serreadreq);
    WaitIO((struct IORequest *)serreadreq);
    printf("Serreadreq avbruten!!\n");
  }
  if(!CheckIO((struct IORequest *)timerreq)) {
    AbortIO((struct IORequest *)timerreq);
    WaitIO((struct IORequest *)timerreq);
    printf("Timerreq avbruten!!\n");
  }
  FreeMem(xio,sizeof(struct XPR_IO));
  Delay(100);
  serchangereq->IOSer.io_Command=CMD_CLEAR;
  DoIO((struct IORequest *)serchangereq);
  serchangereq->IOSer.io_Command=CMD_FLUSH;
  DoIO((struct IORequest *)serchangereq);
  serreqtkn();
  UpdateInactive();
  if(Servermem->cfg.ar.postup2) {
    sendautorexx(Servermem->cfg.ar.postup2);
  }

  if(ulfiles > 0) {
    puttekn("\r\n\n�verf�ringen lyckades.\r\n",-1);
    return 0;
  }
  else {
    puttekn("\r\n\n�verf�ringen misslyckades.\r\n",-1);
    return 2;
  }
}

int sendbinfile(void) {
	struct TransferFiles *tf;
	int xprreturkod,cnt=0;
	if(!(XProtocolBase=(struct Library *)OpenLibrary("xprzmodem.library",0L)))
	{
		puttekn("\r\n\nKunde inte �ppna xprzmodem.library!\r\n",-1);
		return(2);
	}
	if(!(xio=(struct XPR_IO *)AllocMem(sizeof(struct XPR_IO),MEMF_PUBLIC | MEMF_CLEAR))) {
		puttekn("\r\n\nKunde inte allokera en io-struktur\r\n",-1);
		CloseLibrary(XProtocolBase);
		return(2);
	}
	puttekn("\r\nDu kan b�rja ta emot med Zmodem.\r\n",-1);
	puttekn("Tryck Ctrl-X n�gra g�nger f�r att avbryta.\r\n",-1);
	AbortIO((struct IORequest *)serreadreq);
	WaitIO((struct IORequest *)serreadreq);
        AbortInactive();

	xpr_setup(xio);
	xio->xpr_filename=zinitstring;
	XProtocolSetup(xio);
	xio->xpr_filename="Hejhopp";
	xprreturkod=XProtocolSend(xio);
	Delay(30);
	XProtocolCleanup(xio);
	CloseLibrary(XProtocolBase);
	if(!CheckIO((struct IORequest *)serreadreq)) {
		AbortIO((struct IORequest *)serreadreq);
		WaitIO((struct IORequest *)serreadreq);
		printf("Serreadreq avbruten!!\n");
	}
	if(!CheckIO((struct IORequest *)timerreq)) {
		AbortIO((struct IORequest *)timerreq);
		WaitIO((struct IORequest *)timerreq);
		printf("Timerreq avbruten!!\n");
	}
	FreeMem(xio,sizeof(struct XPR_IO));
	Delay(100);
	serchangereq->IOSer.io_Command=CMD_CLEAR;
	DoIO((struct IORequest *)serchangereq);
	serchangereq->IOSer.io_Command=CMD_FLUSH;
	DoIO((struct IORequest *)serchangereq);
	serreqtkn();
	UpdateInactive();
	if(Servermem->cfg.logmask & LOG_SENDFILE) {
		for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ)
			if(tf->sucess) {
                          LogEvent(USAGE_LOG, INFO, "Skickar filen %s till %s",
                                   tf->path, getusername(inloggad));
			}
	}
	for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ)
		if(tf->sucess) cnt++;
	if(cnt==1) strcpy(outbuffer,"\n\n\rF�rde �ver 1 fil.\n\n\r");
	else sprintf(outbuffer,"\n\n\rF�rde �ver %d filer.\n\n\r",cnt);
	puttekn(outbuffer,-1);
	return(0);
}
