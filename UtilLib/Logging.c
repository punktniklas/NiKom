#include <dos/dos.h>
#include <dos/datetime.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdarg.h>
#include "Logging.h"
#include "NiKomStr.h"


#ifdef NIKOMLIB
void LogEvent(struct System *Servermem, enum LogFiles logFile, enum LogLevels logLevel, char *fmt, ...) {
#else
extern struct System *Servermem;

void LogEvent(enum LogFiles logFile, enum LogLevels logLevel, char *fmt, ...) {
#endif

  // For some mysterious reason these two string arrays need to be declared
  // inside the function (and not as static). If not you get NULL pointers
  // when referencing elements inside nikom.library. (Normal code works fine.)
  char *logFileNames[] = {
    "NiKom:Log/Usage.log",
    "NiKom:Log/System.log",
    "NiKom:Log/Fido.log"
  };

  char *logLevelNames[] = {
    "DEBUG",
    "INFO",
    "VERBOSE",
    "WARN",
    "ERROR",
    "CRITICAL"
  };

  BPTR fh;
  struct DateTime dt;
  char datebuf[14],timebuf[10],logbuf[500];
  va_list arglist;

  va_start(arglist, fmt);

  ObtainSemaphore(&Servermem->semaphores[NIKSEM_LOGFILES]);
  if(!(fh = Open((STRPTR)logFileNames[logFile], MODE_OLDFILE))) {
    if(!(fh = Open((STRPTR)logFileNames[logFile], MODE_NEWFILE))) {
      sprintf(logbuf, "%d - '%s'", logFile, logFileNames[logFile]);
#ifndef NIKOMLIB
      printf("LOG ERROR! Can't open log file %s\n", logFileNames[logFile]);
#endif
      ReleaseSemaphore(&Servermem->semaphores[NIKSEM_LOGFILES]);
      va_end(arglist);
      return;
    }
  }

  DateStamp(&dt.dat_Stamp);
  dt.dat_Format = FORMAT_INT;
  dt.dat_Flags = 0;
  dt.dat_StrDay = NULL;
  dt.dat_StrDate = datebuf;
  dt.dat_StrTime = timebuf;
  DateToStr(&dt);

  Seek(fh,0,OFFSET_END);
  sprintf(logbuf, "%s %s %-8s - ", datebuf, timebuf, logLevelNames[logLevel]);
  FPuts(fh,logbuf);  
  vsprintf(logbuf, fmt, arglist);
  FPuts(fh,logbuf);  
  FPutC(fh, '\n');

  Close(fh);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_LOGFILES]);
  va_end(arglist);
}
