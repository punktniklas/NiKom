#include <proto/dos.h>
#include <stdio.h>

#include "DiskMem.h"
#include "UnitTest.h"

#include "UnitTestHooks.h"

#define TEST_PATH "RAM:NiKomTest"

void TestCreate(void) {
  int res;

  res = CreateDiskMem(TEST_PATH "/Create", 42);
  TestAssert(res > 0, "Initial create did not return >0");

  res = CreateDiskMem(TEST_PATH "/Create", 42);
  TestAssert(res == 0, "Second create did not return 0");

  res = CreateDiskMem("RAM:NiKomBad/Create", 42);
  TestAssert(res == 0, "Bad create did not return 0");
}

void TestOpenClose(void) {
  int res;
  struct DiskMem *diskMem, *diskMem2;

  res = CreateDiskMem(TEST_PATH "/Create", 42);
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

  CreateDiskMem(TEST_PATH "/Create", 42);
  diskMem = OpenDiskMem(TEST_PATH "/Create");
  TestAssert(diskMem != NULL, "Open returned NULL");
  blockSize = GetDiskMemBlockSize(diskMem);
  TestAssert(blockSize == 42, "Wrong block size returned: %d.", blockSize);
  CloseDiskMem(diskMem);
}

void TestAllocateBlock(void) {
  struct DiskMem *diskMem;
  int i, block;

  CreateDiskMem(TEST_PATH "/Create", 42);
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

void main(void) {
  RunTest(TestCreate, "Create");
  RunTest(TestOpenClose, "OpenClose");
  RunTest(TestGetBlockSize, "GetBlockSize");
  RunTest(TestAllocateBlock, "AllocateBlock");
}

void SetupTest(void) {
  Execute("delete " TEST_PATH " ALL", NULL, NULL);
  Execute("makedir " TEST_PATH, NULL, NULL);
}

void CleanupTest(void) {
  Execute("delete " TEST_PATH " ALL", NULL, NULL);
}
