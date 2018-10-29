#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "UnitTestHooks.h"

void TestAssert(int cond, char *msg, ...) {
  va_list arglist;

  if(cond) {
    return;
  }

  printf("\n\n*** Assertion error: ");

  va_start(arglist, msg);
  vprintf(msg, arglist);
  va_end(arglist);

  printf("\n");
  exit(5);
}

void RunTest(void (*testFunction)(void), char *testName) {
  printf("Running test: %s...", testName);
  SetupTest();
  testFunction();
  CleanupTest();
  printf("SUCCESS!\n");
}
