#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "BTree.h"
#include "UnitTest.h"

#include "UnitTestHooks.h"

#define TEST_PATH "RAM:NiKomTest"
//#define TEST_PATH "Niklas:tmp/Test"
#define KEYS_PER_NODE 9
#define KEYSIZE 20
#define VALUESIZE 4
#define LARGE_BLOCK_SIZE 32000

void TestCreate(void) {
  int res;

  res = BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  TestAssert(res > 0, "Initial create did not return >0");

  res = BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  TestAssert(res == 0, "Second create did not return 0");

  res = BTreeCreate("RAM:NiKomBad/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  TestAssert(res == 0, "Bad create did not return 0");

  res = BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE + 1, KEYSIZE, VALUESIZE);
  TestAssert(res == 0, "Create with even number of keys per node did not return 0");
}

void TestOpenClose(void) {
  int res;
  struct BTree *tree, *tree2;

  res = BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  TestAssert(res > 0, "Create did not return >0");

  tree = BTreeOpen(TEST_PATH "/Create");
  TestAssert(tree != NULL, "Open returned NULL");
  tree2 = BTreeOpen(TEST_PATH "/Create");
  TestAssert(tree2 == NULL, "Open did not return NULL on already open DiskMem.");
  BTreeClose(tree);

  tree = BTreeOpen(TEST_PATH "/Create");
  TestAssert(tree != NULL, "Open returned NULL when opening the second time.");
  BTreeClose(tree);

  tree = BTreeOpen(TEST_PATH "/Other");
  TestAssert(tree == NULL, "Open did not return NULL for non-existing path.");
}

void TestInsertAndGet(void) {
  struct BTree *tree;
  int i, res, value;
  char key[20], pass, keyCount = 50;
  BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  tree = BTreeOpen(TEST_PATH "/Create");
  for(pass = 'a'; pass <= 'c'; pass++) {
    for(i = 0; i < keyCount; i++) {
      memset(key, 0, 20);
      sprintf(key, "k%d%c", i, pass);
      res = BTreeInsert(tree, key, &i);
      TestAssert(res != 0, "Error inserting key %s", key);
    }
  }

  for(i = keyCount - 1; i >= 0; i--) {
    for(pass = 'a'; pass <= 'c'; pass++) {
      memset(key, 0, 20);
      sprintf(key, "k%d%c", i, pass);
      res = BTreeGet(tree, key, &value);
      TestAssert(res != 0, "Error getting key %s", key);
      TestAssert(value == i, "Unecpected value for key %s: %d", key, value);
    }
  }

  BTreeClose(tree);
}

void TestInsertReopenAndGet(void) {
  struct BTree *tree;
  int res, value;

  BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  tree = BTreeOpen(TEST_PATH "/Create");

  value = 42;
  res = BTreeInsert(tree, "k1", &value);
  TestAssert(res != 0, "Error inserting key");

  BTreeClose(tree);

  tree = BTreeOpen(TEST_PATH "/Create");

  value = 0;
  res = BTreeGet(tree, "k1", &value);
  TestAssert(res != 0, "Error getting key");
  TestAssert(value == 42, "Unecpected value for key: %d", value);

  BTreeClose(tree);
}

void TestInsertKeyTwice(void) {
  struct BTree *tree;
  int res, value;

  BTreeCreate(TEST_PATH "/Create", KEYS_PER_NODE, KEYSIZE, VALUESIZE);
  tree = BTreeOpen(TEST_PATH "/Create");

  value = 42;
  res = BTreeInsert(tree, "k1", &value);
  TestAssert(res != 0, "Error inserting key");

  value = 43;
  res = BTreeInsert(tree, "k1", &value);
  TestAssert(res != 0, "Error inserting key");

  value = 0;
  res = BTreeGet(tree, "k1", &value);
  TestAssert(res != 0, "Error getting key");
  TestAssert(value == 43, "Unecpected value for key: %d", value);

  // TODO: Assert that keycount == 1
  
  BTreeClose(tree);
}

void main(void) {
  RunTest(TestCreate, "Create");
  RunTest(TestOpenClose, "OpenClose");
  RunTest(TestInsertAndGet, "InsertAndGet");
  RunTest(TestInsertReopenAndGet, "InsertReopenAndGet");
  RunTest(TestInsertKeyTwice, "InsertKeyTwice");
}

void SetupTest(void) {
  Execute("delete " TEST_PATH " ALL", NULL, NULL);
  Execute("makedir " TEST_PATH, NULL, NULL);
}

void CleanupTest(void) {
  Execute("delete " TEST_PATH " ALL", NULL, NULL);
}
