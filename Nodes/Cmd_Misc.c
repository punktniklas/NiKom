#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <string.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "Logging.h"
#include "Languages.h"
#include "InfoFiles.h"
#include "UserDataUtils.h"
#include "UserMessageUtils.h"

#include "Cmd_Misc.h"

extern char *argument, inmat[];
extern int inloggad;

void Cmd_ShowInfo(void) {
  struct AnchorPath *anchor;
  char fileName[100], pattern[100];

  SendString("\r\n\n");
  if(!argument[0]) {
    sendfile(CreateLocalizedInfoFilePath("Info.txt", fileName));
    return;
  }
  if((anchor = AllocMem(sizeof(struct AnchorPath), MEMF_CLEAR))) {
    sprintf(pattern,"%s#?.txt", argument);
    if(MatchFirst(CreateLocalizedInfoFilePath(pattern, fileName), anchor)) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_INFO_NO_SUCH_FILE));
      return;
    }
    sendfile(CreateLocalizedInfoFilePath(anchor->ap_Info.fib_FileName, fileName));
    MatchEnd(anchor);
    FreeMem(anchor, sizeof(struct AnchorPath));
  } else {
    LogEvent(SYSTEM_LOG, ERROR, "Could not allocate %d bytes.", sizeof(struct AnchorPath));
    DisplayInternalError();
  }
}

void Cmd_Say(void) {
  int userId;
  char *quick;

  quick = strchr(argument,',');
  if(quick) {
    *quick++ = 0;
  }
  if((userId = parsenamn(argument)) == -3) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_SAY_SYNTAX));
    return;
  }
  if(userId == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
    return;
  }
  
  if(FindUserDataSlot(userId) == -1) {
    SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_SAY_NOT_LOGGED_IN), getusername(userId));
    return;
  }
  if(!quick) {
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SAY_WHAT));
    if(getstring(EKO, MAXSAYTKN - 1, NULL)) {
      return;
    }
    if(!inmat[0]) {
      return;
    }
  }
  switch(SendUserMessage(inloggad, userId, quick ? quick : inmat, 0)) {
  case 1:
    SendStringCat("\r\n%s\r\n", CATSTR(MSG_SAY_SENT), getusername(userId));
    break;
  case 2:
    SendStringCat("\r\n%s\r\n", CATSTR(MSG_SAY_USER_HAS_UNREAD), getusername(userId));
    break;
  case 3:
    SendString("\r\n\n%s\r\n", CATSTR(MSG_SAY_USER_LOGGED_OUT));
    break;
  }
}
