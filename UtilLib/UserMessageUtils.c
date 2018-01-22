#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "NiKomStr.h"
#include "UserNotificationHooks.h"
#include "Logging.h"
#include "UserDataUtils.h"

#include "UserMessageUtils.h"

extern struct System *Servermem;

int linkSayString(struct SayString *newSayString, int userDataSlot) {
  struct SayString *tmpSayString;
  tmpSayString = Servermem->waitingSayMessages[userDataSlot];
  if(tmpSayString != NULL) {
    while(tmpSayString->NextSay != NULL) {
      tmpSayString = tmpSayString->NextSay;
    }
    tmpSayString->NextSay = newSayString;
    return 2;
  } else {
    Servermem->waitingSayMessages[userDataSlot] = newSayString;
    return 1;
  }
}

int addSayString(int userDataSlot, int fromUserId, char *str) {
  struct SayString *newSayString;
  int ret;

  if((newSayString = AllocMem(sizeof(struct SayString), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't allocate %d bytes for a user message.",
             sizeof(struct SayString));
    DisplayInternalError();
    return 0;
  }
  newSayString->fromuser = fromUserId;
  newSayString->timestamp = time(NULL);
  strncpy(newSayString->text, str, MAXSAYTKN - 1);
  newSayString->text[MAXSAYTKN - 1] = 0;

  ObtainSemaphore(&Servermem->semaphores[NIKSEM_USERMESSAGES]);
  ret = linkSayString(newSayString, userDataSlot);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_USERMESSAGES]);
  return ret;
}

int hasSentToUser(int userId, int sentUsers[]) {
  int i;
  
  for(i = 0; i < MAXNOD; i++) {
    if(sentUsers[i] == -1) {
      sentUsers[i] = userId;
      return FALSE;
    }
    if(sentUsers[i] == userId) {
      return TRUE;
    }
  }
  return FALSE; // Should never reach here
}


int SendUserMessage(int fromUserId, int toUserId, char *str, int messageType) {
  int slot, sentUsers[MAXNOD], i;

  if(toUserId != -1) {
    if((slot = FindUserDataSlot(toUserId)) == -1) {
      return 3;
    }
    return addSayString(slot, fromUserId, str);
  }

  for(i = 0; i < MAXNOD; i++) {
    sentUsers[i] = -1;
  }

  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->nodeInfo[i].nodeType == 0
       || Servermem->nodeInfo[i].userLoggedIn < 0
       || Servermem->nodeInfo[i].userLoggedIn == fromUserId) {
      continue;
    }
    slot = Servermem->nodeInfo[i].userDataSlot;
    if((messageType & NIK_MESSAGETYPE_LOGNOTIFY) && (Servermem->userData[slot].flaggor & NOLOGNOTIFY)) {
      continue;
    }
    if(!hasSentToUser(Servermem->nodeInfo[i].userLoggedIn, sentUsers)) {
      addSayString(slot, fromUserId, str);
    }
  }
  return 1;
}

struct SayString *UnLinkUserMessages(int userDataSlot) {
  struct SayString *ret;

  if(Servermem->waitingSayMessages[userDataSlot] == NULL) {
    return NULL;
  }
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_USERMESSAGES]);
  ret = Servermem->waitingSayMessages[userDataSlot];
  Servermem->waitingSayMessages[userDataSlot] = NULL;
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_USERMESSAGES]);
  return ret;
}

int HasUnreadUserMessages(int userDataSlot) {
  return Servermem->waitingSayMessages[userDataSlot] != NULL;
}

void ClearUserMessages(int userDataSlot) {
  struct SayString *ss, *tmp;

  ss = UnLinkUserMessages(userDataSlot);
  while(ss != NULL) {
    tmp = ss->NextSay;
    FreeMem(ss, sizeof(struct SayString));
    ss = tmp;
  }
}
