#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Remove
#include "DiskMem.h"

#include "BTree.h"

struct BTreeMetadata {
  int keysPerNode, keySize, valueSize, nodeSize, rootBlock;
};

struct BTree {
  struct BTreeMetadata md;
  struct DiskMem *diskMem;
  struct BNode *root;
  char path[100];
};

struct BNode {
  int diskMemBlock;
  char noofKeys, isLeaf;
  // Followed by
  // keysPerNode * (keySize + vakueSize) bytes for key/value data
  // (keysPerNode + 1) * 4 bytes for child pointers
};

#define BTREE_NODESIZE(md) (sizeof(struct BNode) \
                              + md.keysPerNode * (md.keySize + md.valueSize) \
                              + (md.keysPerNode + 1) * sizeof(int))
#define BTREE_KEYPTR(tree, node, keyIndex) (((char *)node) \
                                            + sizeof(struct BNode)      \
                                            + (tree->md.keySize + tree->md.valueSize) * (keyIndex))
#define BTREE_VALUEPTR(tree, node, keyIndex) (((char *)node) \
                                              + sizeof(struct BNode) \
                                              + (tree->md.keySize + tree->md.valueSize) * (keyIndex) \
                                              + tree->md.keySize)
#define BTREE_CHILDPTR(tree, node, childIndex) ((int *) \
                                                (((char *)node) \
                                                 + sizeof(struct BNode) \
                                                 + tree->md.keysPerNode * (tree->md.keySize + tree->md.valueSize) \
                                                 + sizeof(int) * (childIndex)))

static int debugMode = 0;

int saveMetadata(char *path, struct BTreeMetadata *md) {
  BPTR fh;
  char fileName[100];

  sprintf(fileName, "%s.btree", path);
  if((fh = Open(fileName, MODE_NEWFILE)) == NULL) {
    return 0;
  }
  if(Write(fh, md, sizeof(struct BTreeMetadata)) != sizeof(struct BTreeMetadata)) {
    Close(fh);
    return 0;
  }
  Close(fh);
  return 1;
}

void printNode(struct BTree *tree, struct BNode *node, char *header) {
  int i;

  if(!debugMode) {
    return;
  }

  if(node->noofKeys > tree->md.keysPerNode || node->noofKeys < 0) {
    printf("%s XXXXXXXXXXX Node has invalid number of keys (%d)\n", header, node->noofKeys);
    exit(0);
  }
  printf("\n%s -----------------------\n", header);
  printf("Node: diskMemBlock = %d   noofKeys = %d   isLeaf = %d\n", node->diskMemBlock, node->noofKeys, node->isLeaf);
  for(i = 0; i < node->noofKeys; i++) {
    printf("Key %d: '%s' -> %d\n", i, BTREE_KEYPTR(tree, node, i), *((int *)BTREE_VALUEPTR(tree, node, i)));
  }
  if(!node->isLeaf) {
    for(i = 0; i <= node->noofKeys; i++) {
      printf("Child %d: %d\n", i, *BTREE_CHILDPTR(tree, node, i));
    }
  }
  printf("\n");
}

void printDebugString(char *str) {
  if(debugMode) {
    printf("### %s\n", str);
  }
}

