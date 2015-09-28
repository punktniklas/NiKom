enum LogFiles {
  USAGE_LOG = 0,
  SYSTEM_LOG = 1
};

enum LogLevels {
  DEBUG = 0,
  INFO = 1,
  VERBOSE = 2,
  WARN = 3,
  ERROR = 4,
  CRITICAL = 5
};

void LogEvent(enum LogFiles logFile, enum LogLevels logLevel, char *fmt, ...);
