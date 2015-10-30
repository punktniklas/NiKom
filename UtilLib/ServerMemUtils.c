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
