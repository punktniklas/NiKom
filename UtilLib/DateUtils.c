#include <stdio.h>

#include "DateUtils.h"

#define MINUTE 60
#define HOUR (60*60)
#define DAY (24*60*60)
#define YEAR (365*24*60*60)

const char *FormatDuration(long seconds, char *buf) {
  if(seconds < HOUR) {
    sprintf(buf, "%dm", seconds / MINUTE);
  } else if(seconds < DAY) {
    sprintf(buf, "%dh%dm", seconds / HOUR, (seconds % HOUR) / MINUTE);
  } else if(seconds < YEAR) {
    sprintf(buf, "%dd%dh", seconds / DAY, (seconds % DAY) / HOUR);
  } else {
    sprintf(buf, "%dy%dd", seconds / YEAR, (seconds % YEAR) / DAY);
  }
  return buf;
}
