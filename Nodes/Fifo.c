#include <exec/ports.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <fifoproto.h>
#include <fifo.h>
#include <string.h>
#include <stdio.h>

#include "NiKomFuncs.h"
#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "Logging.h"
#include "BasicIO.h"

#include "Fifo.h"

#define FIFOEVENT_FROMUSER  1
#define FIFOEVENT_FROMFIFO  2
#define FIFOEVENT_CLOSEW    4
#define FIFOEVENT_NOCARRIER 8

extern int nodnr;

struct Library *FifoBase;

void ExecFifo(char *command,int cooked) {
  struct MsgPort *fifoport;
  void *fiforead, *fifowrite;
  BPTR fh;
  int avail = 0, going = TRUE, event;
  char fifoName[40], *buffer = NULL, userchar, topsbuf[200];
  struct Message fiforeadmess, fifowritemess, *backmess;
  
  fiforeadmess.mn_Node.ln_Type = NT_MESSAGE;
  fiforeadmess.mn_Length = sizeof(struct Message);
  fifowritemess.mn_Node.ln_Type = NT_MESSAGE;
  fifowritemess.mn_Length = sizeof(struct Message);
  if(!(FifoBase = OpenLibrary("fifo.library", 0L))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open fifo.library.");
    DisplayInternalError();
    return;
  }

  if(!(fifoport = CreateMsgPort())) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open create a MessagePort for Fifo.");
    DisplayInternalError();
    CloseLibrary(FifoBase);
    return;
  }
  fiforeadmess.mn_ReplyPort = fifoport;
  fifowritemess.mn_ReplyPort = fifoport;

  sprintf(fifoName, "NiKomFifo%d_s", nodnr);
  if(!(fiforead = OpenFifo(fifoName, 2048, FIFOF_READ | FIFOF_NORMAL | FIFOF_NBIO))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open Fifo '%s' for reading.", fifoName);
    DisplayInternalError();
    DeleteMsgPort(fifoport);
    CloseLibrary(FifoBase);
    return;
  }

  sprintf(fifoName, "NiKomFifo%d_m", nodnr);
  if(!(fifowrite = OpenFifo(fifoName, 2048, FIFOF_WRITE | FIFOF_NORMAL))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open Fifo '%s' for writing.", fifoName);
    DisplayInternalError();
    CloseFifo(fiforead, FIFOF_EOF);
    DeleteMsgPort(fifoport);
    CloseLibrary(FifoBase);
    return;
  }

  sprintf(fifoName, "FIFO:NiKomFifo%d/%s", nodnr, cooked ? "rwkecs" : "rwkes");
  if(!(fh = Open(fifoName, MODE_OLDFILE))) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not open the Fifo file '%s'.", fifoName);
    DisplayInternalError();
    CloseFifo(fifowrite,FIFOF_EOF);
    CloseFifo(fiforead,FIFOF_EOF);
    DeleteMsgPort(fifoport);
    CloseLibrary(FifoBase);
    return;
  }

  if(SystemTags(command,
                SYS_Asynch, TRUE,
                SYS_Input, fh,
                SYS_Output, NULL,
                SYS_UserShell, TRUE) == -1) {
    LogEvent(SYSTEM_LOG, ERROR, "Could not execute the Fifo command '%s'.", command);
    DisplayInternalError();
    Close(fh);
    CloseFifo(fifowrite,FIFOF_EOF);
    CloseFifo(fiforead,FIFOF_EOF);
    DeleteMsgPort(fifoport);
    CloseLibrary(FifoBase);
    return;
  }
  SendString("\n\n\r");
  StopHeartBeat();
  RequestFifo(fiforead, &fiforeadmess, FREQ_RPEND);
  while(going) {
    event = getfifoevent(fifoport, &userchar);
    if(event & FIFOEVENT_FROMFIFO) {
      while((avail = ReadFifo(fiforead, &buffer, avail))) {
        if(avail == -1) {
          going = FALSE;
          break;
        } else {
          if(avail > 199) {
            avail=199;
          }
          strncpy(topsbuf, buffer, avail);
          topsbuf[avail] = '\0';
          SendStringNoBrk(topsbuf);
        }
      }
      RequestFifo(fiforead, &fiforeadmess, FREQ_RPEND);
    }
    if(event & FIFOEVENT_FROMUSER) {
      while(WriteFifo(fifowrite, &userchar, 1) == -2) {
        RequestFifo(fiforead, &fifowritemess, FREQ_WAVAIL);
        WaitPort(fifoport);
        while((backmess = GetMsg(fifoport)) != &fifowritemess) {
          if(backmess == &fiforeadmess) {
            while((avail = ReadFifo(fiforead, &buffer, avail))) {
              if(avail == -1) {
                going = FALSE;
                break;
              } else {
                if(avail > 199) {
                  avail=199;
                }
                strncpy(topsbuf, buffer, avail);
                topsbuf[avail] = 0;
                SendStringNoBrk(topsbuf);
              }
            }
            RequestFifo(fiforead, &fiforeadmess, FREQ_RPEND);
            WaitPort(fifoport);
          }
        }
      }
    }
    if(event & FIFOEVENT_CLOSEW) {
      RequestFifo(fiforead, &fiforeadmess, FREQ_ABORT);
      WaitPort(fifoport);
      GetMsg(fifoport);
      sprintf(fifoName, "FIFO:NiKomFifo%d/C", nodnr);
      if((fh = Open(fifoName, MODE_OLDFILE))) {
        Close(fh);
      } else {
        LogEvent(SYSTEM_LOG, ERROR,
                 "Could not open file '%s' (i.e. sending a Ctrl-C to the Fifo).", fifoName);
        DisplayInternalError();
      }
      CloseFifo(fifowrite, FIFOF_EOF);
      CloseFifo(fiforead, FIFOF_EOF);
      DeleteMsgPort(fifoport);
      CloseLibrary(FifoBase);
      cleanup(0, "");
    }
    if(event & FIFOEVENT_NOCARRIER || ImmediateLogout()) {
      going = FALSE;
    }
  }
  RequestFifo(fiforead, &fiforeadmess, FREQ_ABORT);
  WaitPort(fifoport);
  GetMsg(fifoport);

  if(ImmediateLogout()) {
    WriteFifo(fifowrite, " ", 1);
    sprintf(fifoName, "FIFO:NiKomFifo%d/C", nodnr);
    if((fh=Open(fifoName,MODE_OLDFILE))) {
      Close(fh);
    } else {
      LogEvent(SYSTEM_LOG, ERROR,
               "Could not open file '%s' (i.e. sending a Ctrl-C to the Fifo).", fifoName);
      DisplayInternalError();
    }
  }
  CloseFifo(fifowrite, FIFOF_EOF);
  CloseFifo(fiforead, FALSE);
  DeleteMsgPort(fifoport);
  CloseLibrary(FifoBase);
  StartHeartBeat(TRUE);
  return;
}
