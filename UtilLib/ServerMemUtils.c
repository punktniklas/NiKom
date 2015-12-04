#include "NiKomStr.h"
#include "ServerMemUtils.h"

extern struct System *Servermem;

struct ShortUser *FindShortUser(int userId) {
  struct ShortUser *su;

  ITER_EL(su, Servermem->user_list, user_node, struct ShortUser *) {
    if(su->nummer == userId) {
      return su;
    }
  }
  return NULL;
}

struct UserGroup *FindUserGroup(int groupId) {
  struct UserGroup *ug;

  ITER_EL(ug, Servermem->grupp_list, grupp_node, struct UserGroup *) {
    if(ug->nummer == groupId) {
      return ug;
    }
  }
  return NULL;
}
