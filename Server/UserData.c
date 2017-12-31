#include <string.h>
#include "NiKomStr.h"
#include "UserDataUtils.h"

#include "UserData.h"

struct User *GetUserData(int userId) {
  static struct User user, *loggedInUser;

  loggedInUser = GetLoggedInUser(userId);
  if(loggedInUser != NULL) {
    memcpy(&user, loggedInUser, sizeof(struct User));
    return &user;
  }
  return ReadUser(userId, &user);
}
