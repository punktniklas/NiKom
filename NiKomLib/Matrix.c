#include <exec/types.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "NiKomBase.h"
#include "NiKomLib.h"
#include "Funcs.h"
#include "Logging.h"

static int getNextMsgNum(char *dir) {
  char path[100], buffer[20];
  strcpy(path, dir);
  AddPart(path, "NiKomNext", 100);
  if(GetVar(path, buffer, 19, GVF_GLOBAL_ONLY) == -1) {
    return 2;
  }
  return atoi(buffer);
}

static void setNextMsgNum(char *dir, int msgNum) {
  char path[100], buffer[20];
  strcpy(path, dir);
  AddPart(path, "NiKomNext", 100);
  sprintf(buffer, "%d", msgNum);
  SetVar(path, buffer, -1, GVF_GLOBAL_ONLY);
}

void __saveds AASM LIBMatrix2NiKom(register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  struct FidoText *fidotext;
  struct FidoLine *fl;
  int userId, mailId, currentMsgNum, oldNextMsgNum, i;
  BPTR fh;
  char buffer[100], *subject, filename[20];
  struct TagItem ti = { TAG_DONE };
  
  if(!NiKomBase->Servermem) {
    return;
  }
  
  LIBLockNiKomBase(NiKomBase);
  for(i = 0; i < 10; i++) {
    if(NiKomBase->Servermem->cfg->fidoConfig.matrixDirs[i].path[0] == '\0') {
      break;
    }
    oldNextMsgNum = getNextMsgNum(NiKomBase->Servermem->cfg->fidoConfig.matrixDirs[i].path);
    LogEvent(NiKomBase->Servermem, FIDO_LOG, DEBUG,
             "Scanning matrix dir %s with NiKomNext = %d",
             NiKomBase->Servermem->cfg->fidoConfig.matrixDirs[i].path,
             oldNextMsgNum);

    for(currentMsgNum = oldNextMsgNum;; currentMsgNum++) {
      strcpy(buffer, NiKomBase->Servermem->cfg->fidoConfig.matrixDirs[i].path);
      sprintf(filename, "%d.msg", currentMsgNum);
      AddPart(buffer, filename, 99);
      if(!(fidotext = LIBReadFidoText(buffer, &ti, NiKomBase))) {
        break;
      }
      if(fidotext->attribut & FIDOT_LOCAL) {
        LogEvent(NiKomBase->Servermem, FIDO_LOG, DEBUG,
                 "Netmail %d originates from here, skipping.", currentMsgNum);
        continue;
      }
      LogEvent(NiKomBase->Servermem,
               FIDO_LOG, INFO, "Importing netmail %d.msg for '%s' (%d:%d/%d.%d)",
               currentMsgNum,
               fidotext->touser,
               fidotext->tozone,
               fidotext->tonet,
               fidotext->tonode,
               fidotext->topoint);
      userId = fidoparsenamn(fidotext->touser, NiKomBase->Servermem);
      if(userId == -1) {
        LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
                 "User '%s' not found, skipping netmail %d.msg", fidotext->touser, currentMsgNum);
        continue;
      }
      LogEvent(NiKomBase->Servermem, FIDO_LOG, DEBUG,
               "User '%s' is NiKom user #%d.", fidotext->touser, userId);
      mailId = updatenextletter(userId);
      if(mailId == -1) {
        LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
                 "Can't update .nextletter, skipping netmail %d.msg", currentMsgNum);
        continue;
      }
      sprintf(buffer, "NiKom:Users/%d/%d/%d.letter", userId / 100, userId, mailId);
      if(!(fh = Open(buffer, MODE_NEWFILE))) {
        LogEvent(NiKomBase->Servermem, FIDO_LOG, WARN,
                 "Can't write to file %s, skipping netmail %d.msg.", buffer, currentMsgNum);
        continue;
      }
      FPuts(fh, "System-ID: Fido\n");
      sprintf(buffer, "From: %s (%d:%d/%d.%d)\n", 
              fidotext->fromuser, fidotext->fromzone, fidotext->fromnet,
              fidotext->fromnode, fidotext->frompoint);
      FPuts(fh, buffer);
      sprintf(buffer, "To: %s (%d:%d/%d.%d)\n",
              fidotext->touser, fidotext->tozone, fidotext->tonet,
              fidotext->tonode, fidotext->topoint);
      FPuts(fh, buffer);
      sprintf(buffer, "Message-ID: %s\n", fidotext->msgid);
      FPuts(fh, buffer);
      sprintf(buffer, "Date: %s\n", fidotext->date);
      FPuts(fh, buffer);
      subject = hittaefter(fidotext->subject);
      if(fidotext->subject[0] == 0 ||
         (fidotext->subject[0] == ' ' && subject[0] == 0)) {
        strcpy(buffer,"Subject: -\n");
      } else {
        sprintf(buffer,"Subject: %s\n",fidotext->subject);
      }
      FPuts(fh, buffer);
      ITER_EL(fl, fidotext->text, line_node, struct FidoLine *) {
        FPuts(fh, fl->text);
        FPutC(fh, '\n');
      }
      Close(fh);
      FreeFidoText(fidotext);
    }
    setNextMsgNum(NiKomBase->Servermem->cfg->fidoConfig.matrixDirs[i].path, currentMsgNum);
    LogEvent(NiKomBase->Servermem, FIDO_LOG, INFO,
             "Finished netmail import in %s. NiKomNext:  %d -> %d (%d new messages)",
             NiKomBase->Servermem->cfg->fidoConfig.matrixDirs[i].path,
             oldNextMsgNum, currentMsgNum, currentMsgNum - oldNextMsgNum);
  }
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
		if(sprattmatchar(skri,Servermem->cfg->fidoConfig.fa[x].namn)) return(Servermem->cfg->fidoConfig.fa[x].nummer);
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
	char nrstr[20],filename[50];
	sprintf(filename,"NiKom:Users/%d/%d/.nextletter",user/100,user);
	if(!(fh=Open(filename,MODE_OLDFILE))) {
		return(-1);
	}
	memset(nrstr,0,20);
	if(Read(fh,nrstr,19)==-1) {
		Close(fh);
		return(-1);
	}
	nr=atoi(nrstr);
	sprintf(nrstr,"%ld",nr+1);
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
