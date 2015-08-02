#include <exec/types.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "NiKomBase.h"
#include "/Include/NiKomLib.h"
#include "funcs.h"

void __saveds debug_req(char *reqtext,APTR arg) {
        struct EasyStruct myeasy = {
                sizeof(struct EasyStruct),
                0,"NiKomLib Debug","","OK"
        };
        myeasy.es_TextFormat=reqtext;
        EasyRequest(NULL,&myeasy,NULL,arg);
}

int getlastmatrix(struct NiKomBase *NiKomBase) {
	char buffer[20];
	GetVar("NiKom:DatoCfg/LastMatrix",buffer,19,GVF_GLOBAL_ONLY);
	NiKomBase->lastmatrix = atoi(buffer);
	return(TRUE);
}

void __saveds __asm LIBMatrix2NiKom(register __a6 struct NiKomBase *NiKomBase) {
	struct FidoText *fidotext;
	struct FidoLine *fl;
	int going=TRUE,anv,brev;
	BPTR fh;
	char buffer[100],*foo,filnamn[20];
	struct TagItem ti = { TAG_DONE };

	if(!NiKomBase->Servermem) return;

	LIBLockNiKomBase(NiKomBase);
	while(going) {
		strcpy(buffer,NiKomBase->Servermem->fidodata.matrixdir);
		sprintf(filnamn,"%d.msg",NiKomBase->lastmatrix);
		AddPart(buffer,filnamn,99);
		if(!(fidotext = LIBReadFidoText(buffer,&ti,NiKomBase))) break;
		NiKomBase->lastmatrix++;
		if(fidotext->attribut & FIDOT_LOCAL) {
			continue;
		}
		sprintf(buffer,"Brev %d till %s (%d:%d/%d.%d)",NiKomBase->lastmatrix-1,fidotext->touser,fidotext->tozone,fidotext->tonet,
			fidotext->tonode,fidotext->topoint);
		writelog(NiKomBase->Servermem->fidodata.fidologfile,buffer);
		anv=fidoparsenamn(fidotext->touser,NiKomBase->Servermem);
		if(anv==-1) {
			sprintf(buffer,"Hittar inte %s",fidotext->touser);
			writelog(NiKomBase->Servermem->fidodata.fidologfile,buffer);
			continue;
		}
		brev=updatenextletter(anv);
		if(brev==-1) {
			continue;
		}
		sprintf(buffer,"NiKom:Users/%d/%d/%d.letter",anv/100,anv,brev);
		if(!(fh=Open(buffer,MODE_NEWFILE))) {
			continue;
		}
		FPuts(fh,"System-ID: Fido\n");
		sprintf(buffer,"From: %s (%d:%d/%d.%d)\n",fidotext->fromuser,fidotext->fromzone,fidotext->fromnet,
			fidotext->fromnode,fidotext->frompoint);
		FPuts(fh,buffer);
		sprintf(buffer,"To: %s (%d:%d/%d.%d)\n",fidotext->touser,fidotext->tozone,fidotext->tonet,
			fidotext->tonode,fidotext->topoint);
		FPuts(fh,buffer);
		sprintf(buffer,"Message-ID: %s\n",fidotext->msgid);
		FPuts(fh,buffer);
		sprintf(buffer,"Date: %s\n",fidotext->date);
		FPuts(fh,buffer);
		foo = hittaefter(fidotext->subject);
		if(fidotext->subject[0] == 0 || (fidotext->subject[0] == ' ' && foo[0] == 0)) strcpy(buffer,"Subject: -\n");
		else sprintf(buffer,"Subject: %s\n",fidotext->subject);
		FPuts(fh,buffer);
		for(fl=(struct FidoLine *)fidotext->text.mlh_Head;fl->line_node.mln_Succ;fl=(struct FidoLine *)fl->line_node.mln_Succ) {
			FPuts(fh,fl->text);
			FPutC(fh,'\n');
		}
		Close(fh);
		FreeFidoText(fidotext);
	}
	sprintf(buffer,"%d",NiKomBase->lastmatrix);
	SetVar("NiKom:DatoCfg/LastMatrix",buffer,-1,GVF_GLOBAL_ONLY);
	LIBUnLockNiKomBase(NiKomBase);
}

