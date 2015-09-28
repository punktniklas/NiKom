#include <dos/dos.h>
#include <proto/dos.h>

#include "DiskUtils.h"
#include "Logging.h"

int HasPartitionEnoughFreeSpace(char *path, int neededBytes) {
  struct InfoData __aligned id;
  BPTR lock;
  int neededBlocks, freeBlocks;

  if(!(lock = Lock(path, ACCESS_READ))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't get a file system lock for %s", path);
    return 0;
  }
  if(!Info(lock, &id)) {
    LogEvent(SYSTEM_LOG, ERROR, "Error calling dos/Info() for %s", path);
    UnLock(lock);
    return 0;
  }
  UnLock(lock);

  neededBlocks = neededBytes / id.id_BytesPerBlock + 1;
  freeBlocks = id.id_NumBlocks - id.id_NumBlocksUsed;

  return freeBlocks >= neededBlocks;
}
