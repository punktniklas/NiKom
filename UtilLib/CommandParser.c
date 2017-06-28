#include <string.h>
#include "NiKomStr.h"
#include "StringUtils.h"

#include "CommandParser.h"

extern struct System *Servermem;

struct LangCommand *ChooseLangCommand(struct Kommando *cmd, int lang) {
  return cmd->langCmd[lang].name[0] ? &cmd->langCmd[lang] : &cmd->langCmd[0];
}

/*
 * -1 - No input
 * -2 - Command is a text number
 * x  - Number of commands matched
 */
int ParseCommand(char *str, int lang, struct User *user, struct Kommando *result[], char *argbuf) {
  int argType, timeSinceFirstLogin, matchedCommands = 0;
  char *str2 = NULL, *word2, *argument;
  struct Kommando *cmd;
  struct LangCommand *langCmd;

  if(user != NULL) {
    timeSinceFirstLogin = time(NULL) - user->forst_in;
  }
  if(str[0] == 0) {
    return -1;
  }
  if(str[0] >= '0' && str[0] <= '9') {
    return -2;
  }

  str2 = FindNextWord(str);
  if(IzDigit(str2[0])) {
    argType = KOMARGNUM;
  } else if(!str2[0]) {
    argType = KOMARGINGET;
  } else {
    argType = KOMARGCHAR;
  }

  ITER_EL(cmd, Servermem->cfg->kom_list, kom_node, struct Kommando *) {
    if(cmd->secret && user != NULL) {
      if(cmd->status > user->status) continue;
      if(cmd->minlogg > user->loggin) continue;
      if(cmd->mindays * 86400 > timeSinceFirstLogin) continue;
      if(cmd->grupper && !(cmd->grupper & user->grupper)) continue;
    }
    langCmd = ChooseLangCommand(cmd, lang);
    if(langCmd->name[0] && InputMatchesWord(str, langCmd->name)) {
      word2 = FindNextWord(langCmd->name);
      if((langCmd->words == 2 && InputMatchesWord(str2, word2) && str2[0]) || langCmd->words == 1) {
        if(langCmd->words == 1) {
          if(cmd->argument == KOMARGNUM && argType == KOMARGCHAR) continue;
          if(cmd->argument == KOMARGINGET && argType != KOMARGINGET) continue;
        }
        result[matchedCommands++] = cmd;
        if(matchedCommands == 10) {
          break;
        }
      }
    }
  }
  if(matchedCommands == 1 && argbuf != NULL) {
    argument = FindNextWord(str);
    if(ChooseLangCommand(result[0], lang)->words == 2) {
      argument = FindNextWord(argument);
    }
    memset(argbuf, 0, 1081);
    strncpy(argbuf, argument, 1080);
  }
  return matchedCommands;
}

int HasUserCmdPermission(struct Kommando *cmd, struct User *user) {
  if(cmd->status > user->status
     || cmd->minlogg > user->loggin
     || cmd->mindays * 86400 > (time(NULL) - user->forst_in)
     || (cmd->grupper && !(cmd->grupper & user->grupper))) {
    return 0;
  }
  return 1;
}
