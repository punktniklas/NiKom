#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "NiKomStr.h"
#include "Shutdown.h"
#include "Startup.h"
#include "StringUtils.h"
#include "ConfigUtils.h"

#include "Config.h"

#define ERROR	     10

 // TODO: Remove need for these prototypes
int parsegrupp(char *skri);

void initSystemConfigDefaults(void);
int handleSystemConfigStatusSection(char *line, BPTR fh);
int handleSystemConfigLine(char *line, BPTR fh);

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
  char buffer[200];

  printf("Reading System.cfg\n");
  initSystemConfigDefaults();

  if(!(fh = Open("NiKom:DatoCfg/System.cfg", MODE_OLDFILE)))
    cleanup(ERROR, "Could not open NiKom:DatoCfg/System.cfg");

  for(;;) {
    if(!FGets(fh, buffer, 199)) {
      Close(fh);
      return;
    }
    if(buffer[0] == '#' || buffer[0] == '*' || buffer[0] == '\n') {
      continue;
    }
    if(!handleSystemConfigLine(buffer, fh)) {
      Close(fh);
      cleanup(ERROR, "Invalid System.cfg");
    }
  }
}

void initSystemConfigDefaults() {
  int i;
  Servermem->cfg.defaultflags = 433;
  Servermem->cfg.defaultstatus = 0;
  Servermem->cfg.defaultrader = 25;
  for(i = 0; i <= 100; i++) {
    Servermem->cfg.maxtid[i] = 0;
    Servermem->cfg.uldlratio[i] = 0;
  }
  Servermem->cfg.cfgflags = NICFG_VALIDATEFILES | NICFG_CRYPTEDPASSWORDS;
  strcpy(Servermem->cfg.brevnamn, "Brevlådan");
  strcpy(Servermem->cfg.ny, "NY");
  Servermem->cfg.diskfree = 100000;
  strcpy(Servermem->cfg.ultmp, "T:");
  Servermem->cfg.ar.preinlogg = 0;
  Servermem->cfg.ar.postinlogg = 1;
  Servermem->cfg.ar.utlogg = 0;
  Servermem->cfg.ar.nyanv = 0;
  Servermem->cfg.ar.preup1 = 0;
  Servermem->cfg.ar.preup2 = 0;
  Servermem->cfg.ar.postup1 = 0;
  Servermem->cfg.ar.postup2 = 0;
  Servermem->cfg.ar.noright = 0;
  Servermem->cfg.ar.nextmeet = 0;
  Servermem->cfg.ar.nexttext = 0;
  Servermem->cfg.ar.nextkom = 0;
  Servermem->cfg.ar.setid = 0;
  Servermem->cfg.ar.nextletter = 0;
  Servermem->cfg.ar.cardropped = 0;
  Servermem->cfg.logmask = 4095;
  strcpy(pubscreen, "-");
  xpos = 50;
  ypos = 50;
  Servermem->cfg.logintries = 5;
  Servermem->cfg.defaultcharset = 1;
}

int handleSystemConfigStatusSection(char *line, BPTR fh) {
  int status;
  char buffer[100];

  for(;;) {
    if(!isdigit(line[6])) {
      printf("Invalid config line, no digit after 'STATUS': %s\n", line);
      return 0;
    }
    status = atoi(&line[6]);
    if(status < 0 || status > 100) {
      printf("Invalid config file, %d is not a valid status level: %s\n",
             status, line);
      return 0;
    }
    for(;;) {
      if((line = FGets(fh, buffer, 99)) == NULL) {
        printf("Invalid config file, 'ENDSTATUS' not found.\n");
        return 0;
      }
      if(line[0] == '#' || line[0] == '*' || line[0] == '\n') {
        continue;
      }
      if(StartsWith(line, "ENDSTATUS")) {
        return 1;
      } else if(StartsWith(line, "MAXTID") || StartsWith(line, "MAXTIME")) {
        if(!GetShortCfgValue(line, &Servermem->cfg.maxtid[status])) {
          return 0;
        }
      } else if(StartsWith(line, "ULDL")) {
        if(!GetCharCfgValue(line, &Servermem->cfg.uldlratio[status])) {
          return 0;
        }
      } else if(StartsWith(line, "STATUS")) {
        break;
      } else {
        printf("Invalid config line in status section: %s\n", line);
        return 0;
      }
    }
  }
}

