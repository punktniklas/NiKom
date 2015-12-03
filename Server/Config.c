#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "NiKomStr.h"
#include "Shutdown.h"
#include "Startup.h"

#include "Config.h"

#define ERROR	     10

 // TODO: Remove need for these prototypes
int parsegrupp(char *skri);
char *hittaefter(char *);

int InitLegacyConversionData(void) {
  BPTR file;
  char line[100];
  int putsRes = 0;

  Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted = -1;

  if(!(file = Open("NiKom:DatoCfg/LegacyConversion.dat", MODE_READWRITE))) {
    return 0;
  }
  while(FGets(file, line, 100)) {
    if(strncmp(line, "LowTextWhenBitmap0ConversionStarted", 35) == 0) {
      Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted =
        atoi(&line[36]);
    }
  }

  if(Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted == -1) {
    Servermem->legacyConversionData.lowTextWhenBitmap0ConversionStarted =
      Servermem->info.lowtext;
    sprintf(line, "LowTextWhenBitmap0ConversionStarted=%d\n",
            Servermem->info.lowtext);
    putsRes = FPuts(file, line);
  }
  Close(file);

  printf("Read LegacyConversion.dat\n");

  return putsRes == 0;
}

int getcfgfilestring(char *str,BPTR fh,char *vart) {
	char buffer[100];
	for(;;) {
		if(!FGets(fh,buffer,99)) {
			printf("Korrupt fil, hittade inte %s\n",str);
			return(1);
		}
		if(!(strncmp(buffer,str,strlen(str)))) {
			strcpy(vart,&buffer[strlen(str)+1]);
			if(vart[strlen(vart)-1]=='\n') vart[strlen(vart)-1]=0;
			return(0);
		}
	}
}

