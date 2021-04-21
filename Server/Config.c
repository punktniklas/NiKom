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
#include "ServerMemUtils.h"
#include "Trie.h"

#include "Config.h"

#define EXIT_ERROR	     10
#define WHITESPACE " \t\n\r"

 // TODO: Remove need for these prototypes
int parsegrupp(char *skri);

void initSystemConfigDefaults(struct Config *cfg);
int handleSystemConfigStatusSection(char *line, BPTR fh, struct Config *cfg, int *lineCnt);
int handleSystemConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt);
int handleStatusConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt);
int handleFidoConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt);
int handleStyleSheetsConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt);
int handleCommandConfigLine(char *line, struct Kommando *command, int lineCnt);

static int currentStyleSheet;

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
    sprintf(line, "LowTextWhenBitmap0ConversionStarted=%ld\n",
            Servermem->info.lowtext);
    putsRes = FPuts(file, line);
  }
  Close(file);

  printf("Read LegacyConversion.dat\n");

  return putsRes == 0;
}

int readConfigFile(char *filename, struct Config *cfg,
                   int (*handleLine)(char *, BPTR, struct Config *, int *)) {
  BPTR fh;
  char buffer[200], *tmp;
  int len, lineCnt = 0;

  printf("Reading %s\n", filename);

  if(!(fh = Open(filename, MODE_OLDFILE))) {
    printf("Could not open config file %s.", filename);
    return 0;
  }

  for(;;) {
    if(!FGets(fh, buffer, 199)) {
      Close(fh);
      return 1;
    }
    lineCnt++;
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
    if(!handleLine(buffer, fh, cfg, &lineCnt)) {
      Close(fh);
      return 0;
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

int readSystemConfig(struct Config *cfg) {
  initSystemConfigDefaults(cfg);
  return readConfigFile("NiKom:DatoCfg/System.cfg", cfg, handleSystemConfigLine);
}

void initSystemConfigDefaults(struct Config *cfg) {
  int i;
  cfg->defaultflags = 433;
  cfg->defaultstatus = 0;
  cfg->defaultrader = 25;
  for(i = 0; i <= 100; i++) {
    cfg->maxtid[i] = 0;
    cfg->uldlratio[i] = 0;
  }
  cfg->cfgflags = NICFG_VALIDATEFILES | NICFG_CRYPTEDPASSWORDS;
  strcpy(cfg->ny, "NY");
  cfg->diskfree = 100000;
  strcpy(cfg->ultmp, "T:");
  cfg->ar.preinlogg = 0;
  cfg->ar.postinlogg = 1;
  cfg->ar.utlogg = 0;
  cfg->ar.nyanv = 0;
  cfg->ar.preup1 = 0;
  cfg->ar.preup2 = 0;
  cfg->ar.postup1 = 0;
  cfg->ar.postup2 = 0;
  cfg->ar.noright = 0;
  cfg->ar.nextmeet = 0;
  cfg->ar.nexttext = 0;
  cfg->ar.nextkom = 0;
  cfg->ar.setid = 0;
  cfg->ar.nextletter = 0;
  cfg->ar.cardropped = 0;
  cfg->logmask = 4095;
  strcpy(pubscreen, "-");
  xpos = 50;
  ypos = 50;
  cfg->logintries = 5;
  cfg->defaultcharset = 1;
}

int handleSystemConfigStatusSection(char *line, BPTR fh, struct Config *cfg, int *lineCnt) {
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
      (*lineCnt)++;
      if(line[0] == '#' || line[0] == '*' || line[0] == '\n') {
        continue;
      }
      if(StartsWith(line, "ENDSTATUS")) {
        return 1;
      } else if(isMatchingConfigLine(line, "MAXTID") || isMatchingConfigLine(line, "MAXTIME")) {
        if(!GetShortCfgValue(line, &cfg->maxtid[status], *lineCnt)) {
          return 0;
        }
      } else if(isMatchingConfigLine(line, "ULDL")) {
        if(!GetCharCfgValue(line, &cfg->uldlratio[status], *lineCnt)) {
          return 0;
        }
      } else if(StartsWith(line, "STATUS")) {
        break;
      } else {
        printf("Invalid config line %d in status section: %s\n", *lineCnt, line);
        return 0;
      }
    }
  }
}

int handleSystemConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt) {
  int len;

  if(isMatchingConfigLine(line, "DEFAULTFLAGS")) {
    if(!GetLongCfgValue(line, &cfg->defaultflags, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DEFAULTSTATUS")) {
    if(!GetCharCfgValue(line, &cfg->defaultstatus, *lineCnt)) {
      return 0;
    }
    if(cfg->defaultstatus < 0 || cfg->defaultstatus > 100) {
      printf("Invalid value for DEFAULTSTATUS, must be between 0 and 100.\n");
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DEFAULTRADER") || isMatchingConfigLine(line, "DEFAULTLINES")) {
    if(!GetCharCfgValue(line, &cfg->defaultrader, *lineCnt)) {
      return 0;
    }
  } else if(StartsWith(line, "STATUS")) {
    if(!handleSystemConfigStatusSection(line, fh, cfg, lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CLOSEDBBS")) {
    if(!GetBoolCfgFlag(line, &cfg->cfgflags, NICFG_CLOSEDBBS, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NY") || isMatchingConfigLine(line, "NEWUSERLOGIN")) {
    if(!GetStringCfgValue(line, cfg->ny, 20)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DISKFREE")) {
    if(!GetLongCfgValue(line, &cfg->diskfree, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "ULTMP")) {
    if(!GetStringCfgValue(line, cfg->ultmp, 98)) {
      return 0;
    }
    len = strlen(cfg->ultmp);
    if(cfg->ultmp[len - 1] != '/' && cfg->ultmp[len - 1] != ':') {
      cfg->ultmp[len] = '/';
      cfg->ultmp[len + 1] = '\0';
    }
  } else if(isMatchingConfigLine(line, "PREINLOGG") || isMatchingConfigLine(line, "AREXX_PRELOGIN")) {
    if(!GetLongCfgValue(line, &cfg->ar.preinlogg, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "POSTINLOGG") || isMatchingConfigLine(line, "AREXX_POSTLOGIN")) {
    if(!GetLongCfgValue(line, &cfg->ar.postinlogg, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "UTLOGG") || isMatchingConfigLine(line, "AREXX_LOGOUT")) {
    if(!GetLongCfgValue(line, &cfg->ar.utlogg, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NYANV") || isMatchingConfigLine(line, "AREXX_NEWUSER")) {
    if(!GetLongCfgValue(line, &cfg->ar.nyanv, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "PREUPLOAD1") || isMatchingConfigLine(line, "AREXX_PREUPLOAD1")) {
    if(!GetLongCfgValue(line, &cfg->ar.preup1, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "PREUPLOAD2") || isMatchingConfigLine(line, "AREXX_PREUPLOAD2")) {
    if(!GetLongCfgValue(line, &cfg->ar.preup2, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "POSTUPLOAD1") || isMatchingConfigLine(line, "AREXX_POSTUPLOAD1")) {
    if(!GetLongCfgValue(line, &cfg->ar.postup1, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "POSTUPLOAD2") || isMatchingConfigLine(line, "AREXX_POSTUPLOAD2")) {
    if(!GetLongCfgValue(line, &cfg->ar.postup2, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NORIGHT") || isMatchingConfigLine(line, "AREXX_NOPERMISSION")) {
    if(!GetLongCfgValue(line, &cfg->ar.noright, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTMEET") || isMatchingConfigLine(line, "AREXX_NEXTFORUM")) {
    if(!GetLongCfgValue(line, &cfg->ar.nextmeet, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTTEXT") || isMatchingConfigLine(line, "AREXX_NEXTTEXT")) {
    if(!GetLongCfgValue(line, &cfg->ar.nexttext, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTKOM") || isMatchingConfigLine(line, "AREXX_NEXTREPLY")) {
    if(!GetLongCfgValue(line, &cfg->ar.nextkom, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "SETID") || isMatchingConfigLine(line, "AREXX_SEETIME")) {
    if(!GetLongCfgValue(line, &cfg->ar.setid, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEXTLETTER") || isMatchingConfigLine(line, "AREXX_NEXTMAIL")) {
    if(!GetLongCfgValue(line, &cfg->ar.nextletter, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CARDROPPED") || isMatchingConfigLine(line, "AREXX_AUTOLOGOUT")) {
    if(!GetLongCfgValue(line, &cfg->ar.cardropped, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LOGMASK")) {
    if(!GetLongCfgValue(line, &cfg->logmask, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "SCREEN")) {
    if(!GetStringCfgValue(line, pubscreen, 39)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "YPOS") || isMatchingConfigLine(line, "WIN_YPOS")) {
    if(!GetLongCfgValue(line, (long *)&ypos, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "XPOS") || isMatchingConfigLine(line, "WIN_XPOS")) {
    if(!GetLongCfgValue(line, (long *)&xpos, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "VALIDERAFILER") || isMatchingConfigLine(line, "UPLOADSNOTVALIDATED")) {
    if(!GetBoolCfgFlag(line, &cfg->cfgflags, NICFG_VALIDATEFILES, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LOGINTRIES") || isMatchingConfigLine(line, "LOGINATTEMPTS")) {
    if(!GetShortCfgValue(line, &cfg->logintries, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LOCALCOLOURS") || isMatchingConfigLine(line, "LOCALCOLORS")) {
    if(!GetBoolCfgFlag(line, &cfg->cfgflags, NICFG_LOCALCOLOURS, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CRYPTEDPASSWORDS")
            || isMatchingConfigLine(line, "ENCRYPTEDPASSWORDS")) {
    if(!GetBoolCfgFlag(line, &cfg->cfgflags, NICFG_CRYPTEDPASSWORDS, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "NEWUSERCHARSET")) {
    if(!GetCharCfgValue(line, &cfg->defaultcharset, *lineCnt)) {
      return 0;
    }
  } else {
    printf("Invalid config line number %d: %s\n", *lineCnt, line);
    return 0;
  }
  return 1;
}

int populateLangCommand(struct LangCommand *langCmd, char *str) {
  if(!GetStringCfgValue(str, langCmd->name, 30)) {
    return 0;
  }
  if(langCmd->name[0] == '-') {
    langCmd->name[0] = '\0';
  } else {
    langCmd->words = CountWords(langCmd->name);
  }
  return 1;
}

int readCommandConfig(struct Config *cfg) {
  BPTR fh;
  struct Kommando *command = NULL;
  char buffer[100];
  int cnt = 0, lineCnt = 0;
  
  if(!(fh = Open("NiKom:DatoCfg/Commands.cfg", MODE_OLDFILE))) {
    printf("Kunde inte öppna NiKom:DatoCfg/Commands.cfg\n");
    return 0;
  }

  while(FGets(fh, buffer, 100)) {
    lineCnt++;
    if(buffer[0] == '\n' || buffer[0] == '*') {
      continue;
    }
    if(buffer[0] == 'N' && buffer[1] == '=') {
      if(!(command = (struct Kommando *)AllocMem(sizeof(struct Kommando),
                                                 MEMF_CLEAR | MEMF_PUBLIC))) {
        Close(fh);
        printf("Out of memory while reading Commands.cfg\n");
        return 0;
      }
      buffer[32] = '\0';
      if(!populateLangCommand(&command->langCmd[0], buffer)) {
        Close(fh);
        printf("Invalid Commands.cfg\n");
        return 0;
      }
      AddTail((struct List *)&cfg->kom_list, (struct Node *)command);
      cnt++;
      continue;
    }

    if(command == NULL) {
      printf("Line %d, found command detail line before command start (\"N=\"): %s\n", lineCnt, buffer);
      Close(fh);
      return 0;
    }
    if(!handleCommandConfigLine(buffer, command, lineCnt)) {
      Close(fh);
      return 0;
    }
  }
  Close(fh);
  cfg->noOfCommands = cnt;
  printf("Read Commands.cfg, %d commands\n", cnt);
  return 1;
}

int handleCommandConfigLine(char *line, struct Kommando *command, int lineCnt) {
  char tmp[100], *pos;
  int group, langId;

  switch(line[0]) {
  case 'N':
    if((pos = strchr(line, '=')) == NULL) {
      printf("Line %d, can't find equals sign: '%s'\n", lineCnt, line);
      return 0;
    }
    *pos = '\0';
    langId = FindLanguageId(&line[1]);
    *pos = '=';
    if(langId == -1) {
      printf("Line %d, unknown language: %s\n", lineCnt, line);
      return 0;
    }
    if(!populateLangCommand(&command->langCmd[langId], line)) {
      return 0;
    }
    break;
  case '#' :
    if(!GetLongCfgValue(line, &command->nummer, lineCnt)) {
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
    if(!GetCharCfgValue(line, &command->status, lineCnt)) {
      return 0;
    }
    break;
  case 'L' :
    if(!GetLongCfgValue(line, &command->minlogg, lineCnt)) {
      return 0;
    }
    break;
  case 'D' :
    if(!GetLongCfgValue(line, &command->mindays, lineCnt)) {
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
    if(!GetLongCfgValue(line, &command->before, lineCnt)) {
      return 0;
    }
    break;
  case ')' :
    if(!GetLongCfgValue(line, &command->after, lineCnt)) {
      return 0;
    }
    break;
  case 'G' :
    if(!GetStringCfgValue(line, tmp, 99)) {
      return 0;
    }
    group = parsegrupp(tmp);
    if(group == -1) {
      printf("Line %d, unknown group name: %s\n", lineCnt, tmp);
      return 0;
    }
    BAMSET((char *)&command->grupper, group);
    break;
  default:
    printf("Invalid line %d: %s\n", lineCnt, line);
    return 0;
  }
  return 1;
}

int readFileKeyConfig(struct Config *cfg) {
  BPTR fh;
  char buffer[100];
  int x=0;
  if(!(fh = Open("NiKom:DatoCfg/Nycklar.cfg", MODE_OLDFILE))) {
    printf("Could not open Nycklar.cfg\n");
    return 0;
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
      strncpy(cfg->fileKeys[x], buffer, 41);
      x++;
    }
  }
  printf("Read Nycklar.cfg, %d keys\n", x);
  Close(fh);
  cfg->noOfFileKeys = x;
}

void initStatusConfigDefaults(struct Config *cfg) {
  cfg->st.skriv = 99;
  cfg->st.texter = 99;
  cfg->st.brev = 99;
  cfg->st.medmoten = 99;
  cfg->st.radmoten = 99;
  cfg->st.sestatus = 99;
  cfg->st.anv = 99;
  cfg->st.chgstatus = 99;
  cfg->st.bytarea = 99;
  cfg->st.radarea = 99;
  cfg->st.filer = 99;
  cfg->st.laddaner = 99;
  cfg->st.crashmail = 99;
  cfg->st.grupper = 99;
}

int readStatusConfig(struct Config *cfg) {
  initStatusConfigDefaults(cfg);
  return readConfigFile("NiKom:DatoCfg/Status.cfg", cfg, handleStatusConfigLine);
}

int handleStatusConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt) {
  if(isMatchingConfigLine(line, "SKRIV") || isMatchingConfigLine(line, "WRITE")) {
    if(!GetCharCfgValue(line, &cfg->st.skriv, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "TEXTER") || isMatchingConfigLine(line, "MANAGETEXTS")) {
    if(!GetCharCfgValue(line, &cfg->st.texter, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "BREV") || isMatchingConfigLine(line, "MANAGEMAIL")) {
    if(!GetCharCfgValue(line, &cfg->st.brev, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "MEDMÖTEN") || isMatchingConfigLine(line, "JOINFORUMS")) {
    if(!GetCharCfgValue(line, &cfg->st.medmoten, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "RADMÖTEN") || isMatchingConfigLine(line, "MANAGEFORUMS")) {
    if(!GetCharCfgValue(line, &cfg->st.radmoten, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "SESTATUS") || isMatchingConfigLine(line, "VIEWUSERINFO")) {
    if(!GetCharCfgValue(line, &cfg->st.sestatus, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "ANVÄNDARE") || isMatchingConfigLine(line, "MANAGEUSERS")) {
    if(!GetCharCfgValue(line, &cfg->st.anv, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "ÄNDSTATUS") || isMatchingConfigLine(line, "MANAGESTATUS")) {
    if(!GetCharCfgValue(line, &cfg->st.chgstatus, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "BYTAREA") || isMatchingConfigLine(line, "JOINAREAS")) {
    if(!GetCharCfgValue(line, &cfg->st.bytarea, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "RADAREA") || isMatchingConfigLine(line, "MANAGEAREAS")) {
    if(!GetCharCfgValue(line, &cfg->st.radarea, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "FILER") || isMatchingConfigLine(line, "MANAGEFILES")) {
    if(!GetCharCfgValue(line, &cfg->st.filer, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "LADDANER") || isMatchingConfigLine(line, "DOWNLOAD")) {
    if(!GetCharCfgValue(line, &cfg->st.laddaner, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "GRUPPER") || isMatchingConfigLine(line, "MANAGEGROUPS")) {
    if(!GetCharCfgValue(line, &cfg->st.grupper, *lineCnt)) {
      return 0;
    }
  } else {
    printf("Invalid config line %d: %s\n", *lineCnt, line);
    return 0;
  }
  return 1;
}

int readNodeTypesConfig(struct Config *cfg) {
  BPTR fh;
  int i;
  char buffer[100], *tmp;
  if(!(fh = Open("NiKom:DatoCfg/NodeTypes.cfg", MODE_OLDFILE))) {
    printf("Could not find NodeTypes.cfg\n");
    return 0;
  }
  for(i = 0; i < MAXNODETYPES; i++) {
    cfg->nodetypes[i].nummer = 0;
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
      printf("Invalid NodeTypes.cfg\n");
      return 0;
    }
    for(i = 0; i < MAXNODETYPES; i++) {
      if(cfg->nodetypes[i].nummer == 0) {
        break;
      }
    }
    if(i == MAXNODETYPES) {
      Close(fh);
      printf("Too many nodetypes defined in NodeTypes.cfg\n");
      return 0;
    }

    tmp = strtok(NULL, WHITESPACE);
    if(tmp == NULL) {
      printf("No node type number found on line: %s\n", buffer);
      Close(fh);
      printf("Invalid NodeTypes.cfg\n");
      return 0;
    }
    cfg->nodetypes[i].nummer = atoi(tmp);

    tmp = strtok(NULL, WHITESPACE);
    if(tmp == NULL) {
      printf("No node type path found on line: %s\n", buffer);
      Close(fh);
      printf("Invalid NodeTypes.cfg\n");
      return 0;
    }
    strcpy(cfg->nodetypes[i].path, tmp);

    tmp = strtok(NULL, "");
    if(tmp == NULL) {
      printf("No node type description found on line: %s\n", buffer);
      Close(fh);
      printf("Invalid NodeTypes.cfg\n");
      return 0;
    }
    strcpy(cfg->nodetypes[i].desc, tmp);
  }
  Close(fh);
  return 1;
}

int readFidoConfig(struct Config *cfg) {
  int i;
  for(i = 0; i < 10; i++) {
    cfg->fidoConfig.fd[i].domain[0] = '\0';
  }
  for(i = 0; i < 20; i++) {
    cfg->fidoConfig.fa[i].namn[0] = '\0';
  }
  cfg->fidoConfig.mailgroups = 0;

  return readConfigFile("NiKom:DatoCfg/NiKomFido.cfg", cfg, handleFidoConfigLine);
}

int handleFidoConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt) {
  int i, address[4], group;
  char *tmp1, *tmp2, tmpbuf[50];

  if(isMatchingConfigLine(line,"DOMAIN")) {
    for(i = 0; i < 10; i++) {
      if(!cfg->fidoConfig.fd[i].domain[0]) {
        break;
      }
    }
    if(i == 10) {
      printf("Too many FidoNet domains defined.\n");
      return 0;
    }
    tmp1 = FindNextWord(line);
    cfg->fidoConfig.fd[i].nummer = atoi(tmp1);
    if(cfg->fidoConfig.fd[i].nummer <= 0) {
      printf("Line %d, the domain number must be a positive integer: %s\n", *lineCnt, line);
      return 0;
    }
    tmp1 = FindNextWord(tmp1);
    tmp2 = FindNextWord(tmp1);
    tmp2[-1] = '\0';
    strncpy(cfg->fidoConfig.fd[i].domain, tmp1, 19);
    if(!ParseFidoAddress(tmp2, address)) {
      printf("Line %d, invalid FidoNet address '%s'\n", *lineCnt, tmp2);
      return 0;
    }
    cfg->fidoConfig.fd[i].zone = address[0];
    cfg->fidoConfig.fd[i].net = address[1];
    cfg->fidoConfig.fd[i].node = address[2];
    cfg->fidoConfig.fd[i].point = address[3];

    tmp1 = FindNextWord(tmp2);
    strncpy(cfg->fidoConfig.fd[i].zones, tmp1, 49);
  }
  else if(isMatchingConfigLine(line, "ALIAS")) {
    for(i = 0; i < 20; i++) {
      if(!cfg->fidoConfig.fa[i].namn[0]) {
        break;
      }
    }
    if(i == 20) {
      printf("Too many FidoNet aliases defined.\n");
      return 0;
    }
    tmp1 = FindNextWord(line);
    cfg->fidoConfig.fa[i].nummer = atoi(tmp1);
    tmp1 = FindNextWord(tmp1);
    strncpy(cfg->fidoConfig.fa[i].namn, tmp1, 35);
  }
  else if(isMatchingConfigLine(line, "BOUNCE")) {
    if(!GetStringCfgValue(line, tmpbuf, 10)) {
      return 0;
    }
    if(tmpbuf[0] == 'Y' || tmpbuf[0] == 'y') {
      cfg->fidoConfig.bounce = TRUE;
    }
  } else if(isMatchingConfigLine(line, "MATRIXDIR")) {
    for(i = 0; i < 10; i++) {
      if(cfg->fidoConfig.matrixDirs[i].path[0] == '\0') {
        break;
      }
    }
    if(i == 10) {
      printf("Too many FidoNet matrix dirs defined.\n");
      return 0;
    }
    if(!(tmp1 = FindStringCfgValue(line))) {
      return 0;
    }
    CopyOneWord(cfg->fidoConfig.matrixDirs[i].path, tmp1);
    strncpy(cfg->fidoConfig.matrixDirs[i].zones, FindNextWord(tmp1), 49);
  } else if(isMatchingConfigLine(line, "MAILGROUP")) {
    if(!GetStringCfgValue(line, tmpbuf, 49)) {
      return 0;
    }
    group = parsegrupp(tmpbuf);
    if(group == -1) {
      printf("Line %d, unknown user group '%s'\n", *lineCnt, tmpbuf);
      return 0;
    }
    BAMSET((char *)&cfg->fidoConfig.mailgroups, group);
  }
  else if(isMatchingConfigLine(line, "MAILSTATUS")) {
    if(!GetCharCfgValue(line, &cfg->fidoConfig.mailstatus, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "DEFAULTORIGIN")) {
    if(!GetStringCfgValue(line, cfg->fidoConfig.defaultorigin, 69)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "CRASHSTATUS")) {
    if(!GetCharCfgValue(line, &cfg->fidoConfig.crashstatus, *lineCnt)) {
      return 0;
    }
  } else if(isMatchingConfigLine(line, "MESSAGE_BYTE_ORDER")) {
    if(!GetStringCfgValue(line, tmpbuf, 49)) {
      return 0;
    }
    if(strcmp(tmpbuf, "BIG_ENDIAN") == 0) {
      cfg->fidoConfig.littleEndianByteOrder = 0;
    } else if(strcmp(tmpbuf, "LITTLE_ENDIAN") == 0) {
      cfg->fidoConfig.littleEndianByteOrder = 1;
    } else {
      printf("Line %d, invalid byte order '%s'\n", *lineCnt, tmpbuf);
    }
  } else {
    printf("Invalid config line %d: %s\n", *lineCnt, line);
    return 0;
  }
  return 1;
}

int readStyleSheetConfig(struct Config *cfg) {
  currentStyleSheet = -1;
  return readConfigFile("NiKom:DatoCfg/StyleSheets.cfg", cfg, handleStyleSheetsConfigLine);
}

int handleStyleSheetsConfigLine(char *line, BPTR fh, struct Config *cfg, int *lineCnt) {
  struct StyleCode *styleCode;
  char *word, codeKey[20];
  if(isMatchingConfigLine(line, "STYLE")) {
    word = FindNextWord(line);
    if(word[0] == '\0') {
      printf("Line %d, no number given for stylesheet: '%s'\n", *lineCnt, line);
      return 0;
    }
    currentStyleSheet = atoi(word);
    if(currentStyleSheet < 0 || currentStyleSheet >= MAXSTYLESHEET) {
      printf("Line %d, invalid style sheet number: %d\n", *lineCnt, currentStyleSheet);
      return 0;
    }
    word = FindNextWord(word);
    if(word[0] == '\0') {
      printf("Line %d, no name given for stylesheet %d\n", *lineCnt, currentStyleSheet);
      return 0;
    }
    strcpy(cfg->styleSheets[currentStyleSheet].name, word);
    return 1;
  }
  if(isMatchingConfigLine(line, "CODE")) {
    if(currentStyleSheet == -1) {
      printf("Line %d, CODE line without any preceeding STYLE line found.\n", *lineCnt);
      return 0;
    }
   word = FindNextWord(line);
    if(word[0] == '\0') {
      printf("Line %d, no name given for code: '%s'\n", *lineCnt, line);
      return 0;
    }
    CopyOneWord(codeKey, word);
    word = FindNextWord(word);
    if(word[0] == '\0') {
      printf("Line %d, no ANSI sequence for code '%s' defined.\n", *lineCnt, codeKey);
      return 0;
    }
    if(!(styleCode = AllocMem(sizeof(struct StyleCode), MEMF_PUBLIC | MEMF_CLEAR))) {
      printf("Couldn't allocate memory for style code (%d bytes)\n", sizeof(struct StyleCode));
      return 0;
    }
    CopyOneWord(styleCode->ansi, word);
    if(!TrieAdd(codeKey, styleCode, cfg->styleSheets[currentStyleSheet].codes)) {
      printf("Could not add code '%s' to stylesheet %d.", styleCode->ansi);
      return 0;
    }
    return 1;
  }
  printf("Invalid config line %d: %s\n", *lineCnt, line);
  return 0;
}

struct Config *ReadAllConfigs(void) {
  int i;
  struct Config *cfg;

  cfg = AllocMem(sizeof(struct Config), MEMF_PUBLIC | MEMF_CLEAR);
  if(cfg == NULL) {
    printf("Couldn't allocate memory for config (%d bytes)\n", sizeof(struct Config));
    return NULL;
  }
  
  NewList((struct List *)&cfg->kom_list);
  for(i = 0; i < MAXSTYLESHEET; i++) {
    if((cfg->styleSheets[i].codes = CreateTrie()) == NULL) {
      printf("Could not allocate a trie for stylesheet.");
      FreeMem(cfg, sizeof(struct Config));
      return 0;
    }
  }
  
  if(readSystemConfig(cfg)
     && readCommandConfig(cfg)
     && readFileKeyConfig(cfg)
     && readStatusConfig(cfg)
     && readNodeTypesConfig(cfg)
     && readFidoConfig(cfg)
     && readStyleSheetConfig(cfg)) {
    return cfg;
  }
  FreeAllConfigs(cfg);
  return NULL;
}

void freeStyleCode(void *styleCode) {
  FreeMem(styleCode, sizeof(struct StyleCode));
}

void FreeAllConfigs(struct Config *cfg) {
  struct Kommando *command;
  int i;

  if(cfg == NULL) {
    return;
  }

  while((command = (struct Kommando *)RemHead((struct List *)&cfg->kom_list))) {
    FreeMem(command, sizeof(struct Kommando));
  }

  for(i = 0; i < MAXSTYLESHEET; i++) {
    FreeTrie(cfg->styleSheets[i].codes, freeStyleCode);
  }

  FreeMem(cfg, sizeof(struct Config));
}

int ReReadConfigs(void) {
  struct Config *newCfg, *oldCfg;

  newCfg = ReadAllConfigs();
  if(newCfg != NULL) {
    oldCfg = Servermem->cfg;
    Servermem->cfg = newCfg;
    FreeAllConfigs(oldCfg);
  }
  return newCfg != NULL;
}
