#include <stdio.h>
#include <proto/dos.h>
#include "NiKomStr.h"
#include "Logging.h"

#include "UserDataUtils.h"

extern struct System *Servermem;

struct User *ReadUser(int userId, struct User *user) {
  BPTR fh;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s", filename);
    return NULL;
  }
  if(Read(fh, (void *)user,sizeof(struct User)) == -1) {
    Close(fh);
    LogEvent(SYSTEM_LOG, ERROR, "Can't read %s", filename);
    return NULL;
  }
  Close(fh);
  return user;
}

struct User *WriteUser(int userId, struct User *user) {
  BPTR fh;
  char filename[40];
  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
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

struct User *GetLoggedInUser(int userId) {
  int i;

  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->inloggad[i] == userId) {
      return &Servermem->inne[i];
    }
  }
  return NULL;
}