int sprattmatchar(char *skrivet,char *facit) {
	int mat=TRUE,count=0;
	char tmpskrivet,tmpfacit,tmpfacit2;
	if(facit!=NULL) {
		while(mat && skrivet[count]!=' ' && skrivet[count]!=0) {
			if(skrivet[count]=='*') { count++; continue; }
			tmpskrivet=ToUpper(skrivet[count]);
			tmpfacit=ToUpper(facit[count]);
			switch(tmpskrivet) {
				case '}' : case ']' : tmpskrivet='Å'; break;
				case '{' : case '[' : tmpskrivet='Ä'; break;
				case '|' : case '\\' : tmpskrivet='Ö'; break;
			}
			switch(tmpfacit) {
				case 'Å': case 'Ä' : tmpfacit2='A'; break;
				case 'Ö': tmpfacit2='O'; break;
				default : tmpfacit2=tmpfacit;
			}
			if(tmpskrivet!=tmpfacit && tmpskrivet!=tmpfacit2) mat=FALSE;
			count++;
		}
	}
	return(mat);
}


int fidoparsenamn(char *skri,struct System *Servermem) {
	int going=TRUE,going2=TRUE,found=-1,x;
	char *faci,*skri2;
	struct ShortUser *letpek;
	for(x=0;x<20;x++) {
		if(sprattmatchar(skri,Servermem->fidodata.fa[x].namn)) return(Servermem->fidodata.fa[x].nummer);
	}
	if(sprattmatchar(skri,"sysop")) return(0);
	letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;
	while(letpek->user_node.mln_Succ && going) {
		faci=letpek->namn;
		skri2=skri;
		going2=TRUE;
		if(sprattmatchar(skri2,faci)) {
			while(going2) {
				skri2=hittaefter(skri2);
				faci=hittaefter(faci);
				if(skri2[0]==0) {
					found=letpek->nummer;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!sprattmatchar(skri2,faci)) {
					faci=hittaefter(faci);
					if(faci[0]==0 || !sprattmatchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		letpek=(struct ShortUser *)letpek->user_node.mln_Succ;
	}
	return(found);
}

int updatenextletter(int user) {
	BPTR fh;
	long nr;
	char nrstr[20],filnamn[50];
	sprintf(filnamn,"NiKom:Users/%d/%d/.nextletter",user/100,user);
	if(!(fh=Open(filnamn,MODE_OLDFILE))) {
		return(-1);
	}
	memset(nrstr,0,20);
	if(Read(fh,nrstr,19)==-1) {
		Close(fh);
		return(-1);
	}
	nr=atoi(nrstr);
	sprintf(nrstr,"%d",nr+1);
	if(Seek(fh,0,OFFSET_BEGINNING)==-1) {
		Close(fh);
		return(-1);
	}
	if(Write(fh,nrstr,strlen(nrstr))==-1) {
		Close(fh);
		return(-1);
	}
	Close(fh);
	return(nr);
}

void writelog(char *file, char *text) {
	BPTR fh;
	struct DateTime dt;
	char datebuf[14],timebuf[10],utbuf[100];
	DateStamp(&dt.dat_Stamp);
	dt.dat_Format = FORMAT_INT;
	dt.dat_Flags = 0;
	dt.dat_StrDay = NULL;
	dt.dat_StrDate = datebuf;
	dt.dat_StrTime = timebuf;
	DateToStr(&dt);
	sprintf(utbuf,"%s %s - %s\n",datebuf,timebuf,text);
	if(!(fh=Open(file,MODE_OLDFILE)))
		if(!(fh=Open(file,MODE_NEWFILE))) return;
	Seek(fh,0,OFFSET_END);
	FPuts(fh,utbuf);
	Close(fh);
}
