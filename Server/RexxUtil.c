#include <rexx/storage.h>
#include <proto/rexxsyslib.h>
#include <string.h>
#include <stdio.h>

#include "RexxUtil.h"

void SetRexxResultString(struct RexxMsg *mess, char *resultStr) {
  mess->rm_Result1=0;
  if(!(mess->rm_Result2 = (long)CreateArgstring(resultStr, strlen(resultStr)))) {
    printf("Couldn't allocate an ARexx ArgString\n");
  }
}

void SetRexxErrorResult(struct RexxMsg *mess, int errorCode) {
  mess->rm_Result1 = errorCode;
  mess->rm_Result2 = NULL;
}
