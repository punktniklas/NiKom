#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "NiKomStr.h"

#include "DiskMem.h"

#define BITMAP_BYTES(b) (b / 8 + (b % 8 ? 1 : 0))
#define MAX_FILE_SIZE 200000
#define BLOCKS_PER_FILE(diskMem) (MAX_FILE_SIZE / diskMem->metadata.blockSize)

struct DiskMemMetadata {
  int blockSize;
};

struct DiskMem {
  BPTR fileHandle;
  char path[100];
  struct DiskMemMetadata metadata;
};

int CreateDiskMem(char *path, int blockSize) {
  char mdFileName[100];
  BPTR lock, fh;
  int res;
  struct DiskMemMetadata md;

  sprintf(mdFileName, "%s.md", path);

  if(lock = Lock(mdFileName, ACCESS_READ)) {
    UnLock(lock);
    return 0;
  }

  if(!(fh = Open(mdFileName, MODE_NEWFILE))) {
    return 0;
  }

  md.blockSize = blockSize;
  res = (Write(fh, &md, sizeof(struct DiskMemMetadata)) == sizeof(struct DiskMemMetadata));
  Close(fh);
  return res;
}

struct DiskMem *OpenDiskMem(char *path) {
  struct DiskMem *diskMem;
  BPTR lock;
  char mdFileName[100];
  if((diskMem = AllocMem(sizeof(struct DiskMem), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return NULL;
  }
  strncpy(diskMem->path, path, 99);
  sprintf(mdFileName, "%s.md", path);
  if((lock = Lock(mdFileName, ACCESS_WRITE)) == NULL) {
    FreeMem(diskMem, sizeof(struct DiskMem));
    return NULL;
  }
  if((diskMem->fileHandle = OpenFromLock(lock)) == NULL) {
    UnLock(lock);
    FreeMem(diskMem, sizeof(struct DiskMem));
    return NULL;
  }

  if(Read(diskMem->fileHandle, &diskMem->metadata, sizeof(struct DiskMemMetadata))
     != sizeof(struct DiskMemMetadata)) {
    Close(diskMem->fileHandle);
    FreeMem(diskMem, sizeof(struct DiskMem));
    return NULL;
  }
  return diskMem;
}

void CloseDiskMem(struct DiskMem *diskMem) {
  Close(diskMem->fileHandle);
  FreeMem(diskMem, sizeof(struct DiskMem));
}

int GetDiskMemBlockSize(struct DiskMem *diskMem) {
  return diskMem->metadata.blockSize;
}

int AllocateDiskMemBlock(struct DiskMem *diskMem) {
  BPTR fh;
  long bitmapSize, bitmapBytes, oldBitmapSize;
  char bitmapFileName[100], *bitmap;
  int block, arraySize;

  sprintf(bitmapFileName, "%s.bitmap", diskMem->path);
  if((fh = Open(bitmapFileName, MODE_OLDFILE)) == NULL) {
    if((fh = Open(bitmapFileName, MODE_NEWFILE)) == NULL) {
      return -1;
    }
    bitmapSize = 0;
  } else {
    if(Read(fh, &bitmapSize, sizeof(long)) != sizeof(long)) {
      Close(fh);
      return -1;
    }
  }
  oldBitmapSize = bitmapSize;

  arraySize = bitmapSize / 8 + 1;
  if((bitmap = AllocMem(arraySize, MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    Close(fh);
    return -1;
  }
  if(bitmapSize > 0) {
    bitmapBytes = BITMAP_BYTES(bitmapSize);
    if(Read(fh, bitmap, bitmapBytes) != bitmapBytes) {
      FreeMem(bitmap, arraySize);
      Close(fh);
      return -1;
    }
  }

  for(block = 0; block < bitmapSize; block++) {
    if(!BAMTEST(bitmap, block)) {
      break;
    }
  }
  if(block == bitmapSize) {
    bitmapSize++;
  }
  BAMSET(bitmap, block);

  if(oldBitmapSize != bitmapSize) {
    if(Seek(fh, 0, OFFSET_BEGINNING) == -1) {
      FreeMem(bitmap, arraySize);
      Close(fh);
      return -1;
    }
    if(Write(fh, &bitmapSize, sizeof(long)) != sizeof(long)) {
      FreeMem(bitmap, arraySize);
      Close(fh);
      return -1;
    }
  }

  if(Seek(fh, block / 8 + sizeof(long), OFFSET_BEGINNING) == -1) {
    FreeMem(bitmap, arraySize);
    Close(fh);
    return -1;
  }
  if(Write(fh, &bitmap[block / 8], 1) != 1) {
    FreeMem(bitmap, arraySize);
    Close(fh);
    return -1;
  }
  FreeMem(bitmap, arraySize);
  Close(fh);
  return block;
}

void FreeDiskMemBlock(struct DiskMem *diskMem, int block) {
  BPTR fh;
  char bitmapFileName[100], bits;
  
  sprintf(bitmapFileName, "%s.bitmap", diskMem->path);
  if((fh = Open(bitmapFileName, MODE_OLDFILE)) == NULL) {
    return;
  }
  if(Seek(fh, block / 8 + sizeof(long), OFFSET_BEGINNING) == -1) {
    Close(fh);
    return;
  }
  if(Read(fh, &bits, 1) != 1) {
    Close(fh);
    return;
  }
  bits &= ~(1 << 7 - (block % 8));
  if(Seek(fh, block / 8 + sizeof(long), OFFSET_BEGINNING) == -1) {
    Close(fh);
    return;
  }
  if(Write(fh, &bits, 1) != 1) {
    Close(fh);
    return;
  }
  Close(fh);
}

char *dataFileName(struct DiskMem *diskMem, int block) {
  static char fileName[120];
  sprintf(fileName, "%s.data%d", diskMem->path, block / BLOCKS_PER_FILE(diskMem));
  return fileName;
}

int ReadDiskMemBlock(struct DiskMem *diskMem, int block, void *buffer) {
  char *fileName = dataFileName(diskMem, block);
  int posInFile = (block % BLOCKS_PER_FILE(diskMem)) * diskMem->metadata.blockSize;
  BPTR fh;

  if((fh = Open(fileName, MODE_OLDFILE)) == NULL) {
    return 0;
  }
  if(Seek(fh, posInFile, OFFSET_BEGINNING) == -1) {
    Close(fh);
    return 0;
  }
  if(Read(fh, buffer, diskMem->metadata.blockSize) != diskMem->metadata.blockSize) {
    Close(fh);
    return 0;
  }
  Close(fh);
  return 1;
}

int WriteDiskMemBlock(struct DiskMem *diskMem, int block, void *buffer) {
  char *fileName = dataFileName(diskMem, block);
  int posInFile = (block % BLOCKS_PER_FILE(diskMem)) * diskMem->metadata.blockSize;
  BPTR fh;

  if((fh = Open(fileName, MODE_OLDFILE)) == NULL) {
    if((fh = Open(fileName, MODE_NEWFILE)) == NULL) {
      return 0;
    }
  } else {
    if(Seek(fh, posInFile, OFFSET_BEGINNING) == -1) {
      Close(fh);
      return 0;
    }
  }
  if(Write(fh, buffer, diskMem->metadata.blockSize) != diskMem->metadata.blockSize) {
    Close(fh);
    return 0;
  }
  Close(fh);
  return 1;
}
