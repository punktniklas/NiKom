#include <stdlib.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include "NiKomStr.h"
#include "Logging.h"
#include "StringUtils.h"
#include "UserDataUtils.h"

#include "Notifications.h"
#include "UserNotificationHooks.h"

extern struct System *Servermem;

void parseReactionNotification(char *line, struct ReactionNotif *reaction) {
  reaction->reactionType = atoi(line);
  line = FindNextWord(line);
  reaction->userId = atoi(line);
  line = FindNextWord(line);
  reaction->textId = atoi(line);
}

void parseNotificationLine(char *line, struct Notification *notification) {
  int typeInt;
  char *restLine;

  typeInt = atoi(line);
  notification->type = typeInt == 1 ? NOTIF_TYPE_REACTION : NOTIF_TYPE_UNKNOWN;
  restLine = FindNextWord(line);
  switch(notification->type) {
  case NOTIF_TYPE_REACTION:
    parseReactionNotification(restLine, &notification->reaction);
    break;
  }
}

struct Notification *doReadNotifications(int userId, int clear) {
  BPTR fh;
  char filename[40], line[100];
  int userDataSlot;
  struct Notification *res = NULL, **next = &res, *tmp;

  sprintf(filename, "NiKom:Users/%d/%d/Notifications", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    return NULL;
  }
  while(FGets(fh, line, 100)) {
    if((tmp = AllocMem(sizeof(struct Notification), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
      LogEvent(SYSTEM_LOG, ERROR, "Can't allocate %d bytes for a notification.", sizeof(struct Notification));
      DisplayInternalError();
      Close(fh);
      return res;
    }
    parseNotificationLine(line, tmp);
    *next = tmp;
    next = &tmp->next;
  }
  Close(fh);
  if(clear) {
    DeleteFile(filename);
    userDataSlot = FindUserDataSlot(userId);
    if(userDataSlot != -1) {
      Servermem->waitingNotifications[userDataSlot] = 0;
    }
  }
  return res;
}

struct Notification *ReadNotifications(int userId, int clear) {
  struct Notification *res;
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_NOTIFICATIONS]);
  res = doReadNotifications(userId, clear);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_NOTIFICATIONS]);
  return res;
}


void FreeNotifications(struct Notification *list) {
  if(list == NULL) {
    return;
  }
  FreeNotifications(list->next);
  FreeMem(list, sizeof(struct Notification));
}

int doCountNotifications(int userId) {
  BPTR fh;
  char filename[40], line[100];
  int cnt = 0;
  sprintf(filename, "NiKom:Users/%d/%d/Notifications", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    return 0;
  }
  while(FGets(fh, line, 100)) {
    cnt++;
  }
  Close(fh);
  return cnt;
}

int CountNotifications(int userId) {
  int res;
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_NOTIFICATIONS]);
  res = doCountNotifications(userId);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_NOTIFICATIONS]);
  return res;
}

void doAddNotification(int userId, struct Notification *notification) {
  BPTR fh;
  char filename[40], line[100];
  int userDataSlot;

  sprintf(filename, "NiKom:Users/%d/%d/Notifications", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    if(!(fh = Open(filename, MODE_NEWFILE))) {
      LogEvent(SYSTEM_LOG, ERROR, "Can't create notifications file '%s'.", filename);
      DisplayInternalError();
      return;
    }
  }
  Seek(fh, 0, OFFSET_END);
  switch(notification->type) {
  case NOTIF_TYPE_REACTION:
    sprintf(line, "1 %d %d %d\n",
            notification->reaction.reactionType,
            notification->reaction.userId,
            notification->reaction.textId);
    break;
  default:
    sprintf(line, "0 Invalid notification type: %d\n", notification->type);
  }
  FPuts(fh, line);
  Close(fh);

  userDataSlot = FindUserDataSlot(userId);
  if(userDataSlot != -1) {
    Servermem->waitingNotifications[userDataSlot]++;
  }
}

void AddNotification(int userId, struct Notification *notification) {
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_NOTIFICATIONS]);
  doAddNotification(userId, notification);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_NOTIFICATIONS]);  
}
