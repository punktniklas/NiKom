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
#include "StringUtils.h"
#include "ConfigUtils.h"
#include "FidoUtils.h"

#include "Config.h"

#define EXIT_ERROR	     10
#define WHITESPACE " \t\n\r"

 // TODO: Remove need for these prototypes
int parsegrupp(char *skri);

void initSystemConfigDefaults(void);
int handleSystemConfigStatusSection(char *line, BPTR fh);
int handleSystemConfigLine(char *line, BPTR fh);
int handleStatusConfigLine(char *line, BPTR fh);
int handleFidoConfigLine(char *line, BPTR fh);
int handleCommandConfigLine(char *line, struct Kommando *command);

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

void readConfigFile(char *filename, int (*handleLine)(char *, BPTR)) {
  BPTR fh;
  char buffer[200], *tmp;
  int len;

  printf("Reading %s\n", filename);

  if(!(fh = Open(filename, MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Could not open config file.");
  }

  for(;;) {
    if(!FGets(fh, buffer, 199)) {
      Close(fh);
      return;
    }
    if(buffer[0] == '#' || buffer[0] == '*') {
      continue;
    }
    if(IzSpace(buffer[0])) {
      for(tmp = buffer; *tmp != '\0' && IzSpace(*tmp); tmp++);
      if(*tmp == '\0') {
        continue; // The line is all white space
      }
    }
    len = strlen(buffer);
    if(buffer[len - 1] == '\n') {
      buffer[len - 1] = '\0';
    }
    if(!handleLine(buffer, fh)) {
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid config file.");
    }
  }
}

int isMatchingConfigLine(char *line, char *keyword) {
  int i;
  for(i = 0;; i++) {
    if(line[i] == '\0') {
      return 0;
    }
    if(keyword[i] == '\0') {
      return line[i] == '=' || line[i] == ' ' || line[i] == '\t';
    }
    if(line[i] != keyword[i]) {
      return 0;
    }
  }
}

void ReadSystemConfig(void) {
  initSystemConfigDefaults();
  readConfigFile("NiKom:DatoCfg/System.cfg", handleSystemConfigLine);
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
    if(!IzDigit(line[6])) {
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
      } else if(isMatchingConfigLine(line, "MAXTID") || isMatchingConfigLine(line, "MAXTIME")) {
        if(!GetShortCfgValue(line, &Servermem->cfg.maxtid[status])) {
          return 0;
        }
      } else if(isMatchingConfigLine(line, "ULDL")) {
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

  if(isMatchingConfigLine(line, "DEFAULTFLAGS")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.defaultflags)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DEFAULTSTATUS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.defaultstatus)) {
      return 0;
    }
    if(Servermem->cfg.defaultstatus < 0 || Servermem->cfg.defaultstatus > 100) {
      printf("Invalid value for DEFAULTSTATUS, must be between 0 and 100.\n");
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DEFAULTRADER") || isMatchingConfigLine(line, "DEFAULTLINES")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.defaultrader)) {
      return 0;
    }
  } else if(StartsWith(line, "STATUS")) {
    if(!handleSystemConfigStatusSection(line, fh)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CLOSEDBBS")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_CLOSEDBBS)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "BREVLÅDA") || isMatchingConfigLine(line, "MAILBOX")) {
    if(!GetStringCfgValue(line, Servermem->cfg.brevnamn, 40)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NY") || isMatchingConfigLine(line, "NEWUSERLOGIN")) {
    if(!GetStringCfgValue(line, Servermem->cfg.ny, 20)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DISKFREE")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.diskfree)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "ULTMP")) {
    if(!GetStringCfgValue(line, Servermem->cfg.ultmp, 98)) {
      return 0;
    }
    len = strlen(Servermem->cfg.ultmp);
    if(Servermem->cfg.ultmp[len - 1] != '/' && Servermem->cfg.ultmp[len - 1] != ':') {
      Servermem->cfg.ultmp[len] = '/';
      Servermem->cfg.ultmp[len + 1] = '\0';
    }
  } else if(isMatchingConfigLine(line, "PREINLOGG") || isMatchingConfigLine(line, "AREXX_PRELOGIN")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.preinlogg)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "POSTINLOGG") || isMatchingConfigLine(line, "AREXX_POSTLOGIN")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.postinlogg)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "UTLOGG") || isMatchingConfigLine(line, "AREXX_LOGOUT")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.utlogg)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NYANV") || isMatchingConfigLine(line, "AREXX_NEWUSER")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nyanv)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "PREUPLOAD1") || isMatchingConfigLine(line, "AREXX_PREUPLOAD1")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.preup1)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "PREUPLOAD2") || isMatchingConfigLine(line, "AREXX_PREUPLOAD2")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.preup2)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "POSTUPLOAD1") || isMatchingConfigLine(line, "AREXX_POSTUPLOAD1")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.postup1)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "POSTUPLOAD2") || isMatchingConfigLine(line, "AREXX_POSTUPLOAD2")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.postup2)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NORIGHT") || isMatchingConfigLine(line, "AREXX_NOPERMISSION")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.noright)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTMEET") || isMatchingConfigLine(line, "AREXX_NEXTFORUM")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nextmeet)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTTEXT") || isMatchingConfigLine(line, "AREXX_NEXTTEXT")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nexttext)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTKOM") || isMatchingConfigLine(line, "AREXX_NEXTREPLY")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nextkom)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "SETID") || isMatchingConfigLine(line, "AREXX_SEETIME")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.setid)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTLETTER") || isMatchingConfigLine(line, "AREXX_NEXTMAIL")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.nextletter)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CARDROPPED") || isMatchingConfigLine(line, "AREXX_AUTOLOGOUT")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.ar.cardropped)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LOGMASK")) {
    if(!GetLongCfgValue(line, &Servermem->cfg.logmask)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "SCREEN")) {
    if(!GetStringCfgValue(line, pubscreen, 39)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "YPOS") || isMatchingConfigLine(line, "WIN_YPOS")) {
    if(!GetLongCfgValue(line, (long *)&ypos)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "XPOS") || isMatchingConfigLine(line, "WIN_XPOS")) {
    if(!GetLongCfgValue(line, (long *)&xpos)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "VALIDERAFILER") || isMatchingConfigLine(line, "UPLOADSNOTVALIDATED")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_VALIDATEFILES)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LOGINTRIES") || isMatchingConfigLine(line, "LOGINATTEMPTS")) {
    if(!GetShortCfgValue(line, &Servermem->cfg.logintries)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LOCALCOLOURS") || isMatchingConfigLine(line, "LOCALCOLORS")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_LOCALCOLOURS)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CRYPTEDPASSWORDS")
            || isMatchingConfigLine(line, "ENCRYPTEDPASSWORDS")) {
    if(!GetBoolCfgFlag(line, &Servermem->cfg.cfgflags, NICFG_CRYPTEDPASSWORDS)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEWUSERCHARSET")) {
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
  struct Kommando *command = NULL;
  char buffer[100];
  int cnt = 0;
  
  FreeCommandMem();
  
  if(!(fh = Open("NiKom:DatoCfg/Kommandon.cfg", MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Kunde inte öppna NiKom:DatoCfg/Kommandon.cfg\n");
  }

  while(FGets(fh, buffer, 100)) {
    if(buffer[0] == '\n' || buffer[0] == '*') {
      continue;
    }
    if(buffer[0] == 'N') {
      if(!(command = (struct Kommando *)AllocMem(sizeof(struct Kommando),
                                                 MEMF_CLEAR | MEMF_PUBLIC))) {
        Close(fh);
        cleanup(EXIT_ERROR, "Out of memory while reading Kommandon.cfg\n");
      }
      buffer[32] = '\0';
      if(!GetStringCfgValue(buffer, command->namn, 30)) {
        Close(fh);
        cleanup(EXIT_ERROR, "Invalid Kommandon.cfg");
      }
      AddTail((struct List *)&Servermem->kom_list, (struct Node *)command);
      cnt++;
      continue;
    }

    if(command == NULL) {
      printf("Found command detail line before command start (\"N=\"): %s\n", buffer);
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid Kommandon.cfg");
    }
    if(!handleCommandConfigLine(buffer, command)) {
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid Kommandon.cfg");
    }
  }
  Close(fh);
  Servermem->info.kommandon = cnt;
  printf("Read Kommandon.cfg, %d commands\n", cnt);
}

int handleCommandConfigLine(char *line, struct Kommando *command) {
  char tmp[100];
  int group;

  switch(line[0]) {
  case '#' :
    if(!GetLongCfgValue(line, &command->nummer)) {
      return 0;
    }
    break;
  case 'O' :
    if(!GetCharCfgValue(line, &command->antal_ord)) {
      return 0;
    }
    break;
  case 'A' :
    if(!GetStringCfgValue(line, tmp, 9)) {
      return 0;
    }
    command->argument = tmp[0] == '#' ? KOMARGNUM : KOMARGCHAR;
    break;
  case 'S' :
    if(!GetCharCfgValue(line, &command->status)) {
      return 0;
    }
    break;
  case 'L' :
    if(!GetLongCfgValue(line, &command->minlogg)) {
      return 0;
    }
    break;
  case 'D' :
    if(!GetLongCfgValue(line, &command->mindays)) {
      return 0;
    }
    break;
  case 'X' :
    if(!GetStringCfgValue(line, command->losen, 19)) {
      return 0;
    }
    break;
  case 'V' :
    if(!GetStringCfgValue(line, tmp, 9)) {
      return 0;
    }
    command->secret = (tmp[0]=='N' || tmp[0]=='n') ? TRUE : FALSE;
    break;
  case 'B' :
    if(!GetStringCfgValue(line, command->beskr, 69)) {
      return 0;
    }
    break;
  case 'W' :
    if(!GetStringCfgValue(line, command->vilkainfo, 49)) {
      return 0;
    }
    break;
  case 'F' :
    if(!GetStringCfgValue(line, command->logstr, 49)) {
      return 0;
    }
    break;
  case '(' :
    if(!GetLongCfgValue(line, &command->before)) {
      return 0;
    }
    break;
  case ')' :
    if(!GetLongCfgValue(line, &command->after)) {
      return 0;
    }
    break;
  case 'G' :
    if(!GetStringCfgValue(line, tmp, 99)) {
      return 0;
    }
    group = parsegrupp(tmp);
    if(group == -1) {
      printf("Unknown group name: %s\n", tmp);
      return 0;
    }
    BAMSET((char *)&command->grupper, group);
    break;
  default:
    printf("Invalid line: %s\n", line);
    return 0;
  }
  return 1;
}

void FreeCommandMem(void) {
  struct Kommando *command;
  while((command = (struct Kommando *)RemHead((struct List *)&Servermem->kom_list))) {
    FreeMem(command, sizeof(struct Kommando));
  }
}

void ReadFileKeyConfig(void) {
  BPTR fh;
  char buffer[100];
  int x=0;
  if(!(fh = Open("NiKom:DatoCfg/Nycklar.cfg", MODE_OLDFILE))) {
    cleanup(EXIT_ERROR,"Could not open Nycklar.cfg");
  }
  while(FGets(fh, buffer, 100)) {
    if(x >= MAXNYCKLAR) {
      printf("Warning: Nycklar.cfg contains too many entries. Only read %d.\n",
             MAXNYCKLAR);
      x--;
      break;
    }
    if(buffer[0] != '\n' && buffer[0] != '*' && buffer[0] != '#') {
      buffer[40] = '\0';
      strncpy(Servermem->Nyckelnamn[x], buffer, 41);
      x++;
    }
  }
  printf("Read Nycklar.cfg, %d keys\n", x);
  Close(fh);
  Servermem->info.nycklar = x;
}

void initStatusConfigDefaults(void) {
  Servermem->cfg.st.skriv = 99;
  Servermem->cfg.st.texter = 99;
  Servermem->cfg.st.brev = 99;
  Servermem->cfg.st.medmoten = 99;
  Servermem->cfg.st.radmoten = 99;
  Servermem->cfg.st.sestatus = 99;
  Servermem->cfg.st.anv = 99;
  Servermem->cfg.st.chgstatus = 99;
  Servermem->cfg.st.bytarea = 99;
  Servermem->cfg.st.radarea = 99;
  Servermem->cfg.st.filer = 99;
  Servermem->cfg.st.laddaner = 99;
  Servermem->cfg.st.crashmail = 99;
  Servermem->cfg.st.grupper = 99;
}

void ReadStatusConfig(void) {
  initStatusConfigDefaults();
  readConfigFile("NiKom:DatoCfg/Status.cfg", handleStatusConfigLine);
}

int handleStatusConfigLine(char *line, BPTR fh) {
  if(isMatchingConfigLine(line, "SKRIV") || isMatchingConfigLine(line, "WRITE")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.skriv)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "TEXTER") || isMatchingConfigLine(line, "MANAGETEXTS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.texter)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "BREV") || isMatchingConfigLine(line, "MANAGEMAIL")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.brev)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "MEDMÖTEN") || isMatchingConfigLine(line, "JOINFORUMS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.medmoten)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "RADMÖTEN") || isMatchingConfigLine(line, "MANAGEFORUMS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.radmoten)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "SESTATUS") || isMatchingConfigLine(line, "VIEWUSERINFO")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.sestatus)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "ANVÄNDARE") || isMatchingConfigLine(line, "MANAGEUSERS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.anv)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "ÄNDSTATUS") || isMatchingConfigLine(line, "MANAGESTATUS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.chgstatus)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "BYTAREA") || isMatchingConfigLine(line, "JOINAREAS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.bytarea)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "RADAREA") || isMatchingConfigLine(line, "MANAGEAREAS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.radarea)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "FILER") || isMatchingConfigLine(line, "MANAGEFILES")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.filer)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LADDANER") || isMatchingConfigLine(line, "DOWNLOAD")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.laddaner)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "GRUPPER") || isMatchingConfigLine(line, "MANAGEGROUPS")) {
    if(!GetCharCfgValue(line, &Servermem->cfg.st.grupper)) {
      return 0;
    }
  } else {
    printf("Invalid config line: %s\n", line);
    return 0;
  }
  return 1;
}

