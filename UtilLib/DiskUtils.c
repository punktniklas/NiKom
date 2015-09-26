#include <dos/dos.h>
#include <proto/dos.h>

#include "DiskUtils.h"

int HasPartitionEnoughFreeSpace(char *path, int neededBytes) {
  struct InfoData __aligned id;
  BPTR lock;
  int neededBlocks, freeBlocks;

  if(!(lock = Lock(path, ACCESS_READ))) {
    // TODO: Log error
    return 0;
  }
  if(!Info(lock, &id)) {
    // TODO: Log error
    UnLock(lock);
    return 0;
  }
  UnLock(lock);

  neededBlocks = neededBytes / id.id_BytesPerBlock + 1;
  freeBlocks = id.id_NumBlocks - id.id_NumBlocksUsed;

  return freeBlocks >= neededBlocks;
}
