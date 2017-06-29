#include <string.h>
#include "NiKomStr.h"
#include "StringUtils.h"

#include "CommandParser.h"

extern struct System *Servermem;

struct LangCommand *ChooseLangCommand(struct Kommando *cmd, int lang) {
  return cmd->langCmd[lang].name[0] ? &cmd->langCmd[lang] : &cmd->langCmd[0];
}

/*
 * Parses the given str as a command line command for the given language. The
 * result array is populated with pointers to struct Kommando instances of the matching
 * commands. Up to 50 pointers may be populated so the array must be large enough to
 * hold this. The actual number of matched commands (and hence pointers populated in the
 * result array) is returned from the function.
 *
 * If a pointer to a user is provided commands that are secret will not be included if
 * the user doesn't have permission to execute them.
 *
 * If a argbuf pointer is provided and the number of matched commands is 1 the arguments
 * to the given command is copied from the commandline into the argbuf array and the global
 * "argument" variable is set to point to argbuf.
 *
 * Apart from the number of matched commands the following values can also be returned.
 * -1 - The input string is empty.
 * -2 - The input string is a number (which usually is interpreted as a text number)
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
        if(matchedCommands == 50) {
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