void ReadNodeTypesConfig(void) {
  BPTR fh;
  int i;
  char buffer[100], *tmp;
  if(!(fh = Open("NiKom:DatoCfg/NodeTypes.cfg", MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Could not find NodeTypes.cfg");
  }
  for(i = 0; i < MAXNODETYPES; i++) {
    Servermem->nodetypes[i].nummer = 0;
  }
  printf("Reading NodeTypes.cfg\n");
  while(FGets(fh,buffer,100)) {
    if(buffer[0] == '\n' || buffer[0] == '#') {
      continue;
    }
    tmp = strtok(buffer, WHITESPACE);
    if(tmp == NULL || strcmp(tmp, "NODETYPE") != 0) {
      printf("Invalid config line: %s\n", buffer);
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid NodeTypes.cfg");
    }
    for(i = 0; i < MAXNODETYPES; i++) {
      if(Servermem->nodetypes[i].nummer == 0) {
        break;
      }
    }
    if(i == MAXNODETYPES) {
      Close(fh);
      cleanup(EXIT_ERROR, "Too many nodetypes defined in NodeTypes.cfg");
    }

    tmp = strtok(NULL, WHITESPACE);
    if(tmp == NULL) {
      printf("No node type number found on line: %s\n", buffer);
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid NodeTypes.cfg");
    }
    Servermem->nodetypes[i].nummer = atoi(tmp);

    tmp = strtok(NULL, WHITESPACE);
    if(tmp == NULL) {
      printf("No node type path found on line: %s\n", buffer);
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid NodeTypes.cfg");
    }
    strcpy(Servermem->nodetypes[i].path, tmp);

    tmp = strtok(NULL, "");
    if(tmp == NULL) {
      printf("No node type description found on line: %s\n", buffer);
      Close(fh);
      cleanup(EXIT_ERROR, "Invalid NodeTypes.cfg");
    }
    strcpy(Servermem->nodetypes[i].desc, tmp);
  }
  Close(fh);
}

void ReadFidoConfig(void) {
  int i;
  for(i = 0; i < 10; i++) {
    Servermem->fidodata.fd[i].domain[0] = '\0';
  }
  for(i = 0; i < 20; i++) {
    Servermem->fidodata.fa[i].namn[0] = '\0';
  }
  Servermem->fidodata.mailgroups = 0;

  readConfigFile("NiKom:DatoCfg/NiKomFido.cfg", handleFidoConfigLine);
}

int handleFidoConfigLine(char *line, BPTR fh) {
  int i, address[4], group;
  char *tmp1, *tmp2, tmpbuf[50];

  if(isMatchingConfigLine(line,"DOMAIN")) {
    for(i = 0; i < 10; i++) {
      if(!Servermem->fidodata.fd[i].domain[0]) {
        break;
      }
    }
    if(i == 10) {
      printf("Too many FidoNet domains defined.\n");
      return 0;
    }
    tmp1 = FindNextWord(line);
    Servermem->fidodata.fd[i].nummer = atoi(tmp1);
    if(Servermem->fidodata.fd[i].nummer <= 0) {
      printf("The domain number must be a positive integer: %s\n", line);
      return 0;
    }
    tmp1 = FindNextWord(tmp1);
    tmp2 = FindNextWord(tmp1);
    tmp2[-1] = '\0';
    strncpy(Servermem->fidodata.fd[i].domain, tmp1, 19);
    if(!ParseFidoAddress(tmp2, address)) {
      printf("Invalid FidoNet address '%s'\n", tmp2);
      return 0;
    }
    Servermem->fidodata.fd[i].zone = address[0];
    Servermem->fidodata.fd[i].net = address[1];
    Servermem->fidodata.fd[i].node = address[2];
    Servermem->fidodata.fd[i].point = address[3];

    tmp1 = FindNextWord(tmp2);
    strncpy(Servermem->fidodata.fd[i].zones, tmp1, 49);
  }
  else if(isMatchingConfigLine(line, "ALIAS")) {
    for(i = 0; i < 20; i++) {
      if(!Servermem->fidodata.fa[i].namn[0]) {
        break;
      }
    }
    if(i == 20) {
      printf("Too many FidoNet aliases defined.\n");
      return 0;
    }
    tmp1 = FindNextWord(line);
    Servermem->fidodata.fa[i].nummer = atoi(tmp1);
    tmp1 = FindNextWord(tmp1);
    strncpy(Servermem->fidodata.fa[i].namn, tmp1, 35);
  }
  else if(isMatchingConfigLine(line, "BOUNCE")) {
    if(!GetStringCfgValue(line, tmpbuf, 10)) {
      return 0;
    }
    if(tmpbuf[0] == 'Y' || tmpbuf[0] == 'y') {
      Servermem->fidodata.bounce = TRUE;
    }
  } else if(isMatchingConfigLine(line, "MATRIXDIR")) {
    if(!GetStringCfgValue(line, Servermem->fidodata.matrixdir, 99)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "MAILGROUP")) {
    if(!GetStringCfgValue(line, tmpbuf, 49)) {
      return 0;
    }
    group = parsegrupp(tmpbuf);
    if(group == -1) {
      printf("Unknown user group '%s'\n", tmpbuf);
      return 0;
    }
    BAMSET((char *)&Servermem->fidodata.mailgroups, group);
  }
  else if(isMatchingConfigLine(line, "MAILSTATUS")) {
    if(!GetCharCfgValue(line, &Servermem->fidodata.mailstatus)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DEFAULTORIGIN")) {
    if(!GetStringCfgValue(line, Servermem->fidodata.defaultorigin, 69)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CRASHSTATUS")) {
    if(!GetCharCfgValue(line, &Servermem->fidodata.crashstatus)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "MESSAGE_BYTE_ORDER")) {
    if(!GetStringCfgValue(line, tmpbuf, 49)) {
      return 0;
    }
    if(strcmp(tmpbuf, "BIG_ENDIAN") == 0) {
      Servermem->fidodata.littleEndianByteOrder = 0;
    } else if(strcmp(tmpbuf, "LITTLE_ENDIAN") == 0) {
      Servermem->fidodata.littleEndianByteOrder = 1;
    } else {
      printf("Invalid byte order '%s'\n", tmpbuf);
    }
  } else {
    printf("Invalid config line: %s\n", line);
    return 0;
  }
  return 1;
}
