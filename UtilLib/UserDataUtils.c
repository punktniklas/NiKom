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
    if(Servermem->inloggad[i] == userId) {
      if(unreadTexts != NULL) {
        *unreadTexts = &Servermem->unreadTexts[i];
      }
      return &Servermem->inne[i];
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
