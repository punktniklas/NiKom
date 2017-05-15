/* Compatibility functions for non-SAS/C compilers. */
#include <string.h>
#include <proto/dos.h>

#include "NiKomCompat.h"

/* Helper used by dfind() and getft() */
static int getfib(struct FileInfoBlock *info, const char *name)
{
  BPTR lock;
  int ret = 0;

  lock = Lock(name, ACCESS_READ);
  if(lock) {
    ret = Examine(lock, info);
    UnLock(lock);
  }

  return ret;
}

/* NOTE: Wildcards are not supported! */
int dfind(struct FileInfoBlock *info, const char *name, int attr)
{
  int ret;

  if(getfib(info, name) != 0) {
    return -1;
  }
  if(attr == 0 && info->fib_DirEntryType > 0) {
    /* Found directory but wanted file. */
    return -1;
  }
  /* Found what we wanted. */
  return 0;
}

long getft(char *name)
{
  __aligned struct FileInfoBlock fib;

  if(getfib(&fib, name) != 0) {
    return -1;
  }

  return (fib.fib_Date.ds_Days * 86400 +
          fib.fib_Date.ds_Minute * 60 +
          fib.fib_Date.ds_Tick / TICKS_PER_SECOND);
}

int stcgfn(char *node, const char *name)
{
  const char *slash;
  const char *colon;
  const char *first;
  int ret;

  slash = strrchr(name, '/');
  colon = strrchr(name, ':');
  if(colon > slash) {
    first = colon+1;
  } else if(slash > colon) {
    first = slash+1;
  } else {
    /* There cannot be a slash and colon at the same offset,
       so we know none was found. */
    first = name;
  }

  ret = strlen(first);
  /* Copy terminating nul as well. */
  memcpy(node, first, ret+1);
  return ret;
}
