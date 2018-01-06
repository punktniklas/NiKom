#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include "NiKomStr.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "Logging.h"
#include "Languages.h"
#include "InfoFiles.h"

#include "Cmd_Misc.h"

extern char *argument;

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
