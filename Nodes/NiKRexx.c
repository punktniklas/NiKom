#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"

#define ERROR   10
#define OK              0
#define EKO             1
#define EJEKO   0
#define KOM             1
#define EJKOM   0

extern struct System *Servermem;
extern long logintime,extratime;
extern int nodnr,inloggad,senast_text_typ,senast_text_nr,senast_text_mote,
        nu_skrivs,rad,mote2,area2,rexxlogout,senast_brev_nr,senast_brev_anv,rxlinecount,radcnt;
extern char outbuffer[],inmat[],*argument,brevtxt[][81],typeaheadbuf[],xprfilnamn[],vilkabuf[];
extern struct MsgPort *rexxport;
extern struct Header readhead;
extern struct MinList edit_list,tf_list;
extern struct Inloggning Statstr;

int sendrexxrc;

int commonsendrexx(int komnr,int hasarg) {
        char macronamn[1081],*rexxargs1;
        int going = TRUE;
        struct RexxMsg *nikrexxmess,*tempmess;
        struct MsgPort *rexxmastport;

        sendrexxrc=-5;
        if(!hasarg)
                sprintf(macronamn,"NiKom:rexx/ExtKom%d %d %d",komnr,nodnr,inloggad);
        else
                sprintf(macronamn,"NiKom:rexx/ExtKom%d %d %d %s",komnr,nodnr,inloggad,argument);

        if(!(nikrexxmess=(struct RexxMsg *)CreateRexxMsg(rexxport,"nik",rexxport->mp_Node.ln_Name))) {
                puttekn("\r\n\nKunde inte allokera ett RexxMsg!\r\n\n",-1);
                return(-5);
        }
        if(!(nikrexxmess->rm_Args[0]=(STRPTR)CreateArgstring(macronamn,strlen(macronamn)))) {
                DeleteRexxMsg(nikrexxmess);
                puttekn("\r\n\nKunde inte allokera en ArgString1\r\n\n",-1);
                return(-5);
        }
        nikrexxmess->rm_Action=RXCOMM | RXFB_TOKEN;
        Forbid();
        if(!(rexxmastport=(struct MsgPort *)FindPort("REXX"))) {
                Permit();
                puttekn("\r\n\nRexxMast är inte igång!\r\n\n",-1);
                return(-5);
        }
        PutMsg(rexxmastport,(struct Message *)nikrexxmess);
        Permit();
        while(going) {
                tempmess = (struct RexxMsg *)WaitPort(rexxport);
                if(!tempmess) printf("*** Something's fishy around here... ***\n");
                while(tempmess=(struct RexxMsg *)GetMsg(rexxport)) {
                        if(tempmess->rm_Node.mn_Node.ln_Type==NT_REPLYMSG) {
                                DeleteArgstring(nikrexxmess->rm_Args[0]);
                                if(nikrexxmess->rm_Result1) {
                                        sprintf(outbuffer,"\r\n\nRexx: Return-code %d\r\n\n",nikrexxmess->rm_Result1);
                                        puttekn(outbuffer,-1);
                                }
                                DeleteRexxMsg(nikrexxmess);
                                if(!rxlinecount)
                                {
	                                rxlinecount=TRUE;
	                                radcnt = 0;
								}
                                return(sendrexxrc);
                        }
                        if(!strnicmp(tempmess->rm_Args[0],"sendstring",10)) rexxsendstring(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"sendserstring",13)) rexxsendserstring(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"getstring",9)) rexxgetstring(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"sendtextfile",12)) sendfile(rexxargs1=hittaefter(tempmess->rm_Args[0]));
                        else if(!strnicmp(tempmess->rm_Args[0],"showtext",8)) rxvisatext(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"showletter",10)) rexxvisabrev(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"lasttext",8)) senastread(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"nikcommand",10)) kommando(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"niknrcommand",12)) niknrcommand(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"edit",4)) rxedit(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"sendbinfile",11)) rexxsendbinfile(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"recbinfile",10)) rexxrecbinfile(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"getchar",7)) rexxgettekn(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"chkbuffer",9)) rexxchkbuffer(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"yesno",5)) rexxyesno(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"whicharea",9)) whicharea(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"whichmeet",9)) whichmeet(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"logout",6)) rxlogout(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"runfifo",7)) rxrunfifo(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"runrawfifo",10)) rxrunrawfifo(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"entermeet",9)) rxentermeet(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"setlinecount",12)) rxsetlinecount(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"extratime",9)) rxextratime(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"gettime",7)) rxgettime(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"sendchar",8)) rxsendchar(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"sendserchar",11)) rxsendserchar(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"setnodeaction",13)) rxsetnodeaction(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"sendrawfile",11)) rxsendrawfile(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"changelatestinfo",16)) rxchglatestinfo(tempmess);
                        else if(!strnicmp(tempmess->rm_Args[0],"getnumber",9)) rxgetnumber(tempmess);
                        else {
                                sprintf(outbuffer,"\r\n\nKan inte hantera: %s\r\n",tempmess->rm_Args[0]);
                                puttekn(outbuffer,-1);
                                tempmess->rm_Result1=5;
                                tempmess->rm_Result2=NULL;
                        }
                        ReplyMsg((struct Message *)tempmess);
                }
        }
        if(carrierdropped()) return(-8);
        return(sendrexxrc);
}

