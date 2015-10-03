#include <proto/exec.h>

struct MsgPort *SafePutToPort(struct Message *message, char *portname) {
  struct MsgPort *port;
  Forbid();
  port = (struct MsgPort *) FindPort(portname);
  if(port) {
    PutMsg(port, message);
  }
  Permit();
  return port;
}
