#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "DiskMem.h"
#include "UnitTest.h"

#include "UnitTestHooks.h"

#define TEST_PATH "RAM:NiKomTest"
#define BLOCK_SIZE 42
#define LARGE_BLOCK_SIZE 32000

void TestCreate(void) {
  int res;

  res = CreateDiskMem(TEST_PATH "/Create", BLOCK_SIZE);
  TestAssert(res > 0, "Initial create did not return >0");

  res = CreateDiskMem(TEST_PATH "/Create", BLOCK_SIZE);
  TestAssert(res == 0, "Second create did not return 0");

  res = CreateDiskMem("RAM:NiKomBad/Create", BLOCK_SIZE);
  TestAssert(res == 0, "Bad create did not return 0");
}

void TestOpenClose(void) {
  int res;
  struct DiskMem *diskMem, *diskMem2;

  res = CreateDiskMem(TEST_PATH "/Create", BLOCK_SIZE);
  TestAssert(res > 0, "Create did not return >0");

  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL");
  diskMem2 = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem2 == NULL, "Open did not return NULL on already open DiskMem.");
  CloseDiskMem(diskMem);

  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL when opening the second time.");
  CloseDiskMem(diskMem);

  diskMem = OpenDiskMem(TEST_PATH "/Other");
  TestAssert(diskMem == NULL, "Open did not return NULL for non-existing path.");
}

void TestGetBlockSize(void) {
  int blockSize;
  struct DiskMem *diskMem;

  CreateDiskMem(TEST_PATH "/Create", BLOCK_SIZE);
  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL");
  blockSize = GetDiskMemBlockSize(diskMem);
  TestAssert(blockSize == BLOCK_SIZE, "Wrong block size returned: %d.", blockSize);
  CloseDiskMem(diskMem);
}

void TestAllocateBlock(void) {
  struct DiskMem *diskMem;
  int i, block;

  CreateDiskMem(TEST_PATH "/Create", BLOCK_SIZE);
  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL");
  for(i = 0; i < 18; i++) {
    block = AllocateDiskMemBlock(diskMem);
    TestAssert(block == i, "Wrong block allocated. Expected %d, got %d", i, block);
  }
  FreeDiskMemBlock(diskMem, 10);
  block = AllocateDiskMemBlock(diskMem);
  TestAssert(block == 10, "Wrong block allocated. Expected %d, got %d", 10, block);
  FreeDiskMemBlock(diskMem, 5);
  FreeDiskMemBlock(diskMem, 12);
  block = AllocateDiskMemBlock(diskMem);
  TestAssert(block == 5, "Wrong block allocated. Expected %d, got %d", 5, block);
  block = AllocateDiskMemBlock(diskMem);
  TestAssert(block == 12, "Wrong block allocated. Expected %d, got %d", 12, block);
  CloseDiskMem(diskMem);
}

void TestReadWriteSmallBlocks(void) {
  struct DiskMem *diskMem;
  int res, block, i;
  char writebuf[BLOCK_SIZE], readbuf[BLOCK_SIZE];
  
  CreateDiskMem(TEST_PATH "/Create", BLOCK_SIZE);
  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL");

  for(i = 0; i < 10; i++) {
    block = AllocateDiskMemBlock(diskMem);
    TestAssert(block != -1, "Could not allocate block.");
    memset(writebuf, 17 + i, BLOCK_SIZE);
    res = WriteDiskMemBlock(diskMem, block, writebuf);
    TestAssert(res, "Error writing block %d", block);
    res = ReadDiskMemBlock(diskMem, block, readbuf);
    TestAssert(res, "Error reading block %d", block);
    TestAssert(memcmp(writebuf, readbuf, BLOCK_SIZE) == 0,
               "Read data does not match written data for block %d.", block);
  }
  for(i = 9; i >= 0; i--) {
    memset(writebuf, 17 + i, BLOCK_SIZE);
    res = ReadDiskMemBlock(diskMem, i, readbuf);
    TestAssert(res, "Error reading block %d in pass 2", i);
    TestAssert(memcmp(writebuf, readbuf, BLOCK_SIZE) == 0,
               "Read data does not match expected data for block %d in pass 2. (%d != %d)",
               i, writebuf[0], readbuf[0]);
  }
  CloseDiskMem(diskMem);
}

void TestReadWriteLargeBlocks(void) {
  struct DiskMem *diskMem;
  int res, block, i;
  char buf[LARGE_BLOCK_SIZE];
  
  CreateDiskMem(TEST_PATH "/Create", LARGE_BLOCK_SIZE);
  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL");

  for(i = 0; i < 10; i++) {
    block = AllocateDiskMemBlock(diskMem);
    TestAssert(block != -1, "Could not allocate block.");
    buf[0] = 17 + i;
    res = WriteDiskMemBlock(diskMem, block, buf);
    TestAssert(res, "Error writing block %d", block);
  }
  for(i = 9; i>= 0; i--) {
    res = ReadDiskMemBlock(diskMem, i, buf);
    TestAssert(res, "Error reading block %d", block);
    TestAssert(buf[0] == (17 + i),
               "Read data does not match written data. Expected %d, found %d",
               (17 + i), buf[0]);
  }
  CloseDiskMem(diskMem);
}

void main(void) {
  RunTest(TestCreate, "Create");
  RunTest(TestOpenClose, "OpenClose");
  RunTest(TestGetBlockSize, "GetBlockSize");
  RunTest(TestAllocateBlock, "AllocateBlock");
  RunTest(TestReadWriteSmallBlocks, "ReadWriteSmallBlocks");
  RunTest(TestReadWriteLargeBlocks, "ReadWriteLargeBlocks");
}

void SetupTest(void) {
  Execute("delete " TEST_PATH " ALL", NULL, NULL);
  Execute("makedir " TEST_PATH, NULL, NULL);
}

void CleanupTest(void) {
  Execute("delete " TEST_PATH " ALL", NULL, NULL);
}