struct BNode *allocateNode(struct BTree *tree) {
  struct BNode *node;
  if((node = AllocMem(tree->md.nodeSize, MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return NULL;
  }
  if((node->diskMemBlock = AllocateDiskMemBlock(tree->diskMem)) == -1) {
    FreeMem(node, tree->md.nodeSize);
    return NULL;
  }
  return node;
}

struct BNode *readNode(struct BTree *tree, int block) {
  struct BNode *node;

  if((node = AllocMem(tree->md.nodeSize, MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return NULL;
  }

  if(!ReadDiskMemBlock(tree->diskMem, block, node)) {
    FreeMem(node, tree->md.nodeSize);
    return NULL;
  }
  if(block != node->diskMemBlock) {
    FreeMem(node, tree->md.nodeSize);
    return NULL;
  }
  printNode(tree, node, "Reading node");
 return node;
}

int writeNode(struct BTree *tree, struct BNode *node) {
  printNode(tree, node, "Writing node");
  return WriteDiskMemBlock(tree->diskMem, node->diskMemBlock, node);
}
              
void BTreePrintRoot(struct BTree *tree) {
  printNode(tree, tree->root, "Root node");
}

void BTreePrintNodeFromBlock(struct BTree *tree, int block) {
  struct BNode *node;
  if((node = readNode(tree, block)) == NULL) {
    printf("Could not read node %d.\n", block);
    return;
  }
  printNode(tree, node, "Node");
  FreeMem(node, tree->md.nodeSize);
}

void BTreeSetDebugMode(int debug) {
  debugMode = debug;
}

int BTreeCreate(char *path, int keysPerNode, int keySize, int valueSize) {
  struct BTree *tree;
  struct BNode *node;

  if(keysPerNode % 2 == 0) {
    return 0;
  }
  
  if((tree = AllocMem(sizeof(struct BTree), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return 0;
  }
  tree->md.keysPerNode = keysPerNode;
  tree->md.keySize = keySize;
  tree->md.valueSize = valueSize;
  tree->md.nodeSize = BTREE_NODESIZE(tree->md);

  if(!CreateDiskMem(path, tree->md.nodeSize)) {
    FreeMem(tree, sizeof(struct BTree));
    return 0;
  }

  if((tree->diskMem = OpenDiskMem(path)) == NULL) {
    FreeMem(tree, sizeof(struct BTree));
    return 0;
  }
  if((node = allocateNode(tree)) == NULL) {
    CloseDiskMem(tree->diskMem);
    FreeMem(tree, sizeof(struct BTree));
    return 0;
  }
  tree->md.rootBlock = node->diskMemBlock;

  if(!saveMetadata(path, &tree->md)) {
    FreeMem(node, tree->md.nodeSize);
    CloseDiskMem(tree->diskMem);
    FreeMem(tree, sizeof(struct BTree));
    return 0;
  }

  node->noofKeys = 0;
  node->isLeaf = 1;
  if(!writeNode(tree, node)) {
    FreeMem(node, tree->md.nodeSize);
    CloseDiskMem(tree->diskMem);
    FreeMem(tree, sizeof(struct BTree));
    return 0;
  }
  
  FreeMem(node, tree->md.nodeSize);
  CloseDiskMem(tree->diskMem);
  FreeMem(tree, sizeof(struct BTree));
  return 1;
}

struct BTree *BTreeOpen(char *path) {
  struct BTree *tree;
  BPTR fh;
  char fileName[100];

  if((tree = AllocMem(sizeof(struct BTree), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return NULL;
  }

  if((tree->diskMem = OpenDiskMem(path)) == NULL) {
    FreeMem(tree, sizeof(struct BTree));
    return NULL;
  }

  sprintf(fileName, "%s.btree", path);
  if((fh = Open(fileName, MODE_OLDFILE)) == NULL) {
    CloseDiskMem(tree->diskMem);
    FreeMem(tree, sizeof(struct BTree));
    return NULL;
  }
  if(Read(fh, &tree->md, sizeof(struct BTreeMetadata)) != sizeof(struct BTreeMetadata)) {
    Close(fh);
    CloseDiskMem(tree->diskMem);
    FreeMem(tree, sizeof(struct BTree));
    return NULL;
  }
  Close(fh);

  strcpy(tree->path, path);

  if((tree->root = readNode(tree, tree->md.rootBlock)) == NULL) {
    CloseDiskMem(tree->diskMem);
    FreeMem(tree, sizeof(struct BTree));
    return NULL;
  }

  return tree;
}

void BTreeClose(struct BTree *tree) {
  CloseDiskMem(tree->diskMem);
  FreeMem(tree->root, tree->md.nodeSize);
  FreeMem(tree, sizeof(struct BTree));
}

struct BNode *splitChild(struct BTree *tree, struct BNode *parentNode, struct BNode *childNode, int insertIndex) {
  struct BNode *newNode;
  int keysToCopy, keysToMove;

  // Since keysPerNode is always odd keysToCopy is the number of keys that is both to the left
  // and to the right of the middle key. The right keys should be copied into the left slots of
  // the new node.
  keysToCopy = tree->md.keysPerNode / 2;

  // Allocate a new node and copy the right side keys and children of childNode into it.
  if((newNode = allocateNode(tree)) == NULL) {
    return NULL;
  }
  newNode->isLeaf = childNode->isLeaf;
  newNode->noofKeys = keysToCopy;
  memcpy(BTREE_KEYPTR(tree, newNode, 0),
         BTREE_KEYPTR(tree, childNode, keysToCopy + 1),
         (tree->md.keySize + tree->md.valueSize) * keysToCopy);
  if(!childNode->isLeaf) {
    memcpy(BTREE_CHILDPTR(tree, newNode, 0),
           BTREE_CHILDPTR(tree, childNode, keysToCopy + 1),
           sizeof(int) * (keysToCopy + 1));
  }
  childNode->noofKeys = keysToCopy;

  // Insert the middle key of childNode into position insertIndex in parentNode
  keysToMove = parentNode->noofKeys - insertIndex;
  if(keysToMove > 0) {
    memmove(BTREE_KEYPTR(tree, parentNode, insertIndex + 1),
            BTREE_KEYPTR(tree, parentNode, insertIndex),
            (tree->md.keySize + tree->md.valueSize) * keysToMove);
    memmove(BTREE_CHILDPTR(tree, parentNode, insertIndex + 2),
            BTREE_CHILDPTR(tree, parentNode, insertIndex + 1),
            sizeof(int) * keysToMove);
  }
  memcpy(BTREE_KEYPTR(tree, parentNode, insertIndex),
         BTREE_KEYPTR(tree, childNode, keysToCopy),
         tree->md.keySize + tree->md.valueSize);
  *BTREE_CHILDPTR(tree, parentNode, insertIndex + 1) = newNode->diskMemBlock;
  parentNode->noofKeys++;

  if(writeNode(tree, parentNode)
     && writeNode(tree, childNode)
     && writeNode(tree, newNode)) {
    return newNode;
  }
  FreeMem(newNode, tree->md.nodeSize);
  return NULL;
}

int insertNonFull(struct BTree *tree, struct BNode *node, void *key, void *value) {
  struct BNode *childNode, *newChildNode = NULL, *childToUse;
  int i, childBlock, res = 1, cmpRes;
  if(node->isLeaf) {
    printDebugString("Node is a leaf node, inserting key into node.");
    for(i = node->noofKeys - 1; i >= 0; i--) {
      cmpRes = memcmp(key, BTREE_KEYPTR(tree, node, i), tree->md.keySize);
      if(cmpRes == 0) {
        memcpy(BTREE_VALUEPTR(tree, node, i), value, tree->md.valueSize);
        return writeNode(tree, node);
      }
      if(cmpRes > 0) {
        break;
      }
      memcpy(BTREE_KEYPTR(tree, node, i + 1), BTREE_KEYPTR(tree, node, i), tree->md.keySize + tree->md.valueSize);
    }
    memcpy(BTREE_KEYPTR(tree, node, i + 1), key, tree->md.keySize);
    memcpy(BTREE_VALUEPTR(tree, node, i + 1), value, tree->md.valueSize);
    node->noofKeys++;
    if(!writeNode(tree, node)) {
      return 0;
    }
  } else {
    for(i = node->noofKeys - 1; i >= 0; i--) {
      cmpRes = memcmp(key, BTREE_KEYPTR(tree, node, i), tree->md.keySize);
      if(cmpRes == 0) {
        memcpy(BTREE_VALUEPTR(tree, node, i), value, tree->md.valueSize);
        return writeNode(tree, node);
      }
      if(cmpRes > 0) {
        break;
      }
    }
    childBlock = *BTREE_CHILDPTR(tree, node, i + 1);
    if((childNode = readNode(tree, childBlock)) == NULL) {
      return 0;
    }
    if(childNode->noofKeys == tree->md.keysPerNode) {
      printDebugString("Child is full, splitting it.");
      if((newChildNode = splitChild(tree, node, childNode, i + 1)) == NULL) {
        FreeMem(childNode, tree->md.nodeSize);
        return 0;
      }
      childToUse = memcmp(key, BTREE_KEYPTR(tree, node, i + 1), tree->md.keySize)  < 0
        ? childNode : newChildNode;
    } else {
      childToUse = childNode;
    }
    printDebugString("Descending into child.");
    res = insertNonFull(tree, childToUse, key, value);
    FreeMem(childNode, tree->md.nodeSize);
    if(newChildNode != NULL) {
      FreeMem(newChildNode, tree->md.nodeSize);
    }
  }
  return res;
}

int BTreeInsert(struct BTree *tree, void *key, void *value) {
  struct BNode *newRootNode, *oldRootNode, *newChildNode;
  int res;

  printNode(tree, tree->root, "Before BTreeInsert() - root: ");
  if(tree->root->noofKeys < tree->md.keysPerNode) {
    printDebugString("Calling insertNonFull() on root.");
    return insertNonFull(tree, tree->root, key, value);
  }

  printDebugString("The root is full, splitting it.");
  oldRootNode = tree->root;
  if((newRootNode = allocateNode(tree)) == NULL) {
    return 0;
  }
  tree->md.rootBlock = newRootNode->diskMemBlock;
  if(!saveMetadata(tree->path, &tree->md)) {
    FreeMem(newRootNode, tree->md.nodeSize);
    return 0;
  }
  tree->root = newRootNode;
  newRootNode->isLeaf = 0;
  newRootNode->noofKeys = 0;
  *BTREE_CHILDPTR(tree, newRootNode, 0) = oldRootNode->diskMemBlock;

  if((newChildNode = splitChild(tree, newRootNode, oldRootNode, 0)) == NULL) {
    FreeMem(oldRootNode, tree->md.nodeSize);
    return 0;
  }
  res = insertNonFull(tree, newRootNode, key, value);
  FreeMem(oldRootNode, tree->md.nodeSize);
  FreeMem(newChildNode, tree->md.nodeSize);
  
  return res;
}

int BTreeRemove(struct BTree *tree, void *key);

int getFromNode(struct BTree *tree, struct BNode *node, void *key, void *valueBuf);

int getFromChild(struct BTree *tree, struct BNode *node, int childIndex, void *key, void *valueBuf) {
  struct BNode *child;
  int res;
  if(node->isLeaf) {
    return 0;
  }
  if((child = readNode(tree, *BTREE_CHILDPTR(tree, node, childIndex))) == NULL) {
    return 0;
  }
  res = getFromNode(tree, child, key, valueBuf);
  FreeMem(child, tree->md.nodeSize);
  return res;
}

int getFromNode(struct BTree *tree, struct BNode *node, void *key, void *valueBuf) {
  int i, cmpRes;
  for(i = 0; i < node->noofKeys; i++) {
    cmpRes = memcmp(key, BTREE_KEYPTR(tree, node, i), tree->md.keySize);
    if(cmpRes == 0) {
      memcpy(valueBuf, BTREE_VALUEPTR(tree, node, i), tree->md.valueSize);
      return 1;
    }
    if(cmpRes < 0) {
      return getFromChild(tree, node, i, key, valueBuf);
    }
  }
  return getFromChild(tree, node, i, key, valueBuf);
}

int BTreeGet(struct BTree *tree, void *key, void *valueBuf) {
  return getFromNode(tree, tree->root, key, valueBuf);
}
