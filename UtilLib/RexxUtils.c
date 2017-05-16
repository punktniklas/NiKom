#include <rexx/storage.h>
#include <proto/rexxsyslib.h>
#include <string.h>

#include "RexxUtils.h"
#include "Logging.h"

void SetRexxResultString(struct RexxMsg *mess, char *resultStr) {
  mess->rm_Result1=0;
  if(resultStr != NULL && mess->rm_Action & RXFF_RESULT) {
    if(!(mess->rm_Result2 = (long)CreateArgstring(resultStr, strlen(resultStr)))) {
      LogEvent(SYSTEM_LOG, ERROR, "Couldn't allocate an ARexx ArgString.");
    }
  } else {
    mess->rm_Result2 = 0;
  }
}

void SetRexxErrorResult(struct RexxMsg *mess, int errorCode) {
  mess->rm_Result1 = errorCode;
  mess->rm_Result2 = 0;
}
