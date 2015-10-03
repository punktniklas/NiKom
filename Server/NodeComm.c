#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include "NiKomStr.h"
#include "ServerFuncs.h"
#include "ExecUtils.h"

extern struct MsgPort *nodereplyport;

void setnodestate(struct NiKMess *mess) {
  int i, cnt = 0;
  if(mess->data != -1) {
    mess->data =
      sendnodemess(NIKMESS_SETNODESTATE, mess->data, mess->extdata1, 0L, 0L);
  } else {
    for(i = 0; i < MAXNOD; i++) {
      if(sendnodemess(NIKMESS_SETNODESTATE, i, mess->extdata1, 0L, 0L)) {
        cnt++;
      }
    }
    mess->data = cnt;
  }
}

long sendnodemess(short command, long node, long data, long extdata1, long extdata2) {
  struct NiKMess *mess;
  char portname[15];
  if(mess = (struct NiKMess *)AllocMem(sizeof(struct NiKMess),
                                       MEMF_PUBLIC | MEMF_CLEAR)) {
    mess->stdmess.mn_Node.ln_Type = NT_MESSAGE;
    mess->stdmess.mn_Length = sizeof(struct NiKMess);
    mess->stdmess.mn_ReplyPort = nodereplyport;
    mess->kommando = command;
    mess->data = data;
    mess->nod = node;
    mess->extdata1 = extdata1;
    mess->extdata2 = extdata2;
    sprintf(portname, "NiKomNode%d", node);
    if(SafePutToPort((struct Message *) mess, portname)) {
      return 1;
    } else {
      FreeMem(mess, sizeof(struct NiKMess));
    }
  }
  return 0;
}