void ReadSystemConfig(void) {
	BPTR fh;
	char buffer[100];
	int status,len;
	struct SpecialLogin *tempnode;
	NewList((struct List *)&Servermem->special_login);
	if(!(fh=Open("NiKom:DatoCfg/System.cfg",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/System.cfg");
	if(getcfgfilestring("DEFAULTFLAGS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.defaultflags=atoi(buffer);
	if(getcfgfilestring("DEFAULTSTATUS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.defaultstatus=atoi(buffer);
	if(getcfgfilestring("DEFAULTRADER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.defaultrader=atoi(buffer);
	for(;;) {
		if(!FGets(fh,buffer,99)) {
			printf("Korrupt fil, hittar inte ENDSTATUS\n");
			Close(fh);
			return;
		}
		if(!strncmp(buffer,"ENDSTATUS",9)) break;
		if(!strncmp(buffer,"STATUS",6)) {
			status=atoi(&buffer[6]);
			if(status<0 || status>100) {
				printf("Man kan inte ha %d som status!\n",status);
				Close(fh);
				return;
			}
			if(getcfgfilestring("MAXTID",fh,buffer)) {
				Close(fh);
				return;
			} else Servermem->cfg.maxtid[status]=atoi(buffer);
			if(getcfgfilestring("ULDL",fh,buffer)) {
				Close(fh);
				return;
			} else Servermem->cfg.uldlratio[status]=atoi(buffer);
			if(getcfgfilestring("INAKTIV",fh,buffer)) {
				Close(fh);
				return;
			} else Servermem->cfg.inaktiv[status]=atoi(buffer);
		}
	}
	if(getcfgfilestring("CLOSEDBBS",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(!(strncmp(buffer,"JA",2))) Servermem->cfg.cfgflags |= NICFG_CLOSEDBBS;
		else Servermem->cfg.cfgflags &= ~NICFG_CLOSEDBBS;
	}
	if(getcfgfilestring("BREVLÅDA",fh,buffer)) {
		Close(fh);
		return;
	} else {
		strncpy(Servermem->cfg.brevnamn,buffer,40);
	}
	if(getcfgfilestring("NY",fh,buffer)) {
		Close(fh);
		return;
	} else {
		strncpy(Servermem->cfg.ny,buffer,20);
	}
	if(getcfgfilestring("DISKFREE",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.diskfree=atoi(buffer);
	if(getcfgfilestring("ULTMP",fh,buffer)) {
		Close(fh);
		return;
	} else {
		len=strlen(buffer);
		if(buffer[len-1]!='/' && buffer[len]!=':') {
			buffer[len]='/';
			buffer[len+1]=0;
		}
		strncpy(Servermem->cfg.ultmp,buffer,99);
	}
	if(getcfgfilestring("PREINLOGG",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.preinlogg=atoi(buffer);
	if(getcfgfilestring("POSTINLOGG",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.postinlogg=atoi(buffer);
	if(getcfgfilestring("UTLOGG",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.utlogg=atoi(buffer);
	if(getcfgfilestring("NYANV",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nyanv=atoi(buffer);
	if(getcfgfilestring("PREUPLOAD1",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.preup1=atoi(buffer);
	if(getcfgfilestring("PREUPLOAD2",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.preup2=atoi(buffer);
	if(getcfgfilestring("POSTUPLOAD1",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.postup1=atoi(buffer);
	if(getcfgfilestring("POSTUPLOAD2",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.postup2=atoi(buffer);
	if(getcfgfilestring("NORIGHT",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.noright=atoi(buffer);
	if(getcfgfilestring("NEXTMEET",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nextmeet=atoi(buffer);
	if(getcfgfilestring("NEXTTEXT",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nexttext=atoi(buffer);
	if(getcfgfilestring("NEXTKOM",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nextkom=atoi(buffer);
	if(getcfgfilestring("SETID",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.setid=atoi(buffer);
	if(getcfgfilestring("NEXTLETTER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.nextletter=atoi(buffer);
	if(getcfgfilestring("CARDROPPED",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.ar.cardropped=atoi(buffer);
	for(;;) {
		if(getcfgfilestring("SPECIALLOGIN",fh,buffer)) {
			Close(fh);
			return;
		}
		if(!strnicmp(buffer,"END",3)) break;
		if(!(tempnode=(struct SpecialLogin *)AllocMem(sizeof(struct SpecialLogin),MEMF_PUBLIC|MEMF_CLEAR)))
			cleanup(ERROR,"Kunde inte allokera en SpecialLogin-struktur\n");
		tempnode->bokstav=buffer[0];
		tempnode->rexxprg=atoi(&buffer[1]);
		AddTail((struct List *)&Servermem->special_login,(struct Node *)tempnode);
	}
	if(getcfgfilestring("LOGMASK",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.logmask=atoi(buffer);
	if(getcfgfilestring("SCREEN",fh,buffer)) {
		Close(fh);
		return;
	} else strcpy(pubscreen,buffer);
	if(getcfgfilestring("YPOS",fh,buffer)) {
		Close(fh);
		return;
	} else ypos=atoi(buffer);
	if(getcfgfilestring("XPOS",fh,buffer)) {
		Close(fh);
		return;
	} else xpos=atoi(buffer);
	if(getcfgfilestring("VALIDERAFILER",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_VALIDATEFILES;
		else Servermem->cfg.cfgflags &= ~NICFG_VALIDATEFILES;
	}
	if(getcfgfilestring("LOGINTRIES",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.logintries=atoi(buffer);
	if(getcfgfilestring("LOCALCOLOURS",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_LOCALCOLOURS;
		else Servermem->cfg.cfgflags &= ~NICFG_LOCALCOLOURS;
	}

	if(getcfgfilestring("CRYPTEDPASSWORDS",fh,buffer)) {
		Close(fh);
		return;
	} else {
		if(buffer[0]=='J' || buffer[0]=='j') Servermem->cfg.cfgflags |= NICFG_CRYPTEDPASSWORDS;
		else Servermem->cfg.cfgflags &= ~NICFG_CRYPTEDPASSWORDS;
	}
	if(getcfgfilestring("NEWUSERCHARSET", fh, buffer)) {
          Close(fh);
          return;
	} else Servermem->cfg.defaultcharset = atoi(buffer);
	Close(fh);
	printf("System.cfg inläst\n");
}

void ReadCommandConfig(void) {
	BPTR fh;
	struct Kommando *allok;
	char buffer[100],*pekare;
	int x=0, going=TRUE, grupp, len;

        FreeCommandMem();

	if(!(fh=Open("NiKom:DatoCfg/Kommandon.cfg",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna NiKom:DatoCfg/Kommandon.cfg\n");
	buffer[0]=0;
	while(buffer[0]!='N') {
		if(!(pekare=FGets(fh,buffer,99))) {
			if(!IoErr()) {
				Close(fh);
				printf("Inga kommandon hittade i Kommandon.cfg\n");
				return;
			} else {
				printf("Error while reading Kommandon.cfg\n");
				Close(fh);
				return;
			}
		}
	}
	len = strlen(buffer);
	if(buffer[len - 1] == '\n') buffer[len - 1] = 0;
	if(!(allok=(struct Kommando *)AllocMem(sizeof(struct Kommando),MEMF_CLEAR | MEMF_PUBLIC)))
		cleanup(ERROR,"Kunde inte allokera minne till en komandostruktur\n");
	AddTail((struct List *)&Servermem->kom_list,(struct Node *)allok);
	strncpy(allok->namn,&buffer[2],30);
	x++;
	while(going) {
		if(!(pekare=FGets(fh,buffer,99))) {
			if(!IoErr()) break;
			else {
				printf("Error while reading Kommandon.cfg\n");
				Close(fh);
				return;
			}
		}
		len = strlen(buffer);
		if(buffer[len - 1] == '\n') buffer[len - 1] = 0;
		switch(buffer[0]) {
			case '#' :
				allok->nummer=atoi(&buffer[2]);
				break;
			case 'O' :
				allok->antal_ord=atoi(&buffer[2]);
				break;
			case 'A' :
				if(buffer[2]=='#') allok->argument=KOMARGNUM;
				else allok->argument=KOMARGCHAR;
				break;
			case 'S' :
				allok->status=atoi(&buffer[2]);
				break;
			case 'L' :
				allok->minlogg=atoi(&buffer[2]);
				break;
			case 'D' :
				allok->mindays=atoi(&buffer[2]);
				break;
			case 'X' :
				strncpy(allok->losen,&buffer[2],19);
				break;
			case 'V' :
				if(buffer[2]=='N' || buffer[2]=='n') allok->secret=TRUE;
				break;
			case 'B' :
				strncpy(allok->beskr,&buffer[2],68);
				break;
			case 'W' :
				strncpy(allok->vilkainfo,&buffer[2],49);
				break;
			case 'F' :
				strncpy(allok->logstr,&buffer[2],49);
				break;
			case '(' :
				allok->before=atoi(&buffer[2]);
				break;
			case ')' :
				allok->after=atoi(&buffer[2]);
				break;
			case 'M' :
				strncpy(allok->menu,&buffer[2],49);
				break;
			case 'G' :
				grupp = parsegrupp(&buffer[2]);
				if(grupp == -1) printf("I Kommandon.cfg: Okänd grupp %s\n",&buffer[2]);
				else BAMSET((char *)&allok->grupper,grupp);
				break;
			case 'N' :
				if(!(allok=(struct Kommando *)AllocMem(sizeof(struct Kommando),MEMF_CLEAR | MEMF_PUBLIC)))
					cleanup(ERROR,"Kunde inte allokera minne till en komandostruktur\n");
				AddTail((struct List *)&Servermem->kom_list,(struct Node *)allok);
				strncpy(allok->namn,&buffer[2],30);
				x++;
				break;
		}
	}
	printf("Kommandon.cfg inläst (%d kommandon)\n",x);
	Close(fh);
	Servermem->info.kommandon=x;
}

void FreeCommandMem(void) {
  struct Kommando *command;
  while((command = (struct Kommando *)RemHead((struct List *)&Servermem->kom_list))) {
    FreeMem(command, sizeof(struct Kommando));
  }
}

void ReadFileKeyConfig(void) {
	BPTR fh;
	char buffer[100],*pekare;
	int x=0;
	if(!(fh=Open("NiKom:DatoCfg/Nycklar.cfg",MODE_OLDFILE)))
		cleanup(ERROR,"Kunde inte öppna Nycklar.cfg\n");
	while(pekare=FGets(fh,buffer,99)) {
		if(x >= MAXNYCKLAR) break;
		if(buffer[0]!=10 && buffer[0]!='*') {
			strncpy(Servermem->Nyckelnamn[x],buffer,40);
			x++;
		}
	}
	if(IoErr()) printf("Fel vid läsandet av Nycklar.cfg\n");
	else printf("Nycklar.cfg inläst (%d)\n",x);
	Close(fh);
	Servermem->info.nycklar=x;
}

void ReadStatusConfig(void) {
	BPTR fh;
	char buffer[100];
	if(!(fh=Open("NiKom:DatoCfg/Status.cfg",MODE_OLDFILE))) {
		printf("Kunde inte öppna NiKom:DatoCfg/Status.cfg");
		return;
	}
	if(getcfgfilestring("SKRIV",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.skriv=atoi(buffer);
	if(getcfgfilestring("TEXTER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.texter=atoi(buffer);
	if(getcfgfilestring("BREV",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.brev=atoi(buffer);
	if(getcfgfilestring("MEDMÖTEN",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.medmoten=atoi(buffer);
	if(getcfgfilestring("RADMÖTEN",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.radmoten=atoi(buffer);
	if(getcfgfilestring("SESTATUS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.sestatus=atoi(buffer);
	if(getcfgfilestring("ANVÄNDARE",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.anv=atoi(buffer);
	if(getcfgfilestring("ÄNDSTATUS",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.chgstatus=atoi(buffer);
	if(getcfgfilestring("LÖSEN",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.psw=atoi(buffer);
	if(getcfgfilestring("BYTAREA",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.bytarea=atoi(buffer);
	if(getcfgfilestring("RADAREA",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.radarea=atoi(buffer);
	if(getcfgfilestring("FILER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.filer=atoi(buffer);
	if(getcfgfilestring("LADDANER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.laddaner=atoi(buffer);
	if(getcfgfilestring("GRUPPER",fh,buffer)) {
		Close(fh);
		return;
	} else Servermem->cfg.st.grupper=atoi(buffer);
	Close(fh);
	printf("Status.cfg inläst\n");
}

void ReadNodeTypesConfig(void) {
	BPTR fh;
	int x,y;
	char buffer[100],*foo1,*foo2;
	if(!(fh=Open("NiKom:DatoCfg/NodeTypes.cfg",MODE_OLDFILE))) {
		printf("Kunde inte läsa in NodeTypes.cfg\n");
		return;
	}
	for(x=0;x<MAXNODETYPES;x++) Servermem->nodetypes[x].nummer=0;
	printf("Läser in NodeTypes.cfg\n");
	while(FGets(fh,buffer,99)) {
		x=strlen(buffer);
		if(buffer[x-1]=='\n') buffer[x-1]=0;
		if(!strncmp(buffer,"NODETYPE",6)) {
			for(x=0;x<MAXNODETYPES;x++) if(!Servermem->nodetypes[x].nummer) break;
			if(x<MAXNODETYPES) {
				foo1=hittaefter(buffer);
				Servermem->nodetypes[x].nummer = atoi(foo1);
				foo1=hittaefter(foo1);
				foo2=hittaefter(foo1);
				for(y=-1; foo2[y]==' ' && &foo2[y] > foo1; y--) foo2[y]=0;
				strcpy(Servermem->nodetypes[x].path,foo1);
				strcpy(Servermem->nodetypes[x].desc,foo2);
			}
		}
	}
	Close(fh);
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

void ReadFidoConfig(void) {
	BPTR fh;
	int x;
	char buffer[100],*foo1,*foo2;
	if(!(fh=Open("NiKom:DatoCfg/NiKomFido.cfg",MODE_OLDFILE))) {
		printf("Kunde inte läsa in NiKomFido.cfg\n");
		return;
	}
	printf("Läser in NiKomFido.cfg\n");
	for(x = 0; x < 10; x++) Servermem->fidodata.fd[x].domain[0] = 0;
	for(x = 0; x < 20; x++) Servermem->fidodata.fa[x].namn[0] = 0;
	Servermem->fidodata.mailgroups = 0;

	while(FGets(fh,buffer,99)) {
		x=strlen(buffer);
		if(buffer[x-1]=='\n') buffer[x-1]=0;
		if(!strncmp(buffer,"DOMAIN",6)) {
			for(x=0;x<10;x++) if(!Servermem->fidodata.fd[x].domain[0]) break;
			if(x<10) {
				foo1=hittaefter(buffer);
				Servermem->fidodata.fd[x].nummer = atoi(foo1);
				foo1=hittaefter(foo1);
				foo2=hittaefter(foo1);
				foo2[-1]=0;
				strncpy(Servermem->fidodata.fd[x].domain,foo1,19);
				Servermem->fidodata.fd[x].zone=getzone(foo2);
				Servermem->fidodata.fd[x].net=getnet(foo2);
				Servermem->fidodata.fd[x].node=getnode(foo2);
				Servermem->fidodata.fd[x].point=getpoint(foo2);
				foo1=hittaefter(foo2);
				strncpy(Servermem->fidodata.fd[x].zones,foo1,49);
			}
		}
		else if(!strncmp(buffer,"ALIAS",5)) {
			for(x=0;x<20;x++) if(!Servermem->fidodata.fa[x].namn[0]) break;
			if(x<20) {
				foo1=hittaefter(buffer);
				Servermem->fidodata.fa[x].nummer = atoi(foo1);
				foo1=hittaefter(foo1);
				strncpy(Servermem->fidodata.fa[x].namn,foo1,35);
			}
		}
		else if(!strncmp(buffer,"BOUNCE",6)) {
			if(buffer[7]=='Y' || buffer[7]=='y') Servermem->fidodata.bounce=TRUE;
		}
		else if(!strncmp(buffer,"MATRIXDIR",9)) strncpy(Servermem->fidodata.matrixdir,&buffer[10],99);
		else if(!strncmp(buffer,"MAILGROUP",9)) {
			x=parsegrupp(&buffer[10]);
			if(x==-1) printf("I NiKomFido.cfg: Okänd grupp %s\n",&buffer[10]);
			else BAMSET((char *)&Servermem->fidodata.mailgroups,x);
		}
		else if(!strncmp(buffer,"MAILSTATUS",10)) Servermem->fidodata.mailstatus=atoi(&buffer[11]);
		else if(!strncmp(buffer,"DEFAULTORIGIN",13)) strncpy(Servermem->fidodata.defaultorigin,&buffer[14],69);
		else if(!strncmp(buffer,"CRASHSTATUS",11)) Servermem->fidodata.crashstatus = atoi(&buffer[12]);
	}
	Close(fh);
}
