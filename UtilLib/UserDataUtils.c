#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "NiKomStr.h"
#include "Logging.h"

#include "UserDataUtils.h"
#include "UserNotificationHooks.h"

extern struct System *Servermem;

struct User *doReadUser(int userId, struct User *user) {
  BPTR fh;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    return NULL;
  }
  if(Read(fh, (void *)user, sizeof(struct User)) == -1) {
    Close(fh);
    LogEvent(SYSTEM_LOG, ERROR, "Can't read %s", filename);
    return NULL;
  }
  Close(fh);
  return user;
}

struct User *ReadUser(int userId, struct User *user) {
  struct User *retUser;
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_USERS]);
  retUser = doReadUser(userId, user);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_USERS]);
  if(!retUser) {
    DisplayInternalError();
  }
  return retUser;
}

struct User *doWriteUser(int userId, struct User *user, int newUser) {
  BPTR fh;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  if(!(fh = Open(filename, newUser ? MODE_NEWFILE : MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    return NULL;
  }
  if(Write(fh, (void *)user, sizeof(struct User)) == -1) {
    Close(fh);
    LogEvent(SYSTEM_LOG, ERROR, "Can't write %s", filename);
    return NULL;
  }
  Close(fh);
  return user;
}

struct User *WriteUser(int userId, struct User *user, int newUser) {
  struct User *retUser;
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_USERS]);
  retUser = doWriteUser(userId, user, newUser);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_USERS]);
  if(!retUser) {
    DisplayInternalError();
  }
  return retUser;
}

struct User *GetLoggedInUser(int userId, struct UnreadTexts **unreadTexts) {
  int i;

  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->nodeInfo[i].nodeType != 0 && Servermem->nodeInfo[i].userLoggedIn == userId) {
      if(unreadTexts != NULL) {
        *unreadTexts = &Servermem->unreadTexts[Servermem->nodeInfo[i].userDataSlot];
      }
      return &Servermem->userData[Servermem->nodeInfo[i].userDataSlot];
    }
  }
  return NULL;
}

struct User *GetUserData(int userId) {
  static struct User user;
  struct User *loggedInUser;

  loggedInUser = GetLoggedInUser(userId, NULL);
  if(loggedInUser != NULL) {
    memcpy(&user, loggedInUser, sizeof(struct User));
    return &user;
  }
  return ReadUser(userId, &user);
}

struct User *GetUserDataForUpdate(int userId, int *needsWrite) {
  static struct User user;
  struct User *loggedInUser;

  loggedInUser = GetLoggedInUser(userId, NULL);
  if(loggedInUser != NULL) {
    *needsWrite = FALSE;
    return loggedInUser;
  }
  *needsWrite = TRUE;
  return ReadUser(userId, &user);
}

int isSlotInUse(int slot, int nodeId) {
  int i;
  for(i = 0; i < MAXNOD; i++) {
    if(i != nodeId
       && Servermem->nodeInfo[i].nodeType != 0
       && Servermem->nodeInfo[i].userLoggedIn >= 0
       && Servermem->nodeInfo[i].userDataSlot == slot) {
      return TRUE;
    }
  }
  return FALSE;
}

int doAllocateUserDataSlot(int nodeId, int userId) {
  int i, slot;

  for(i = 0; i < MAXNOD; i++) {
    if(i != nodeId
       && Servermem->nodeInfo[i].nodeType != 0
       && Servermem->nodeInfo[i].userLoggedIn == userId) {
      Servermem->nodeInfo[nodeId].userDataSlot = Servermem->nodeInfo[i].userDataSlot;
      return 1;
    }
  }

  for(slot = 0; slot < MAXNOD; slot++) {
    if(isSlotInUse(slot, nodeId)) {
      continue;
    }
    Servermem->nodeInfo[nodeId].userDataSlot = slot;
    return 2;
  }
  return 0;
}

int AllocateUserDataSlot(int nodeId, int userId) {
  int res;
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_USERDATASLOT]);
  res = doAllocateUserDataSlot(nodeId, userId);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_USERDATASLOT]);
  return res;
}

void doReleaseUserDataSlot(int nodeId) {
  Servermem->nodeInfo[nodeId].userLoggedIn = -1;
  Servermem->nodeInfo[nodeId].userDataSlot = -1;
}

void ReleaseUserDataSlot(int nodeId) {
  ObtainSemaphore(&Servermem->semaphores[NIKSEM_USERDATASLOT]);
  doReleaseUserDataSlot(nodeId);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_USERDATASLOT]);
}

int FindUserDataSlot(int userId) {
  int i;
  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->nodeInfo[i].nodeType != 0
       && Servermem->nodeInfo[i].userLoggedIn == userId) {
      return Servermem->nodeInfo[i].userDataSlot;
    }
  }
  return -1;
}