int handleSystemConfigLine(char *line, BPTR fh) {
  int len;

  if(StartsWith(line, "DEFAULTFLAGS")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.defaultflags)) {
      return 0;
    }
  } else if(StartsWith(line, "DEFAULTSTATUS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.defaultstatus)) {
      return 0;
    }
    if(Servermem->cfg.defaultstatus < 0 || Servermem->cfg.defaultstatus > 100) {
      printf("Invalid value for DEFAULTSTATUS, must be between 0 and 100.\n");
      return 0;
    }
  } else if(StartsWith(line, "DEFAULTRADER") || StartsWith(line, "DEFAULTLINES")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.defaultrader)) {
      return 0;
    }
  } else if(StartsWith(line, "STATUS")) {
    if(!handleSystemConfigStatusSection(line, fh)) {
      return 0;
    }
  } else if(StartsWith(line, "CLOSEDBBS")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_CLOSEDBBS)) {
      return 0;
    }
  } else if(StartsWith(line, "BREVLÅDA") || StartsWith(line, "MAILBOX")) {
    if(!GetStringCfgValue(line, Servermem->cfg.brevnamn, 40)) {
      return 0;
    }
  } else if(StartsWith(line, "NY") || StartsWith(line, "NEWUSERLOGIN")) {
    if(!GetStringCfgValue(line, Servermem->cfg.ny, 20)) {
      return 0;
    }
  } else if(StartsWith(line, "DISKFREE")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.diskfree)) {
      return 0;
    }
  } else if(StartsWith(line, "ULTMP")) {
    if(!GetStringCfgValue(line, Servermem->cfg.ultmp, 98)) {
      return 0;
    }
    len = strlen(Servermem->cfg.ultmp);
    if(Servermem->cfg.ultmp[len - 1] != '/' && Servermem->cfg.ultmp[len - 1] != ':') {
      Servermem->cfg.ultmp[len] = '/';
      Servermem->cfg.ultmp[len + 1] = '\0';
    }
  } else if(StartsWith(line, "PREINLOGG") || StartsWith(line, "AREXX_PRELOGIN")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.preinlogg)) {
      return 0;
    }
  } else if(StartsWith(line, "POSTINLOGG") || StartsWith(line, "AREXX_POSTLOGIN")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.postinlogg)) {
      return 0;
    }
  } else if(StartsWith(line, "UTLOGG") || StartsWith(line, "AREXX_LOGOUT")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.utlogg)) {
      return 0;
    }
  } else if(StartsWith(line, "NYANV") || StartsWith(line, "AREXX_NEWUSER")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nyanv)) {
      return 0;
    }
  } else if(StartsWith(line, "PREUPLOAD1") || StartsWith(line, "AREXX_PREUPLOAD1")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.preup1)) {
      return 0;
    }
  } else if(StartsWith(line, "PREUPLOAD2") || StartsWith(line, "AREXX_PREUPLOAD2")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.preup2)) {
      return 0;
    }
  } else if(StartsWith(line, "POSTUPLOAD1") || StartsWith(line, "AREXX_POSTUPLOAD1")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.postup1)) {
      return 0;
    }
  } else if(StartsWith(line, "POSTUPLOAD2") || StartsWith(line, "AREXX_POSTUPLOAD2")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.postup2)) {
      return 0;
    }
  } else if(StartsWith(line, "NORIGHT") || StartsWith(line, "AREXX_NOPERMISSION")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.noright)) {
      return 0;
    }
  } else if(StartsWith(line, "NEXTMEET") || StartsWith(line, "AREXX_NEXTFORUM")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nextmeet)) {
      return 0;
    }
  } else if(StartsWith(line, "NEXTTEXT") || StartsWith(line, "AREXX_NEXTTEXT")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nexttext)) {
      return 0;
    }
  } else if(StartsWith(line, "NEXTKOM") || StartsWith(line, "AREXX_NEXTREPLY")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nextkom)) {
      return 0;
    }
  } else if(StartsWith(line, "SETID") || StartsWith(line, "AREXX_SEETIME")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.setid)) {
      return 0;
    }
  } else if(StartsWith(line, "NEXTLETTER") || StartsWith(line, "AREXX_NEXTMAIL")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nextletter)) {
      return 0;
    }
  } else if(StartsWith(line, "CARDROPPED") || StartsWith(line, "AREXX_AUTOLOGOUT")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.cardropped)) {
      return 0;
    }
  } else if(StartsWith(line, "LOGMASK")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.logmask)) {
      return 0;
    }
  } else if(StartsWith(line, "SCREEN")) {
    if(!GetStringCfgValue(line, pubscreen, 39)) {
      return 0;
    }
  } else if(StartsWith(line, "YPOS") || StartsWith(line, "WIN_YPOS")) {
    if(!GetLongCfgValue(line, (long *)&ypos)) {
      return 0;
    }
  } else if(StartsWith(line, "XPOS") || StartsWith(line, "WIN_XPOS")) {
    if(!GetLongCfgValue(line, (long *)&xpos)) {
      return 0;
    }
  } else if(StartsWith(line, "VALIDERAFILER") || StartsWith(line, "UPLOADSNOTVALIDATED")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_VALIDATEFILES)) {
      return 0;
    }
  } else if(StartsWith(line, "LOGINTRIES") || StartsWith(line, "LOGINATTEMPTS")) {
    if(!GetShortCfgValue(line, &Servermem->cfg.logintries)) {
      return 0;
    }
  } else if(StartsWith(line, "LOCALCOLOURS") || StartsWith(line, "LOCALCOLORS")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_LOCALCOLOURS)) {
      return 0;
    }
  } else if(StartsWith(line, "CRYPTEDPASSWORDS")
            || StartsWith(line, "ENCRYPTEDPASSWORDS")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_CRYPTEDPASSWORDS)) {
      return 0;
    }
  } else if(StartsWith(line, "NEWUSERCHARSET")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.defaultcharset)) {
      return 0;
    }
  } else {
    printf("Invalid config line: %s\n", line);
    return 0;
  }
  return 1;
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
				foo1 = FindNextWord(buffer);
				Servermem->nodetypes[x].nummer = atoi(foo1);
				foo1 = FindNextWord(foo1);
				foo2 = FindNextWord(foo1);
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
				foo1=FindNextWord(buffer);
				Servermem->fidodata.fd[x].nummer = atoi(foo1);
				foo1=FindNextWord(foo1);
				foo2=FindNextWord(foo1);
				foo2[-1]=0;
				strncpy(Servermem->fidodata.fd[x].domain,foo1,19);
				Servermem->fidodata.fd[x].zone=getzone(foo2);
				Servermem->fidodata.fd[x].net=getnet(foo2);
				Servermem->fidodata.fd[x].node=getnode(foo2);
				Servermem->fidodata.fd[x].point=getpoint(foo2);
				foo1=FindNextWord(foo2);
				strncpy(Servermem->fidodata.fd[x].zones,foo1,49);
			}
		}
		else if(!strncmp(buffer,"ALIAS",5)) {
			for(x=0;x<20;x++) if(!Servermem->fidodata.fa[x].namn[0]) break;
			if(x<20) {
				foo1=FindNextWord(buffer);
				Servermem->fidodata.fa[x].nummer = atoi(foo1);
				foo1=FindNextWord(foo1);
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
