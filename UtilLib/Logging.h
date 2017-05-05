#include "NiKomStr.h"

enum LogFiles {
  USAGE_LOG = 0,
  SYSTEM_LOG = 1,
  FIDO_LOG = 2
};

enum LogLevels {
  DEBUG = 0,
  INFO = 1,
  VERBOSE = 2,
  WARN = 3,
  ERROR = 4,
  CRITICAL = 5
};

#ifdef NIKOMLIB
void LogEvent(struct System *Servermem, enum LogFiles logFile, enum LogLevels logLevel, char *fmt, ...);
#else
void LogEvent(enum LogFiles logFile, enum LogLevels logLevel, char *fmt, ...);
#endif