int sendrexx(int komnr) { return commonsendrexx(komnr,1); }
int sendautorexx(int komnr) { return commonsendrexx(komnr,0); }

void rexxsendstring(struct RexxMsg *mess) {
        char *str,ret,retstr[2];
        str=hittaefter(mess->rm_Args[0]);
        ret=puttekn(str,-1);
        if(ret) retstr[0]='1';
        else retstr[0]='0';
        retstr[1]=0;
        mess->rm_Result1=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(retstr,1)))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rexxsendserstring(struct RexxMsg *mess) {
        char *str,ret,retstr[2];
        str=hittaefter(mess->rm_Args[0]);
        if(Servermem->nodtyp[nodnr] != NODCON)
        	ret=serputtekn(str,-1);
        else
        	ret=conputtekn(str,-1);
        if(ret) retstr[0]='1';
        else retstr[0]='0';
        retstr[1]=0;
        mess->rm_Result1=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(retstr,1)))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rexxgetstring(struct RexxMsg *tempmess) {
        char *rexxargs1,*rexxargs2;
        char defaultstring[257];
        int ra1=0,ra2=0;

        defaultstring[0] = NULL;
        rexxargs1=hittaefter(tempmess->rm_Args[0]);
        if(rexxargs1[0])
        {
			ra2=EKO;
			if(!(ra1=atoi(rexxargs1))) ra1=50;
			rexxargs2=hittaefter(rexxargs1);
			if(rexxargs2[0] && !strncmp(rexxargs2,"NOECHO",6)) ra2=EJEKO;
			else
			{
				if(rexxargs2[0])
				{
					if(!strncmp(rexxargs2,"STARECHO",8))
						ra2 = STAREKO;
					else
						strcpy(defaultstring,rexxargs2);
				}
			}
        }
        else
        {
                ra1=50;
                ra2=EKO;
        }
        if(defaultstring[0] != NULL)
        {
                if(getstring(ra2,ra1,defaultstring))
                {
                        tempmess->rm_Result1=100;
                        tempmess->rm_Result2=NULL;
                        return;
                }
        }
        else
        {
                if(getstring(ra2,ra1,NULL)) {
                        tempmess->rm_Result1=100;
                        tempmess->rm_Result2=NULL;
                        return;
                }
        }
        tempmess->rm_Result1=0;
        if(tempmess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(tempmess->rm_Result2=(long)CreateArgstring(inmat,strlen(inmat))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void senastread(struct RexxMsg *tempmess) {
        char tempstr[20];
        tempmess->rm_Result1=0;
        if(senast_text_typ!=BREV && senast_text_typ!=TEXT) {
                tempmess->rm_Result2=NULL;
                return;
        }
        if(senast_text_typ==BREV) sprintf(tempstr,"B %d %d",senast_brev_anv,senast_brev_nr);
        else sprintf(tempstr,"T %d %d",senast_text_nr,senast_text_mote);
        if(tempmess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(tempmess->rm_Result2=(long)CreateArgstring(tempstr,strlen(tempstr))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void kommando(struct RexxMsg *tempmess) {
        int parseret;
        if((parseret=parse(hittaefter(tempmess->rm_Args[0])))==-1) {
                sprintf(outbuffer,"\r\nFelaktigt kommando: %s\r\n",hittaefter(tempmess->rm_Args[0]));
                puttekn(outbuffer,-1);
                tempmess->rm_Result1=0;
                tempmess->rm_Result2=NULL;
        } else if(parseret==-3) {
                tempmess->rm_Result1=0;
                tempmess->rm_Result2=NULL;
        } else if(parseret==-4) {
                sprintf(outbuffer,"Du har ingen rätt att utföra kommandot: %s",hittaefter(tempmess->rm_Args[0]));
                puttekn(outbuffer,-1);
                tempmess->rm_Result1=0;
                tempmess->rm_Result2=NULL;
        } else {
                if((sendrexxrc=dokmd(parseret,0))==-8) {
                        tempmess->rm_Result1=100;
                        tempmess->rm_Result2=NULL;
                } else {
                        tempmess->rm_Result1=0;
                        tempmess->rm_Result2=NULL;
                }
        }
}

void niknrcommand(struct RexxMsg *tempmess) {
        int nr;
        char *kom=hittaefter(tempmess->rm_Args[0]);
        nr=atoi(kom);
        argument=hittaefter(kom);
        if((sendrexxrc=dokmd(nr,0))==-8) {
                tempmess->rm_Result1=100;
                tempmess->rm_Result2=NULL;
        } else {
                tempmess->rm_Result1=0;
                tempmess->rm_Result2=NULL;
        }
}

void rxedit(struct RexxMsg *mess) {
        int editret,cnt,len;
        char *filnamn,retstr[2];
        BPTR fh;
        struct EditLine *el;
        nu_skrivs=ANNAT;
        filnamn=hittaefter(mess->rm_Args[0]);
        if(!filnamn[0]) {
                mess->rm_Result1=5;
                mess->rm_Result2=NULL;
                return;
        }
/*      puttekn("\r\n\n",-1); Detta måste nu fixas i ARexx-programmet
		istället */

        if((editret=edittext(filnamn))==1) {
                mess->rm_Result1=100;
                mess->rm_Result2=NULL;
                return;
        } else if(editret==0) {
                if(!(fh=Open(filnamn,MODE_NEWFILE))) {
                        sprintf(outbuffer,"\r\n\nKunde inte öppna %s för skrivning!\r\n");
                        puttekn(outbuffer,-1);
                        mess->rm_Result1=5;
                        mess->rm_Result2=NULL;
                        return;
                }
                cnt=0;
                for(el=(struct EditLine *)edit_list.mlh_Head;el->line_node.mln_Succ;el=(struct EditLine *)el->line_node.mln_Succ) {
                        len=strlen(el->text);
                        el->text[len]='\n';
                        if(Write(fh,el->text,len+1)==-1) {
                                sprintf(outbuffer,"\r\n\nFel vid skrivandet av %s!\r\n",filnamn);
                                puttekn(outbuffer,-1);
                                Close(fh);
                                mess->rm_Result1=5;
                                mess->rm_Result2=NULL;
                                return;
                        }
                        cnt++;
                }
                Close(fh);
                freeeditlist();
        }
        mess->rm_Result1=0;
        retstr[0]= editret ? '0' : '1';
        retstr[1]=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(retstr,1)))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rexxgettekn(struct RexxMsg *mess) {
        UBYTE foo[2];
        foo[0]=gettekn();
        foo[1]=0;
        if(foo[0]==10 && carrierdropped()) {
                mess->rm_Result1=100;
                mess->rm_Result2=NULL;
        } else {
                mess->rm_Result1=0;
                if(mess->rm_Action & 1L<<RXFB_RESULT) {
                        if(!(mess->rm_Result2=(long)CreateArgstring(foo,1)))
                                puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
                }
        }
}

void rexxchkbuffer(struct RexxMsg *mess) {
        char foo[5];

        sprintf(foo,"%d",checkcharbuffer());
        mess->rm_Result1=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(foo,strlen(foo))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rexxyesno(struct RexxMsg *mess)
{
        char foo[2],ja,nej,def,*pek;
        pek=hittaefter(mess->rm_Args[0]);
        if(!pek[0]) {
                ja='j'; nej='n'; def=1;
        } else {
                ja=pek[0];
                pek=hittaefter(pek);
                if(!pek[0]) {
                        nej='n'; def=1;
                } else {
                        nej=pek[0];
                        pek=hittaefter(pek);
                        if(!pek[0]) def=1;
                        else def=pek[0]-'0';
                }
        }
        foo[0]=jaellernej(ja,nej,def)+'0';
        foo[1]=0;
        if(carrierdropped()) {
                mess->rm_Result1=100;
                return;
        }
        mess->rm_Result1=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(foo,1)))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void whicharea(struct RexxMsg *mess) {
        char buf[5];
        sprintf(buf,"%d",area2);
        mess->rm_Result1=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(buf,strlen(buf))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void whichmeet(struct RexxMsg *mess) {
        char buf[5];
        sprintf(buf,"%d",mote2);
        mess->rm_Result1=0;
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(buf,strlen(buf))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rexxsendbinfile(struct RexxMsg *mess) {
        char buf[257],*pek,filtemp[257],tmp[3];
        int i,quote;
        struct TransferFiles *tf;

        mess->rm_Result1=0;
        pek=hittaefter(mess->rm_Args[0]);
        NewList((struct List *)&tf_list);
        while(pek[0]!=NULL && mess->rm_Result1!=20) {
		filtemp[0]=NULL;
		i=0; quote=0;
		while(pek[0]==' ' && pek[0]!=NULL) pek++;
		if(pek[0]=='"') { quote=1; pek++; }
		if(!pek[0]) break;
		while(((pek[0]!='"' && quote) || (pek[0]!=' ' && !quote)) && pek[0]!=NULL) {
			filtemp[i++]=pek[0];
			pek++;
		}
		if(pek[0]=='"') pek++;
		filtemp[i]=NULL;
                if(!(tf=(struct TransferFiles *)AllocMem(sizeof(struct TransferFiles),MEMF_CLEAR))) {
                        mess->rm_Result1=20;
                        mess->rm_Result2=NULL;
		} else {
			strcpy(tf->path,filtemp);
			tf->filpek=NULL;
			AddTail((struct List *)&tf_list,(struct Node *)tf);
		}
	}
        sendbinfile();
        buf[0] = NULL;
        for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ)
        {
                sprintf(tmp,"%d ",tf->sucess);
                strcat(buf,tmp);
        }
        buf[strlen(buf)-1] = NULL;

        while(tf=(struct TransferFiles *)RemHead((struct List *)&tf_list))
                FreeMem(tf,sizeof(struct TransferFiles));

        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(buf,strlen(buf))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rexxrecbinfile(struct RexxMsg *mess) {
        char buf[1024];
        struct TransferFiles *tf;

        Servermem->action[nodnr] = UPLOAD;
        Servermem->varmote[nodnr] = 0;
        Servermem->vilkastr[nodnr] = NULL;

        if(recbinfile(hittaefter(mess->rm_Args[0]))) strcpy(buf,"0");
        else
        {
                buf[0] = NULL;
                for(tf=(struct TransferFiles *)tf_list.mlh_Head;tf->node.mln_Succ;tf=(struct TransferFiles *)tf->node.mln_Succ)
                {
                        strcat(buf,FilePart(tf->Filnamn));
                        strcat(buf,(char *)" ");
                }
                buf[strlen(buf)-1] = NULL;
        }

        while(tf=(struct TransferFiles *)RemHead((struct List *)&tf_list))
                FreeMem(tf,sizeof(struct TransferFiles));

        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(buf,strlen(buf))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
}

void rxlogout(struct RexxMsg *mess) {
        rexxlogout=TRUE;
        radcnt = -174711;
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rexxvisabrev(struct RexxMsg *mess) {
        char *pek1,*pek2;
        pek1=hittaefter(mess->rm_Args[0]);
        pek2=hittaefter(pek1);
        if(!pek1[0] || !pek2[0]) {
                mess->rm_Result1=10;
                mess->rm_Result2=NULL;
        }
        visabrev(atoi(pek2),atoi(pek1));
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxrunfifo(struct RexxMsg *mess) {
        execfifo(hittaefter(mess->rm_Args[0]),TRUE);
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxrunrawfifo(struct RexxMsg *mess) {
        execfifo(hittaefter(mess->rm_Args[0]),FALSE);
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxvisatext(struct RexxMsg *mess) {
        struct Mote *motpek;
        int text,mote,type;
        char *pek;
        pek=hittaefter(mess->rm_Args[0]);
        if(!pek[0]) {
                mess->rm_Result1=10;
                mess->rm_Result2=NULL;
                return;
        }
        text=atoi(pek);
        pek=hittaefter(pek);
        if(!pek[0]) type=MOTE_ORGINAL;
        else {
                mote=atoi(pek);
                motpek=getmotpek(mote);
                if(!motpek) {
                        mess->rm_Result1=10;
                        mess->rm_Result2=NULL;
                        return;
                }
                type=motpek->type;
        }
        if(type==MOTE_ORGINAL) org_visatext(text);
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxentermeet(struct RexxMsg *mess) {
        int motnr;
        motnr=atoi(hittaefter(mess->rm_Args[0]));
        if(!getmotpek(motnr)) {
                mess->rm_Result1=10;
                mess->rm_Result2=NULL;
                return;
        }
        sendrexxrc=motnr;
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxsetlinecount(struct RexxMsg *mess) {
        char *arg=hittaefter(mess->rm_Args[0]);
        if(!stricmp(arg,"ON"))
        {
        	rxlinecount=TRUE;
        	radcnt = 0;
        }
        else
        	if(!stricmp(arg,"OFF"))
        	{
        		rxlinecount=FALSE;
        		radcnt = 0;
        	}
        else {
                mess->rm_Result1=10;
                mess->rm_Result2=NULL;
                return;
        }
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxextratime(struct RexxMsg *mess) {
        char buf[10],*arg=hittaefter(mess->rm_Args[0]);
        if(!strcmp(arg,"GET")) {
                sprintf(buf,"%d",extratime);
                if(mess->rm_Action & 1L<<RXFB_RESULT) {
                        if(!(mess->rm_Result2=(long)CreateArgstring(buf,strlen(buf))))
                                puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
                }
        } else {
                extratime=atoi(arg);
                mess->rm_Result2=NULL;
        }
        mess->rm_Result1=0;
}

void rxgettime(struct RexxMsg *mess) {
        long tidgrans,tidkvar,tid;
        char buf[10];
        if(Servermem->cfg.maxtid[Servermem->inne[nodnr].status]) {
                time(&tid);
                tidgrans=60*(Servermem->cfg.maxtid[Servermem->inne[nodnr].status])+extratime;
                tidkvar=tidgrans-(tid-logintime);
        } else tidkvar=0;
        sprintf(buf,"%d",tidkvar);
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(buf,strlen(buf))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
        mess->rm_Result1=0;
}

void rxsendchar(struct RexxMsg *mess) {
        char *arg;
        arg=hittaefter(mess->rm_Args[0]);
        if(arg[0]!='/' || arg[1]==0) {
                mess->rm_Result1=10;
                mess->rm_Result2=NULL;
                return;
        }
        eka(arg[1]);
        if(carrierdropped()) mess->rm_Result1=100;
        else mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxsendserchar(struct RexxMsg *mess) {
        char *arg;
        arg=hittaefter(mess->rm_Args[0]);
        if(arg[0]!='/' || arg[1]==0) {
                mess->rm_Result1=10;
                mess->rm_Result2=NULL;
                return;
        }
        sereka(arg[1]);
        if(carrierdropped()) mess->rm_Result1=100;
        else mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxsetnodeaction(struct RexxMsg *mess) {
        strncpy(vilkabuf,hittaefter(mess->rm_Args[0]),49);
        vilkabuf[49]=0;
        Servermem->action[nodnr] = GORNGTANNAT;
        Servermem->vilkastr[nodnr] = vilkabuf;
        mess->rm_Result1=0;
        mess->rm_Result2=NULL;
}

void rxsendrawfile(struct RexxMsg *mess) {
        BPTR fh;
        int antal;
        char retstr[2] = "x", *filnamn = hittaefter(mess->rm_Args[0]);
        if(!(fh = Open(filnamn,MODE_OLDFILE))) {
                sprintf(outbuffer,"\n\n\rKan inte öppna filen %s\n\r",filnamn);
                puttekn(outbuffer,-1);
                retstr[0]='0';
        } else {
                while(antal = Read(fh,outbuffer,99)) {
                        if(antal == -1) break;
                        outbuffer[antal] = 0;
                        putstring(outbuffer,-1,0);
                }
                Close(fh);
                retstr[0] = '1';
        }
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(retstr,1)))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
        mess->rm_Result1=0;
}

void rxchglatestinfo(struct RexxMsg *mess) {
        char *what,retstr[15];
        int ammount,retval;
        what = hittaefter(mess->rm_Args[0]);
        ammount = atoi(hittaefter(what));
        switch(what[0]) {
                case 'r' : case 'R' :
                        retval = Statstr.read += ammount;
                        break;
                case 'w' : case 'W' :
                        retval = Statstr.write += ammount;
                        break;
                case 'd' : case 'D' :
                        retval = Statstr.dl += ammount;
                        break;
                case 'u' : case 'U' :
                        retval = Statstr.ul += ammount;
                        break;
                default:
                        retval = -1;
        }
        sprintf(retstr,"%d",retval);
        if(mess->rm_Action & 1L<<RXFB_RESULT) {
                if(!(mess->rm_Result2=(long)CreateArgstring(retstr,strlen(retstr))))
                        puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
        }
        mess->rm_Result1=0;
}

void rxgetnumber(struct RexxMsg *mess)
{
	int minvarde, maxvarde, defaultvarde, returvarde;
	char retstr[15];
	char *pek1 = NULL, *pek2 = NULL, *pek3 = NULL;

	pek1=hittaefter(mess->rm_Args[0]);
	if(pek1)
		pek2=hittaefter(pek1);
	if(pek2)
		pek3=hittaefter(pek2);

	if(pek1[0] && !pek2[0])
	{
		defaultvarde = atoi(pek1);
		returvarde = getnumber(0, 0, &defaultvarde);
	}
	else if(pek1[0] && pek2[0])
	{
		minvarde = atoi(pek1);
		maxvarde = atoi(pek2);
		if(pek3[0])
		{
			defaultvarde = atoi(pek3);
			returvarde = getnumber(&minvarde, &maxvarde, &defaultvarde);
		}
		else
		{
			returvarde = getnumber(&minvarde, &maxvarde, 0);
		}

	}
	else
	{
		returvarde = getnumber(0, 0, 0);
	}

	sprintf(retstr,"%d",returvarde);
	if(mess->rm_Action & 1L<<RXFB_RESULT)
	{
		if(!(mess->rm_Result2=(long)CreateArgstring(retstr,strlen(retstr))))
			puttekn("\r\n\nKunde inte allokera en Argstring!\r\n\n",-1);
	}
	mess->rm_Result1=0;

}
