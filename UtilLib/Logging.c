#include <proto/exec.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "Logging.h"
#include "NiKomStr.h"

const static char *logFileNames[] = {
  "NiKom:Log/Usage.log",
  "NiKom:Log/System.log"
};

const static char *logLevelNames[] = {
  "DEBUG",
  "INFO",
  "VERBOSE",
  "WARN",
  "ERROR",
  "CRITICAL"
};

extern struct System *Servermem;

void LogEvent(enum LogFiles logFile, enum LogLevels logLevel, char *fmt, ...) {
  FILE *fp;
  long tid;
  struct tm *ts;
  va_list arglist;

  va_start(arglist, fmt);

  ObtainSemaphore(&Servermem->semaphores[NIKSEM_LOGFILES]);
  if(!(fp = fopen(logFileNames[logFile], "a+"))) {
    printf("LOG ERROR! Can't open log file %s\n", logFileNames[logFile]);
    ReleaseSemaphore(&Servermem->semaphores[NIKSEM_LOGFILES]);
    va_end(arglist);
    return;
  }

  time(&tid);
  ts=localtime(&tid);

  fprintf(fp, "%4d%02d%02d %02d:%02d %-8s - ",
          ts->tm_year + 1900,
          ts->tm_mon + 1,
          ts->tm_mday,
          ts->tm_hour,
          ts->tm_min,
          logLevelNames[logLevel]);
  vfprintf(fp, fmt, arglist);
  fputc('\n', fp);

  fclose(fp);
  ReleaseSemaphore(&Servermem->semaphores[NIKSEM_LOGFILES]);
  va_end(arglist);
}
