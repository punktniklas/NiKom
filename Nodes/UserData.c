#include <string.h>
#include "NiKomStr.h"
#include "UserDataUtils.h"
#include "NiKomFuncs.h"
#include "Terminal.h"

#include "UserData.h"

struct User *NodeReadUser(int userId, struct User *user) {
  struct User *retUser;
  NiKForbid();
  retUser = ReadUser(userId, user);
  NiKPermit();
  if(!retUser) {
    DisplayInternalError();
  }
  return retUser;
}

struct User *NodeWriteUser(int userId, struct User *user) {
  struct User *retUser;
  NiKForbid();
  retUser = WriteUser(userId, user);
  NiKPermit();
  if(!retUser) {
    DisplayInternalError();
  }
  return retUser;
}

struct User *GetUserData(int userId) {
  static struct User user, *loggedInUser;

  loggedInUser = GetLoggedInUser(userId);
  if(loggedInUser != NULL) {
    memcpy(&user, loggedInUser, sizeof(struct User));
    return &user;
  }
  return NodeReadUser(userId, &user);
}
